"""Bounded prefix recovery and resource-checked sealing, without a new clock."""
import os
from pathlib import Path
from accounting import AccountingUnavailable, ResourceExceeded
from evidence import HEADER, write_json, load_json, seal


def preserve_failure_status(root, fallback):
    # Only completed validator failures outrank a later cap/accounting stop.
    # Guard rejections are expected and never ordinary scientific failures.
    for path in sorted(Path(root).glob('*-supervisor.json')) + sorted(Path(root).glob('*.failure.json')):
        try:
            row=load_json(path)
            if row.get('probe',0):continue
            status=row.get('status_id')
            normal='-worker-0-supervisor' in path.name or path.name.endswith('.failure.json')
            if status==65 and normal:return 65
            if status==68 and ('oracle' in path.name or path.name.endswith('.failure.json')):return 68
        except (OSError,ValueError,TypeError):continue
    return fallback


def recover_prefix(path):
    """Retain complete raw frames; move partial bytes to an explicit diagnostic.

    This repairs only physical framing after termination. It never verifies
    semantics or turns an incomplete stream into a passing stream.
    """
    path=Path(path)
    with path.open('r+b',buffering=0) as stream:
        raw=stream.read(16)
        if len(raw)!=16:raise ValueError('incomplete binary header')
        magic,kind,profile,probe,res,_=HEADER.unpack(raw)
        if magic!=b'A2R1' or kind not in (0,1) or res:
            raise ValueError('invalid binary header')
        size=88 if kind==0 else 32
        extent=path.stat().st_size-16
        count,partial=divmod(extent,size)
        if partial:
            stream.seek(16+count*size)
            tail=stream.read()
            diagnostic=path.with_name(path.name+'.partial.stderr')
            # Never overwrite previously retained evidence.
            with diagnostic.open('xb') as retained:retained.write(tail)
            stream.truncate(16+count*size)
        stream.seek(0)
        stream.write(HEADER.pack(magic,kind,profile,probe,0,count))
        os.fsync(stream.fileno())
    return dict(path=path.name,records=count,partial_bytes=partial,semantic_verification=False)


def finalize(acc, root, manifest):
    root=Path(root)
    # Sampling remains active during hashing and metadata writes, with no V/W.
    acc.checkpoint()
    for path in sorted(root.glob('*.bin')):
        acc.checkpoint()
        # Already complete streams are unchanged. Interrupted bytes stay retained.
        recover_prefix(path)
    write_json(root/'accounting.json',{'jobs':acc.jobs,'last_live_roster':acc.samples,'sampling':{'count':acc.sample_count,'maximum_gap_ns':acc.max_sample_gap_ns}})
    for _ in range(6):
        acc.checkpoint()
        manifest['resources']=acc.resources()
        write_json(root/'campaign.json',manifest)
        acc.checkpoint()
        seal(root,acc.checkpoint)
        retained=acc.check_output()
        acc.checkpoint()
        if retained==manifest['resources']['retained_bytes']:
            return
        # Fixed-width hashes settle the size of the retained-byte field after
        # index creation. Re-measure before each write; never trust an estimate.
    raise AccountingUnavailable('final retained-byte accounting did not settle')


def retain_interruption(acc, root, manifest, exc):
    """Do not retry sealing past a cap. Keep the raw prefix and failure reason."""
    root=Path(root)
    fallback=71 if isinstance(exc,ResourceExceeded) else 70 if isinstance(exc,AccountingUnavailable) else 67
    status=preserve_failure_status(root,manifest['status_id'] if manifest['status_id'] in (65,66,67,68) else fallback)
    manifest.update(status_id=status,status={65:'FAIL_CORRECTNESS',66:'FAIL_GUARD',67:'FAIL_EVIDENCE',68:'FAIL_ORACLE',70:'BLOCKED_RESOURCE_ACCOUNTING',71:'INCONCLUSIVE_RESOURCE'}[status],complete=False)
    manifest['failure_receipt']='failure.json'
    # These bounded terminal diagnostics are not another experiment or a retry.
    # No new success root is published after a failed checkpoint.
    for name in ('artifact-index.json','evidence.sha256'):
        (root/name).unlink(missing_ok=True)
    recoveries=[]
    for path in sorted(root.glob('*.bin')):
        try:recoveries.append(recover_prefix(path))
        except (OSError,ValueError) as error:recoveries.append({'path':path.name,'error':str(error)})
    write_json(root/'failure.json',{'status_id':status,'message':str(exc),'prefix_recoveries':recoveries,'sealed':False})
    # A failed resource read is unavailable, not a fabricated passing zero.
    if acc is not None:
        manifest['resources'].update(accounting_complete=acc.complete,
            elapsed_ns=acc.clock()-acc.start,rss_peak_bound_bytes=acc.peak_bound if acc.complete else None)
        try:manifest['resources']['retained_bytes']=acc.check_output()
        except ResourceExceeded:manifest['resources']['retained_bytes']=None
        write_json(root/'accounting.json',{'jobs':acc.jobs,'last_live_roster':acc.samples,'sampling':{'count':acc.sample_count,'maximum_gap_ns':acc.max_sample_gap_ns}})
    write_json(root/'campaign.json',manifest)
    return status
