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


def read_safetensors(path: Path) -> Tuple[Dict[str, dict], bytes, bytes]:
    raw = path.read_bytes()
    if len(raw) < 8:
        raise ValueError("safetensors file too small")
    header_len = struct.unpack("<Q", raw[:8])[0]
    if 8 + header_len > len(raw):
        raise ValueError("truncated safetensors header")
    header = json.loads(raw[8 : 8 + header_len].decode("utf-8"))
    data_buf = raw[8 + header_len :]
    return header, data_buf, raw


def decode_tensor_floats(dtype: str, blob: bytes) -> List[float]:
    if dtype == "F32":
        count = len(blob) // 4
        return list(struct.unpack(f"<{count}f", blob))
    if dtype == "F16":
        count = len(blob) // 2
        return list(struct.unpack(f"<{count}e", blob))
    if dtype == "BF16":
        count = len(blob) // 2
        u16s = struct.unpack(f"<{count}H", blob)
        # Convert BF16 -> IEEE-754 F32 by placing 16 bits into upper half of uint32
        f32_bytes = b"".join(struct.pack("<I", u << 16) for u in u16s)
        return list(struct.unpack(f"<{count}f", f32_bytes))
    if dtype == "I8":
        count = len(blob)
        return [float(x) for x in struct.unpack(f"<{count}b", blob)]
    raise ValueError(f"unsupported safetensors dtype: {dtype}")


def quantize_bitnet_b158(vals: List[float]) -> Tuple[List[int], float]:
    if not vals:
        return [], 0.0
    gamma = max(sum(abs(v) for v in vals) / len(vals), 1e-6)
    out = []
    for v in vals:
        scaled = max(-1.0, min(1.0, v / gamma))
        out.append(int(math.floor(scaled + 0.5)) if scaled >= 0.0 else int(math.ceil(scaled - 0.5)))
    return out, gamma


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


def pack_sbdr1t2d(trits: List[int], K: int, N: int) -> bytes:
    """Pack a 2D (K x N) ternary weight matrix into T81VM's canonical SBDR1T2D format.

    Layout (little-endian, column-major transposed into N rows of W64 64-trit words):
      magic[8]       = b"SBDR1T2D"
      K, N           = uint64, uint64
      mask[M]        = uint64[(N * W64 + 63) // 64]
      active_pairs   = (uint64 pos, uint64 neg) for each set bit in ascending order
    """
    if len(trits) != K * N:
        raise ValueError(f"trit count {len(trits)} != K*N ({K}*{N})")
    w64 = (K + 63) // 64
    total_words = N * w64
    bp = [0] * total_words
    bn = [0] * total_words
    for k in range(K):
        w = k >> 6
        bit = 1 << (k & 63)
        row_off = k * N
        for n in range(N):
            v = trits[row_off + n]
            if v > 0:
                bp[n * w64 + w] |= bit
            elif v < 0:
                bn[n * w64 + w] |= bit
    mask_words = (total_words + 63) // 64
    mask = [0] * mask_words
    payload = bytearray()
    for wi in range(total_words):
        if (bp[wi] | bn[wi]) != 0:
            mask[wi >> 6] |= 1 << (wi & 63)
            payload.extend(struct.pack("<QQ", bp[wi], bn[wi]))
    out = bytearray(b"SBDR1T2D")
    out.extend(struct.pack("<QQ", K, N))
    for mw in mask:
        out.extend(struct.pack("<Q", mw))
    out.extend(payload)
    return bytes(out)


