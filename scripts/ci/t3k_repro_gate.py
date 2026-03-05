#!/usr/bin/env python3
import argparse
import hashlib
import math
import struct
import subprocess
from pathlib import Path

GGUF_TYPE_UINT32 = 4
GGUF_TYPE_STRING = 8
GGUF_TYPE_ARRAY = 9


def align_up(v: int, a: int) -> int:
    return v if a == 0 else ((v + a - 1) // a) * a


def read_u32(buf: bytes, off: int) -> tuple[int, int]:
    return struct.unpack_from("<I", buf, off)[0], off + 4


def read_u64(buf: bytes, off: int) -> tuple[int, int]:
    return struct.unpack_from("<Q", buf, off)[0], off + 8


def read_str(buf: bytes, off: int) -> tuple[str, int]:
    n, off = read_u64(buf, off)
    end = off + n
    if end > len(buf):
        raise ValueError("GGUF string out of bounds")
    return buf[off:end].decode("utf-8"), end


def skip_kv_value(buf: bytes, off: int, vtype: int) -> int:
    if vtype == GGUF_TYPE_UINT32:
        return off + 4
    if vtype == GGUF_TYPE_STRING:
        _, off = read_str(buf, off)
        return off
    if vtype == GGUF_TYPE_ARRAY:
        elem_type, off = read_u32(buf, off)
        elem_count, off = read_u64(buf, off)
        if elem_type != 0:
            raise ValueError(f"unsupported array elem type: {elem_type}")
        return off + elem_count
    raise ValueError(f"unsupported GGUF kv type: {vtype}")


def parse_single_t3k_payload(path: Path) -> tuple[bytes, int]:
    data = path.read_bytes()
    if len(data) < 24 or data[:4] != b"GGUF":
        raise ValueError("invalid GGUF magic")

    off = 4
    version, off = read_u32(data, off)
    if version != 3:
        raise ValueError(f"unsupported GGUF version {version}")

    tensor_count, off = read_u64(data, off)
    kv_count, off = read_u64(data, off)
    if tensor_count != 1:
        raise ValueError(f"expected 1 tensor, got {tensor_count}")

    alignment = 32
    for _ in range(kv_count):
        key, off = read_str(data, off)
        vtype, off = read_u32(data, off)
        if key == "general.alignment" and vtype == GGUF_TYPE_UINT32:
            alignment, off = read_u32(data, off)
        else:
            off = skip_kv_value(data, off, vtype)

    _, off = read_str(data, off)
    ndims, off = read_u32(data, off)
    dims: list[int] = []
    for _ in range(ndims):
        d, off = read_u64(data, off)
        dims.append(d)
    ggml_type, off = read_u32(data, off)
    if ggml_type != 99:
        raise ValueError(f"unexpected ggml type {ggml_type}")
    tensor_data_offset, off = read_u64(data, off)

    n_elem = 1
    for d in dims:
        n_elem *= d
    block_count = (n_elem + 127) // 128
    tensor_bytes = block_count * 30

    data_base = align_up(off, alignment)
    payload_off = data_base + tensor_data_offset
    payload_end = payload_off + tensor_bytes
    if payload_end > len(data):
        raise ValueError("tensor payload out of bounds")
    return data[payload_off:payload_end], n_elem


def dequantize_t3k_payload(payload: bytes, n_elem: int) -> list[float]:
    out: list[float] = []
    n_blocks = (n_elem + 127) // 128
    for b in range(n_blocks):
        boff = b * 30
        scale = struct.unpack_from("<f", payload, boff)[0]
        packed = payload[boff + 4: boff + 30]

        count = min(128, n_elem - b * 128)
        trits_seen = 0
        for pb in packed:
            rem = pb
            for _ in range(5):
                digit = rem % 3
                rem //= 3
                if trits_seen < count:
                    out.append((float(digit) - 1.0) * scale)
                trits_seen += 1
    return out


def write_f32_safetensors(path: Path, tensor_name: str, values: list[float]) -> None:
    payload = b"".join(struct.pack("<f", v) for v in values)
    header = (
        '{"' + tensor_name + '":{"dtype":"F32","shape":[' + str(len(values)) +
        '],"data_offsets":[0],"data_lengths":[' + str(len(payload)) + ']}}'
    ).encode("utf-8")
    with path.open("wb") as f:
        f.write(struct.pack("<Q", len(header)))
        f.write(header)
        f.write(payload)


def run_quantize(t81_bin: Path, src: Path, dst: Path) -> None:
    cmd = [str(t81_bin), "weights", "quantize", str(src), "--to-gguf", str(dst)]
    proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    if proc.returncode != 0:
        print(proc.stdout)
        raise RuntimeError(f"quantize failed for {src}")


def main() -> int:
    ap = argparse.ArgumentParser(description="Run T3_K reproducibility gates")
    ap.add_argument("--t81-bin", required=True)
    ap.add_argument("--workdir", required=True)
    ap.add_argument("--hash-out", required=True)
    args = ap.parse_args()

    t81_bin = Path(args.t81_bin)
    workdir = Path(args.workdir)
    hash_out = Path(args.hash_out)

    workdir.mkdir(parents=True, exist_ok=True)

    src = workdir / "fixture.safetensors"
    q1 = workdir / "fixture_q1.gguf"
    rt = workdir / "fixture_roundtrip.safetensors"
    q2 = workdir / "fixture_q2.gguf"

    values = [
        float(math.sin(i * 0.17) * 0.75 + math.cos(i * 0.11) * 0.25)
        for i in range(257)
    ]
    write_f32_safetensors(src, "tensor", values)

    run_quantize(t81_bin, src, q1)

    q1_bytes = q1.read_bytes()
    q1_sha256 = hashlib.sha256(q1_bytes).hexdigest()
    hash_out.write_text(q1_sha256 + "\n", encoding="utf-8")

    payload1, n_elem1 = parse_single_t3k_payload(q1)
    deq = dequantize_t3k_payload(payload1, n_elem1)
    write_f32_safetensors(rt, "tensor", deq)

    run_quantize(t81_bin, rt, q2)
    payload2, n_elem2 = parse_single_t3k_payload(q2)

    if n_elem1 != n_elem2:
        raise RuntimeError("invariance gate failed: element count mismatch")
    if payload1 != payload2:
        raise RuntimeError("invariance gate failed: payload mismatch")

    print(f"T3_K gates passed: sha256={q1_sha256}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
