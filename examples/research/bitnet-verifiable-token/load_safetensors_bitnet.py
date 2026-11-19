#!/usr/bin/env python3
"""Zero-dependency HuggingFace .safetensors -> BitNet b1.58 Dual-Rail (P, N) ingester.

Parses real .safetensors binary headers (F32 / F16 / I8), quantizes weight matrices
using the BitNet b1.58 absmean rule round(clip(W / mean(|W|), -1, +1)), packs them
into orthogonal Dual-Rail (P, N) 64-bit bitplanes (0.25 B/trit: 4x smaller than int8,
8x smaller than FP16), and emits a canonical SHA3-512 manifest.

Usage:
  python3 load_safetensors_bitnet.py --self-test
  python3 load_safetensors_bitnet.py --input model.safetensors [--out-dir ./dualrail_bundle]
"""

import argparse
import hashlib
import json
import math
import struct
import tempfile
from pathlib import Path
from typing import Dict, List, Tuple


def read_safetensors(path: Path) -> Tuple[Dict[str, dict], bytes]:
    raw = path.read_bytes()
    if len(raw) < 8:
        raise ValueError("safetensors file too small")
    header_len = struct.unpack("<Q", raw[:8])[0]
    if 8 + header_len > len(raw):
        raise ValueError("truncated safetensors header")
    header = json.loads(raw[8 : 8 + header_len].decode("utf-8"))
    data_buf = raw[8 + header_len :]
    return header, data_buf


def decode_tensor_floats(dtype: str, blob: bytes) -> List[float]:
    if dtype == "F32":
        count = len(blob) // 4
        return list(struct.unpack(f"<{count}f", blob))
    if dtype == "F16":
        count = len(blob) // 2
        return list(struct.unpack(f"<{count}e", blob))
    if dtype == "I8":
        count = len(blob)
        return [float(x) for x in struct.unpack(f"<{count}b", blob)]
    raise ValueError(f"unsupported safetensors dtype: {dtype}")


def quantize_bitnet_b158(vals: List[float]) -> List[int]:
    if not vals:
        return []
    gamma = max(sum(abs(v) for v in vals) / len(vals), 1e-6)
    out = []
    for v in vals:
        scaled = max(-1.0, min(1.0, v / gamma))
        out.append(int(math.floor(scaled + 0.5)) if scaled >= 0.0 else int(math.ceil(scaled - 0.5)))
    return out


def pack_dualrail_64(trits: List[int]) -> Tuple[bytes, bytes]:
    words = (len(trits) + 63) // 64
    pos = bytearray(words * 8)
    neg = bytearray(words * 8)
    for w in range(words):
        p = 0
        n = 0
        base = w * 64
        lim = min(len(trits), base + 64)
        for i in range(base, lim):
            t = trits[i]
            if t > 0:
                p |= 1 << (i - base)
            elif t < 0:
                n |= 1 << (i - base)
        struct.pack_into("<Q", pos, w * 8, p)
        struct.pack_into("<Q", neg, w * 8, n)
    return bytes(pos), bytes(neg)


def unpack_dualrail_64(pos: bytes, neg: bytes, n_trits: int) -> List[int]:
    out = [0] * n_trits
    words = len(pos) // 8
    for w in range(words):
        p = struct.unpack_from("<Q", pos, w * 8)[0]
        n = struct.unpack_from("<Q", neg, w * 8)[0]
        if (p & n) != 0:
            raise ValueError("dual-rail disjoint invariant violated (pos & neg != 0)")
        base = w * 64
        lim = min(n_trits, base + 64)
        for i in range(base, lim):
            shift = i - base
            out[i] = ((p >> shift) & 1) - ((n >> shift) & 1)
    return out


