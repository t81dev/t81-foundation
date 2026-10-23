"""Independently audit native tarball bytes and compare protocol envelopes offline."""
import argparse
import hashlib
import json
from pathlib import Path
import tarfile
import tempfile
from audit_corpus import audit
from verify import canonical, unique


def inspect(path):
    with tarfile.open(path, 'r:gz') as archive:
        names = [m.name for m in archive.getmembers() if m.isfile()]
        if len(names) != len(set(names)) or any(m.issym() or m.islnk() for m in archive.getmembers()):
            raise ValueError('ambiguous archive inventory')
        data = archive.extractfile('challenge-results/corpus.jsonl').read()
        with tempfile.TemporaryDirectory() as temp:
            corpus = Path(temp)/'corpus.jsonl'
            corpus.write_bytes(data)
            outcome = audit(corpus)
        rows = [json.loads(line, object_pairs_hook=unique) for line in data.splitlines()]
        signatures = {}
        for row in rows:
            trial = f"{row['case']}-{row['arm']}-{row['repetition']}"
            for field, folder in [('protected_files', 'store'), ('evidence_files', 'evidence-store')]:
                prefix = 'challenge-results/'+trial+'/'+folder+'/'
                actual = {name[len(prefix):]:archive.extractfile(name).read().hex()
                          for name in names if name.startswith(prefix)}
                if actual != row[field]:
                    raise ValueError('archived storage differs from observed inventory')
            signatures[(row['arm'], row['case'], row['repetition'])] = canonical(row['bundle'])
        report = json.loads(archive.extractfile('challenge-results/report.json').read(), object_pairs_hook=unique)
        if report['failures']:
            raise ValueError('archive contains unexpected producer failures')
        return dict(outcome, source_revision=report['git_sha'], platform=report['platform'],
                    machine=report['machine'], actual_archived_file_bytes_match=True,
                    archive_sha256=hashlib.sha256(Path(path).read_bytes()).hexdigest()), signatures


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('archives', type=Path, nargs='+')
    args = p.parse_args()
    reports, previous = {}, None
    for path in args.archives:
        report, signatures = inspect(path)
        if previous is not None and previous != signatures:
            raise ValueError('native archives disagree at the protocol boundary')
        previous = signatures
        reports[str(path)] = report
    print(json.dumps(dict(archives=reports, protocol_bytes_equal=True, authenticated_occurrence=False), indent=2))

if __name__ == '__main__':
    main()
