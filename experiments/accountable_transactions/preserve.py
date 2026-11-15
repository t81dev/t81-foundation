"""Export public test evidence only; exclude ephemeral private keys and databases."""
import argparse
from pathlib import Path
import shutil


def preserve(source, destination):
    destination.mkdir(parents=True, exist_ok=False)
    for path in sorted(source.rglob('*')):
        relative = path.relative_to(source)
        if 'witness' in relative.parts or any(p.endswith('-witness') for p in relative.parts):
            continue
        if path.is_file() and path.suffix in ('.json', '.txt', '.log'):
            target = destination / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(path, target)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path)
    parser.add_argument('destination', type=Path)
    args = parser.parse_args()
    preserve(args.source, args.destination)