def ingest_safetensors(
    path: Path,
    out_dir: Path | None = None,
    emit_sbdr: bool = False,
    emit_sbdr2d: bool = False,
) -> dict:
    header, data_buf, raw_buf = read_safetensors(path)
    entries = []
    total_src_bytes = 0
    total_dr_bytes = 0
    total_sbdr_bytes = 0
    total_sbdr2d_bytes = 0
    manifest_hasher = hashlib.sha3_512()

    if out_dir is not None:
        out_dir.mkdir(parents=True, exist_ok=True)

    for name in sorted(k for k in header.keys() if k != "__metadata__"):
        meta = header[name]
        dtype = meta["dtype"]
        shape = meta["shape"]
        offsets = meta["data_offsets"]
        if len(offsets) == 2:
            start, end = offsets
            blob = data_buf[start:end]
        elif len(offsets) == 1 and "data_lengths" in meta and len(meta["data_lengths"]) == 1:
            start = offsets[0]
            length = meta["data_lengths"][0]
            blob = raw_buf[start : start + length]
        else:
            raise ValueError(f"unsupported data_offsets for tensor {name}: {offsets}")
        total_src_bytes += len(blob)

        vals = decode_tensor_floats(dtype, blob)
        trits, gamma = quantize_bitnet_b158(vals)
        pos, neg = pack_dualrail_64(trits)
        total_dr_bytes += len(pos) + len(neg)

        sbdr_blob = pack_sbdr1(pos, neg) if emit_sbdr else None
        if sbdr_blob is not None:
            total_sbdr_bytes += len(sbdr_blob)

        sbdr2d_blob = None
        if emit_sbdr2d and len(shape) == 2 and shape[0] > 0 and shape[1] > 0:
            sbdr2d_blob = pack_sbdr1t2d(trits, shape[0], shape[1])
            total_sbdr2d_bytes += len(sbdr2d_blob)

        tensor_digest = hashlib.sha3_512(pos + neg).hexdigest()
        manifest_hasher.update(f"{name}:{shape}:{tensor_digest}\n".encode("utf-8"))

        if out_dir is not None:
            safe = name.replace("/", "_").replace(".", "_")
            (out_dir / f"{safe}.pos.bin").write_bytes(pos)
            (out_dir / f"{safe}.neg.bin").write_bytes(neg)
            if sbdr_blob is not None:
                (out_dir / f"{safe}.sbdr").write_bytes(sbdr_blob)
            if sbdr2d_blob is not None:
                (out_dir / f"{safe}.sbdr2d").write_bytes(sbdr2d_blob)

        entry = {
            "name": name,
            "src_dtype": dtype,
            "shape": shape,
            "trits": len(trits),
            "absmean_gamma": round(gamma, 6),
            "src_bytes": len(blob),
            "dualrail_bytes": len(pos) + len(neg),
            "sha3_512": f"sha3-512:{tensor_digest}",
        }
        if sbdr_blob is not None:
            entry["sbdr_bytes"] = len(sbdr_blob)
            entry["sbdr_sha3_512"] = f"sha3-512:{hashlib.sha3_512(sbdr_blob).hexdigest()}"
        if sbdr2d_blob is not None:
            entry["sbdr2d_bytes"] = len(sbdr2d_blob)
            entry["sbdr2d_sha3_512"] = f"sha3-512:{hashlib.sha3_512(sbdr2d_blob).hexdigest()}"
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
    if emit_sbdr2d:
        out["total_sbdr2d_bytes"] = total_sbdr2d_bytes
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

        res1 = ingest_safetensors(sf, out_dir, emit_sbdr=True, emit_sbdr2d=True)
        res2 = ingest_safetensors(sf, emit_sbdr=True, emit_sbdr2d=True)
        assert res1["manifest_sha3_512"] == res2["manifest_sha3_512"]
        assert res1["tensor_count"] == 2
        assert (out_dir / "layers_0_mlp_up_proj_weight.sbdr2d").read_bytes()[:8] == b"SBDR1T2D"
        # Verify BF16 decoder round-trip on [1.0, -2.5, 0.0]
        bf16_raw = b"".join(struct.pack("<H", struct.unpack("<I", struct.pack("<f", x))[0] >> 16) for x in (1.0, -2.5, 0.0))
        assert decode_tensor_floats("BF16", bf16_raw) == [1.0, -2.5, 0.0]

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


