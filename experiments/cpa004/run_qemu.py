#!/usr/bin/env python3
"""Boot fresh EFI guests and compare live records. Never equate timeout with Halt."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import select
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parent


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--qemu', default='qemu-system-aarch64')
    parser.add_argument('--firmware-code', type=Path, required=True)
    parser.add_argument('--firmware-vars', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    contract_bytes = (ROOT/'contract.json').read_bytes()
    identity = hashlib.sha256(contract_bytes).hexdigest()
    if identity != (ROOT/'contract.sha256').read_text().split()[0]:
        raise RuntimeError('experiment identity changed')
    contract = json.loads(contract_bytes)
    program = (ROOT/'program.raw13').read_bytes()
    if hashlib.sha256(program).hexdigest() != contract['program_sha256']:
        raise RuntimeError('artifact identity changed')
    for file, key in [('expected.json', 'expected_states_sha256'),
                      ('observation.schema.json', 'schema_sha256')]:
        if digest(ROOT/file) != contract[key]:
            raise RuntimeError(file+' identity changed')
    expected = json.loads((ROOT/'expected.json').read_text())
    efi = args.build/'ternaryos/qemu_slice6/BOOTAA64.EFI'
    results = []
    for run in range(1,4):
        directory = args.output/f'boot-{run}'
        directory.mkdir(parents=True, exist_ok=False)
        stage = directory/'esp'
        (stage/'EFI/BOOT').mkdir(parents=True)
        # Existing EFI entry writes its marker/report here before ExitBootServices.
        (stage/'TERNOS').mkdir()
        shutil.copyfile(efi, stage/'EFI/BOOT/BOOTAA64.EFI')
        variables = directory/'vars.fd'
        shutil.copyfile(args.firmware_vars, variables)
        command = [args.qemu, '-nodefaults', '-machine', 'virt,gic-version=3',
                   '-accel', 'tcg', '-cpu', 'cortex-a57', '-smp', '1', '-m', '512',
                   '-nographic', '-monitor', 'none', '-serial', 'stdio',
                   '-global', 'virtio-mmio.force-legacy=off',
                   '-drive', f'if=pflash,format=raw,readonly=on,file={args.firmware_code}',
                   '-drive', f'if=pflash,format=raw,file={variables}',
                   '-drive', f'id=boot0,if=none,format=raw,file=fat:rw:{stage}',
                   '-device', 'virtio-blk-device,drive=boot0,bootindex=0']
        start = time.monotonic()
        process = subprocess.Popen(command, stdin=subprocess.DEVNULL,
                                   stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        raw = bytearray()
        complete = False
        deadline = start+contract['limits']['guest_boot_seconds']
        try:
            while time.monotonic() < deadline:
                ready, _, _ = select.select([process.stdout], [], [], 0.1)
                if ready:
                    chunk = os.read(process.stdout.fileno(), 65536)
                    if not chunk:
                        break
                    raw.extend(chunk)
                    if b'\nCPA004_EXIT=' in raw and bytes(raw).split(b'CPA004_EXIT=')[-1].find(b'\n') >= 0:
                        complete = True
                        break
                if process.poll() is not None:
                    break
        finally:
            if process.poll() is None:
                process.terminate()  # Deliberate stop only after evidence/deadline; not guest Halt.
            try:
                tail, _ = process.communicate(timeout=5)
            except subprocess.TimeoutExpired:
                process.kill()
                tail, _ = process.communicate()
            raw.extend(tail)
        data = bytes(raw)
        (directory/'serial.log.gz').write_bytes(gzip.compress(data,mtime=0))
        text = data.decode('utf-8',errors='replace')
        observations = []
        errors = []
        for line in text.splitlines():
            if line.startswith('CPA004_OBSERVATION='):
                try:
                    observations.append(json.loads(line.split('=',1)[1]))
                except json.JSONDecodeError as exc:
                    errors.append(str(exc))
        markers = ['[axion] bare-metal EL1 kernel entry',
                   '[axion] ExitBootServices complete;',
                   '[cpa004] live post-ExitBootServices evaluator',
                   'CPA004_ARTIFACT_HEX=']
        offsets = [text.find(m) for m in markers]
        ordered = all(p>=0 for p in offsets) and offsets == sorted(offsets)
        hex_lines = [line.split('=',1)[1] for line in text.splitlines()
                     if line.startswith('CPA004_ARTIFACT_HEX=')]
        actual_input = len(hex_lines)==1 and hex_lines[0] == program.hex()
        equal = observations == expected
        differing = [i for i,(a,b) in enumerate(zip(observations,expected)) if a!=b]
        if len(observations)!=len(expected):
            errors.append(f'observation count {len(observations)}, expected {len(expected)}')
        guest_exit = [line for line in text.splitlines() if line.startswith('CPA004_EXIT=')]
        passed = complete and guest_exit==['CPA004_EXIT=0'] and ordered and actual_input and equal and not errors
        result = dict(run=run, command=command, supervisor_returncode=process.returncode,
                      stopped_after_guest_exit_record=complete, timeout=not complete and time.monotonic()>=deadline,
                      post_exit_boot_services_path=ordered, input_buffer_identical=actual_input,
                      observation_count=len(observations), all_states_equal=equal,
                      differing_sequences=differing, errors=errors, guest_exit_records=guest_exit,
                      raw_sha256=hashlib.sha256(data).hexdigest(), disposition='PASS' if passed else 'INCONCLUSIVE_OR_FAIL')
        (directory/'normalized.json').write_text(json.dumps(observations,indent=2)+'\n')
        (directory/'result.json').write_text(json.dumps(result,indent=2)+'\n')
        results.append(result)
        print(json.dumps(result),flush=True)
    summary = dict(experiment_identity=identity, artifact_sha256=contract['program_sha256'],
                   efi_sha256=digest(efi), firmware_code_sha256=digest(args.firmware_code),
                   firmware_vars_template_sha256=digest(args.firmware_vars),
                   qemu_version=subprocess.check_output([args.qemu,'--version'],text=True).splitlines()[0],
                   runs=results, disposition='PASS' if all(r['disposition']=='PASS' for r in results) else 'PARTIAL_OR_FAIL')
    (args.output/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
    if summary['disposition'] != 'PASS':
        raise SystemExit(1)


if __name__ == '__main__':
    main()