def pack_sbdr1(pos: bytes, neg: bytes) -> bytes:
    """Pack Dual-Rail (pos, neg) planes into canonical SBDR1 sparse-block wire format.

    Layout (little-endian):
      magic[8]       = b"SBDR1\\x00\\x00\\x00"
      total_words    = uint64 (number of 64-trit words)
      mask[M]        = uint64[(total_words + 63) // 64] (bit w set iff (pos[w] | neg[w]) != 0)
      active_pairs   = (uint64 pos, uint64 neg) for each set bit in ascending order
    """
    if len(pos) != len(neg) or (len(pos) % 8) != 0:
        raise ValueError("pos/neg must have equal lengths divisible by 8")
    total_words = len(pos) // 8
    mask_words = (total_words + 63) // 64
    mask = [0] * mask_words
    payload = bytearray()
    for w in range(total_words):
        p = struct.unpack_from("<Q", pos, w * 8)[0]
        n = struct.unpack_from("<Q", neg, w * 8)[0]
        if (p & n) != 0:
            raise ValueError("dual-rail disjoint invariant violated")
        if (p | n) != 0:
            mask[w >> 6] |= 1 << (w & 63)
            payload.extend(struct.pack("<QQ", p, n))
    out = bytearray(b"SBDR1\x00\x00\x00")
    out.extend(struct.pack("<Q", total_words))
    for mw in mask:
        out.extend(struct.pack("<Q", mw))
    out.extend(payload)
    return bytes(out)


def unpack_sbdr1(blob: bytes) -> Tuple[bytes, bytes]:
    if len(blob) < 16 or blob[:8] != b"SBDR1\x00\x00\x00":
        raise ValueError("invalid SBDR1 magic")
    total_words = struct.unpack_from("<Q", blob, 8)[0]
    mask_words = (total_words + 63) // 64
    header_bytes = 16 + mask_words * 8
    if len(blob) < header_bytes:
        raise ValueError("truncated SBDR1 mask")
    pos = bytearray(total_words * 8)
    neg = bytearray(total_words * 8)
    off = header_bytes
    for m in range(mask_words):
        mw = struct.unpack_from("<Q", blob, 16 + m * 8)[0]
        for bit in range(64):
            if (mw >> bit) & 1:
                w = (m << 6) + bit
                if w >= total_words or off + 16 > len(blob):
                    raise ValueError("corrupt SBDR1 payload")
                p, n = struct.unpack_from("<QQ", blob, off)
                off += 16
                struct.pack_into("<Q", pos, w * 8, p)
                struct.pack_into("<Q", neg, w * 8, n)
    if off != len(blob):
        raise ValueError("trailing bytes in SBDR1 payload")
    return bytes(pos), bytes(neg)


def ingest_safetensors(path: Path, out_dir: Path | None = None, emit_sbdr: bool = False) -> dict:
    header, data_buf = read_safetensors(path)
    entries = []
    total_src_bytes = 0
    total_dr_bytes = 0
    total_sbdr_bytes = 0
    manifest_hasher = hashlib.sha3_512()

    if out_dir is not None:
        out_dir.mkdir(parents=True, exist_ok=True)

    for name in sorted(k for k in header.keys() if k != "__metadata__"):
        meta = header[name]
        dtype = meta["dtype"]
        shape = meta["shape"]
        start, end = meta["data_offsets"]
        blob = data_buf[start:end]
        total_src_bytes += len(blob)

        vals = decode_tensor_floats(dtype, blob)
        trits = quantize_bitnet_b158(vals)
        pos, neg = pack_dualrail_64(trits)
        total_dr_bytes += len(pos) + len(neg)

        sbdr_blob = pack_sbdr1(pos, neg) if emit_sbdr else None
        if sbdr_blob is not None:
            total_sbdr_bytes += len(sbdr_blob)

        tensor_digest = hashlib.sha3_512(pos + neg).hexdigest()
        manifest_hasher.update(f"{name}:{shape}:{tensor_digest}\n".encode("utf-8"))

        if out_dir is not None:
            safe = name.replace("/", "_").replace(".", "_")
            (out_dir / f"{safe}.pos.bin").write_bytes(pos)
            (out_dir / f"{safe}.neg.bin").write_bytes(neg)
            if sbdr_blob is not None:
                (out_dir / f"{safe}.sbdr").write_bytes(sbdr_blob)

        entry = {
            "name": name,
            "src_dtype": dtype,
            "shape": shape,
            "trits": len(trits),
            "src_bytes": len(blob),
            "dualrail_bytes": len(pos) + len(neg),
            "sha3_512": f"sha3-512:{tensor_digest}",
        }
        if sbdr_blob is not None:
            entry["sbdr_bytes"] = len(sbdr_blob)
            entry["sbdr_sha3_512"] = f"sha3-512:{hashlib.sha3_512(sbdr_blob).hexdigest()}"
        entries.append(entry)

    out = {
        "schema": "t81.bitnet-safetensors-ingest.v1",
        "ok": True,
        "source_file": str(path.name),
        "tensor_count": len(entries),
        "total_src_bytes": total_src_bytes,
        "total_dualrail_bytes": total_dr_bytes,
        "compression_ratio": round(total_src_bytes / max(total_dr_bytes, 1), 2),
        "manifest_sha3_512": f"sha3-512:{manifest_hasher.hexdigest()}",
        "tensors": entries,
    }
    if emit_sbdr:
        out["total_sbdr_bytes"] = total_sbdr_bytes
    return out