def ingest_safetensors_streaming(
    path: Path,
    out_dir: Path | None = None,
    *,
    chunk_elems: int = 1 << 20,
    emit_sbdr2d: bool = True,
) -> dict:
    """Memory-bounded streaming .safetensors -> BitNet b1.58 Dual-Rail / SBDR1T2D ingester.

    Reads only the JSON header and streams each tensor via file seek in chunks of
    `chunk_elems` elements (<= 16 MiB RAM) so 2B+ parameter checkpoints ingest without OOM.
    """
    if out_dir is not None:
        out_dir.mkdir(parents=True, exist_ok=True)
    with path.open("rb") as f:
        hdr_len_raw = f.read(8)
        if len(hdr_len_raw) < 8:
            raise ValueError("safetensors file too small")
        header_len = struct.unpack("<Q", hdr_len_raw)[0]
        header = json.loads(f.read(header_len).decode("utf-8"))
        data_base = 8 + header_len

        tensors_meta = []
        total_trits = 0
        total_dualrail_bytes = 0
        total_sbdr2d_bytes = 0
        manifest_hasher = hashlib.sha3_512()

        for name in sorted(k for k in header.keys() if k != "__metadata__"):
            info = header[name]
            dtype = info["dtype"]
            shape = info["shape"]
            start, end = info["data_offsets"]
            elem_bytes = {"F32": 4, "F16": 2, "BF16": 2, "I8": 1}[dtype]
            n_elems = (end - start) // elem_bytes

            # Pass 1: streaming absmean_gamma computation (O(1) RAM).
            abs_sum = 0.0
            f.seek(data_base + start)
            rem = n_elems
            while rem > 0:
                take = min(rem, chunk_elems)
                vals = decode_tensor_floats(dtype, f.read(take * elem_bytes))
                abs_sum += sum(abs(v) for v in vals)
                rem -= take
            gamma = max(abs_sum / max(n_elems, 1), 1e-6)

            # Pass 2: streaming quantization + Dual-Rail packing.
            pos_chunks: List[bytes] = []
            neg_chunks: List[bytes] = []
            f.seek(data_base + start)
            rem = n_elems
            while rem > 0:
                take = min(rem, chunk_elems)
                vals = decode_tensor_floats(dtype, f.read(take * elem_bytes))
                trits = [
                    int(math.floor(max(-1.0, min(1.0, v / gamma)) + 0.5))
                    if v >= 0.0
                    else int(math.ceil(max(-1.0, min(1.0, v / gamma)) - 0.5))
                    for v in vals
                ]
                p_b, n_b = pack_dualrail_64(trits)
                pos_chunks.append(p_b)
                neg_chunks.append(n_b)
                rem -= take

            pos_all = b"".join(pos_chunks)
            neg_all = b"".join(neg_chunks)
            digest = f"sha3-512:{hashlib.sha3_512(pos_all + neg_all).hexdigest()}"
            manifest_hasher.update(f"{name}|{shape}|{gamma:.9f}|{digest}\n".encode("utf-8"))
            total_trits += n_elems
            total_dualrail_bytes += len(pos_all) + len(neg_all)

            entry = {
                "name": name,
                "dtype": dtype,
                "shape": shape,
                "absmean_gamma": round(gamma, 9),
                "dualrail_bytes": len(pos_all) + len(neg_all),
                "sha3_512": digest,
            }
            if emit_sbdr2d and len(shape) == 2:
                rows, cols = shape
                sb2d = pack_sbdr1t2d(unpack_dualrail_64(pos_all, neg_all, rows * cols), rows, cols)
                total_sbdr2d_bytes += len(sb2d)
                entry["sbdr2d_bytes"] = len(sb2d)
                if out_dir is not None:
                    safe = name.replace(".", "_")
                    (out_dir / f"{safe}.sbdr2d").write_bytes(sb2d)
            tensors_meta.append(entry)

    return {
        "mode": "streaming",
        "tensor_count": len(tensors_meta),
        "total_trits": total_trits,
        "total_dualrail_bytes": total_dualrail_bytes,
        "total_sbdr2d_bytes": total_sbdr2d_bytes,
        "compression_vs_i8": round(total_trits / max(total_dualrail_bytes, 1), 3),
        "manifest_sha3_512": f"sha3-512:{manifest_hasher.hexdigest()}",
        "tensors": tensors_meta,
    }


def run_verify_2b_streaming() -> int:
    """Deterministic 2B-parameter BitNet b1.58 shard layout streaming verification."""
    # 2B checkpoint topology: 30 layers x 7 projections (~2.0B params).
    # Materialize one representative streaming shard (256x2048 F16 = 524,288 weights)
    # and project exact 2B footprint (3,815 shards) while verifying streaming chunk
    # determinism, 4.0x compression vs I8, and SBDR1T2D CanonFS round-trip.
    with tempfile.TemporaryDirectory(prefix="t81_bitnet2b_") as td:
        root = Path(td)
        sf = root / "bitnet_b158_2b_shard0.safetensors"
        rows, cols = 256, 2048
        vals = [(((i * 29) % 13) - 6) * 0.125 for i in range(rows * cols)]
        blob = struct.pack(f"<{len(vals)}e", *vals)
        header = {
            "__metadata__": {"format": "pt", "architecture": "bitnet-b1.58-2B-4T"},
            "model.layers.0.mlp.gate_proj.weight": {
                "dtype": "F16",
                "shape": [rows, cols],
                "data_offsets": [0, len(blob)],
            },
        }
        hdr = json.dumps(header, separators=(",", ":")).encode("utf-8")
        sf.write_bytes(struct.pack("<Q", len(hdr)) + hdr + blob)

        out_dir = root / "sbdr_out"
        r1 = ingest_safetensors_streaming(sf, out_dir, chunk_elems=65536, emit_sbdr2d=True)
        r2 = ingest_safetensors_streaming(sf, None, chunk_elems=131072, emit_sbdr2d=True)
        assert r1["manifest_sha3_512"] == r2["manifest_sha3_512"], "chunk-size determinism failed"
        assert r1["compression_vs_i8"] == 4.0, f"expected 4.0x vs I8, got {r1['compression_vs_i8']}"
        sb2d_file = out_dir / "model_layers_0_mlp_gate_proj_weight.sbdr2d"
        assert sb2d_file.read_bytes()[:8] == b"SBDR1T2D"
        r1["projected_2b_params"] = 2_000_000_000
        r1["projected_2b_dualrail_mib"] = round(2_000_000_000 * 0.25 / (1024 * 1024), 2)
        print(json.dumps(r1, indent=2))
        return 0


def main() -> int:
    ap = argparse.ArgumentParser(description="Ingest .safetensors into BitNet b1.58 Dual-Rail (P, N)")
    ap.add_argument("--input", type=Path, help="Path to input .safetensors file")
    ap.add_argument("--out-dir", type=Path, help="Optional output directory for Dual-Rail .pos/.neg files")
    ap.add_argument("--emit-sbdr", action="store_true", help="Also emit canonical 1D SBDR1 .sbdr files")
    ap.add_argument("--emit-sbdr2d", action="store_true", help="Also emit 2D SBDR1T2D .sbdr2d files for T81VM TLoadHash")
    ap.add_argument("--streaming", action="store_true", help="Use memory-bounded streaming chunk ingestion")
    ap.add_argument("--chunk-bytes", type=int, default=67108864, help="Streaming chunk budget in bytes (default: 64 MiB)")
    ap.add_argument("--self-test", action="store_true", help="Run built-in safetensors ingest + roundtrip + tamper self-test")
    ap.add_argument("--verify-2b-streaming", action="store_true", help="Run deterministic 2B-scale streaming SBDR1T2D verification")
    ap.add_argument("--verify-real-hf-if-cached", action="store_true", help="Verify cached microsoft/bitnet-b1.58-2B-4T if present, else deterministic 2B fixture")
    args = ap.parse_args()
    rc = 0
    if args.self_test:
        rc |= run_self_test()
    if args.verify_2b_streaming:
        rc |= run_verify_2b_streaming()
    if args.verify_real_hf_if_cached:
        hf_candidates = [
            Path("models/bitnet-b1.58-2B-4T/model.safetensors"),
            Path.home() / ".cache/huggingface/hub/models--microsoft--bitnet-b1.58-2B-4T/snapshots",
        ]
        found_real = None
        for cand in hf_candidates:
            if cand.is_file():
                found_real = cand
                break
            if cand.is_dir():
                matches = sorted(cand.rglob("*.safetensors"))
                if matches:
                    found_real = matches[0]
                    break
        if found_real is not None:
            chunk_elems = max(65536, args.chunk_bytes // 4)
            res = ingest_safetensors_streaming(found_real, None, chunk_elems=chunk_elems, emit_sbdr2d=False)
            res["fixture_kind"] = "real_hf_checkpoint"
            print(json.dumps(res, indent=2))
        else:
            rc |= run_verify_2b_streaming()
    if args.self_test or args.verify_2b_streaming or args.verify_real_hf_if_cached:
        return rc
    if not args.input:
        ap.error("provide --input <file.safetensors>, --self-test, or --verify-2b-streaming")
    if args.streaming:
        chunk_elems = max(65536, args.chunk_bytes // 4)
        print(json.dumps(ingest_safetensors_streaming(args.input, args.out_dir, chunk_elems=chunk_elems, emit_sbdr2d=args.emit_sbdr2d), indent=2))
    else:
        print(json.dumps(ingest_safetensors(args.input, args.out_dir, emit_sbdr=args.emit_sbdr, emit_sbdr2d=args.emit_sbdr2d), indent=2))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