def write_synthetic_safetensors(path: Path) -> None:
    # Build a valid HuggingFace .safetensors file with an F16 weight matrix (64x128)
    # and an F32 projection matrix (64x64).
    w_f16 = [((i * 17) % 11 - 5) * 0.25 for i in range(64 * 128)]
    w_f32 = [((i * 31) % 9 - 4) * 0.30 for i in range(64 * 64)]
    b_f16 = struct.pack(f"<{len(w_f16)}e", *w_f16)
    b_f32 = struct.pack(f"<{len(w_f32)}f", *w_f32)
    header = {
        "__metadata__": {"format": "pt", "quant_target": "bitnet-b1.58"},
        "layers.0.mlp.up_proj.weight": {
            "dtype": "F16",
            "shape": [64, 128],
            "data_offsets": [0, len(b_f16)],
        },
        "layers.0.self_attn.o_proj.weight": {
            "dtype": "F32",
            "shape": [64, 64],
            "data_offsets": [len(b_f16), len(b_f16) + len(b_f32)],
        },
    }
    hdr_bytes = json.dumps(header, separators=(",", ":")).encode("utf-8")
    path.write_bytes(struct.pack("<Q", len(hdr_bytes)) + hdr_bytes + b_f16 + b_f32)


def run_self_test() -> int:
    with tempfile.TemporaryDirectory(prefix="t81_safetensors_") as td:
        root = Path(td)
        sf = root / "bitnet_test.safetensors"
        out_dir = root / "dualrail_bundle"
        write_synthetic_safetensors(sf)

        res1 = ingest_safetensors(sf, out_dir, emit_sbdr=True)
        res2 = ingest_safetensors(sf, emit_sbdr=True)
        assert res1["manifest_sha3_512"] == res2["manifest_sha3_512"]
        assert res1["tensor_count"] == 2

        # Verify lossless DualRail & SBDR1 unpack & disjointness on persisted bundle
        pos = (out_dir / "layers_0_mlp_up_proj_weight.pos.bin").read_bytes()
        neg = (out_dir / "layers_0_mlp_up_proj_weight.neg.bin").read_bytes()
        sbdr = (out_dir / "layers_0_mlp_up_proj_weight.sbdr").read_bytes()
        sb_pos, sb_neg = unpack_sbdr1(sbdr)
        assert sb_pos == pos and sb_neg == neg
        trits = unpack_dualrail_64(pos, neg, 64 * 128)
        assert len(trits) == 64 * 128 and all(t in (-1, 0, 1) for t in trits)

        # 1-bit tamper check
        tampered = bytearray(pos)
        tampered[0] ^= 1
        assert f"sha3-512:{hashlib.sha3_512(bytes(tampered) + neg).hexdigest()}" != res1["tensors"][0]["sha3_512"]

        print(json.dumps(res1, indent=2))
        return 0


def main() -> int:
    ap = argparse.ArgumentParser(description="Ingest .safetensors into BitNet b1.58 Dual-Rail (P, N)")
    ap.add_argument("--input", type=Path, help="Path to input .safetensors file")
    ap.add_argument("--out-dir", type=Path, help="Optional output directory for Dual-Rail .pos/.neg files")
    ap.add_argument("--emit-sbdr", action="store_true", help="Also emit canonical SBDR1 sparse-block .sbdr files")
    ap.add_argument("--self-test", action="store_true", help="Run built-in safetensors ingest + roundtrip + tamper self-test")
    args = ap.parse_args()
    if args.self_test:
        return run_self_test()
    if not args.input:
        ap.error("provide --input <file.safetensors> or --self-test")
    print(json.dumps(ingest_safetensors(args.input, args.out_dir, emit_sbdr=args.emit_sbdr), indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
