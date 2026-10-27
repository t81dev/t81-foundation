#!/usr/bin/env python3
"""
Conventional Baseline Stack for Investigation 08 (Equivalent Conventional Comparator).
Provides matched capabilities for comparison with T81:
1. Content-Addressed Storage (CAS) using SHA-3/256 hashing over standard OS filesystem.
2. Axion APL Policy Authorization Engine with exact rule matching & single-hash authorization semantics.
3. Pure Python / Conventional T81W2 binary weight parsing and model checksum validation.
4. Conventional FP32/INT8 matrix/tensor execution kernel matching 2-layer attention slice.
5. Structured trace and audit log generation matching T81 artifact schemas.
6. Verification and tamper rejection mechanics separating lookup from re-execution.
"""

import hashlib
import json
import os
import re
import struct
import time
from typing import Dict, Any, Tuple, Optional, List

BASE81_ALPHABET = [
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    '-', '−', '×', '÷', '=', '<', '>', '≤', '≥', '≠', '≈', '∞', 'λ', 'μ', 'π', 'σ', 'τ', 'ω', 'Γ'
]

def encode_base81(data: bytes) -> str:
    """Matches T81 base81::encode_bytes Base-81 encoding."""
    if not data:
        return ""
    buf = list(data)
    digits = []
    while buf:
        next_buf = []
        carry = 0
        for b in buf:
            cur = (carry << 8) | b
            q = cur // 81
            r = cur % 81
            if next_buf or q != 0:
                next_buf.append(q)
            carry = r
        digits.append(carry)
        buf = next_buf
    return "".join(BASE81_ALPHABET[d] for d in reversed(digits))


def canon_sha3_256_ref(text: str) -> str:
    """Matches T81 canon_sha3_256_ref: sha3-512 truncated to 32 bytes and encoded in Base-81."""
    digest32 = hashlib.sha3_512(text.encode('utf-8')).digest()[:32]
    return "sha3-256:" + encode_base81(digest32)


class T81W2Model:
    """Parser and container for T81W2 native binary model weights."""
    def __init__(self, raw_bytes: bytes):
        self.raw_bytes = raw_bytes
        self.file_checksum: str = hashlib.sha3_512(raw_bytes).hexdigest()
        self.payload_checksum: str = ""
        self.format_magic: str = ""
        self.tensors: Dict[str, Dict[str, Any]] = {}
        self._parse()

    def _parse(self):
        data = self.raw_bytes
        if not data:
            raise ValueError("Empty model data")

        # Find first newline (magic)
        first_nl = data.find(b'\n')
        if first_nl == -1:
            raise ValueError("Invalid t81w file: missing magic newline")
        self.format_magic = data[:first_nl].decode('ascii', errors='replace')
        if self.format_magic not in ("T81W1", "T81W2"):
            raise ValueError(f"Invalid t81w magic header: {self.format_magic}")

        # Find second newline (checksum)
        checksum_start = first_nl + 1
        second_nl = data.find(b'\n', checksum_start)
        if second_nl == -1:
            raise ValueError("t81w: missing or malformed checksum header")
        checksum_str = data[checksum_start:second_nl].decode('ascii', errors='replace')
        if len(checksum_str) != 128:
            raise ValueError("t81w: checksum string must be 128 hex characters")

        payload = data[second_nl + 1:]
        # Verify SHA3-512 payload checksum against header
        computed_checksum = hashlib.sha3_512(payload).hexdigest()
        if computed_checksum != checksum_str:
            raise ValueError(f"t81w payload checksum mismatch: computed {computed_checksum} != header {checksum_str}")
        self.payload_checksum = computed_checksum

        # Parse tensors from binary payload
        cursor = 0
        def read_le64() -> int:
            nonlocal cursor
            if cursor + 8 > len(payload):
                raise ValueError("t81w: truncated metadata")
            val = struct.unpack('<Q', payload[cursor:cursor+8])[0]
            cursor += 8
            return val

        num_tensors = read_le64()
        for _ in range(num_tensors):
            name_len = read_le64()
            if cursor + name_len > len(payload):
                raise ValueError("t81w: truncated tensor name")
            name = payload[cursor:cursor+name_len].decode('utf-8', errors='replace')
            cursor += name_len

            rank = read_le64()
            shape = [read_le64() for _ in range(rank)]

            fmt = 0
            if self.format_magic == "T81W2":
                if cursor >= len(payload):
                    raise ValueError("t81w: truncated format byte")
                fmt = payload[cursor]
                cursor += 1

            trits = read_le64()
            bytes_len = read_le64()
            if cursor + bytes_len > len(payload):
                raise ValueError("t81w: truncated tensor data")
            tensor_data_bytes = payload[cursor:cursor+bytes_len]
            cursor += bytes_len

            self.tensors[name] = {
                "name": name,
                "shape": shape,
                "format": fmt,
                "trits": trits,
                "data_bytes": tensor_data_bytes
            }

    @staticmethod
    def unpack_trits(tensor_dict: Dict[str, Any]) -> List[int]:
        """Unpack balanced ternary trits from 64-bit limbs into list of int8 trits (-1, 0, 1)."""
        data_bytes = tensor_dict["data_bytes"]
        total_trits = tensor_dict["trits"]
        out = []
        trit_map = [-1, 0, 1]

        # Read 64-bit LE limbs
        num_limbs = len(data_bytes) // 8
        limbs = struct.unpack(f'<{num_limbs}Q', data_bytes[:num_limbs*8])

        for limb in limbs:
            decoded = [0] * 48
            l_val = limb
            for i in range(47, -1, -1):
                decoded[i] = trit_map[l_val % 3]
                l_val //= 3
            for val in decoded:
                if len(out) < total_trits:
                    out.append(val)
                else:
                    break
        return out


class ConventionalCAS:
    """Conventional Content-Addressed Storage backed by standard OS filesystem."""
    def __init__(self, root_dir: str):
        self.root_dir = root_dir
        self.objects_dir = os.path.join(root_dir, "objects")
        os.makedirs(self.objects_dir, exist_ok=True)

    def put(self, data: bytes) -> str:
        ref = canon_sha3_256_ref(data.decode('utf-8', errors='replace')) if isinstance(data, (bytes, bytearray)) else canon_sha3_256_ref(data)
        digest = ref.split(":", 1)[1]
        obj_path = os.path.join(self.objects_dir, digest)
        if not os.path.exists(obj_path):
            with open(obj_path, "wb") as f:
                f.write(data)
        return ref

    def get(self, ref: str) -> Optional[bytes]:
        if not ref.startswith("sha3-256:"):
            return None
        digest = ref.split(":", 1)[1]
        obj_path = os.path.join(self.objects_dir, digest)
        if not os.path.exists(obj_path):
            return None
        with open(obj_path, "rb") as f:
            data = f.read()
        # Verify content hash on read
        actual_ref = canon_sha3_256_ref(data.decode('utf-8', errors='replace'))
        if actual_ref != ref:
            raise ValueError("CAS corruption / tamper detected on read")
        return data


class ConventionalPolicyEngine:
    """Axion APL Policy Engine parser and authorization evaluator."""
    def __init__(self, policy_input: Any):
        self.raw_text = ""
        self.allowed_model_hashes: List[str] = []
        self.required_axion_events: List[str] = []

        if isinstance(policy_input, dict):
            self.allowed_model_hashes = policy_input.get("allowed_model_hashes", policy_input.get("allowed_ternary_model_hashes", []))
            self.required_axion_events = policy_input.get("require_axion_events", policy_input.get("required_axion_events", []))
            self.raw_text = json.dumps(policy_input)
        elif isinstance(policy_input, str):
            self.raw_text = policy_input
            self._parse_apl(policy_input)
        else:
            raise TypeError("Policy input must be str (APL text) or dict")

    def _parse_apl(self, apl_text: str):
        # Parse allowed-ternary-model-hashes
        match = re.search(r'\(allowed-ternary-model-hashes\s*\[(.*?)\]\)', apl_text, re.DOTALL)
        if match:
            raw_list = match.group(1)
            hashes = re.findall(r'"([^"]+)"', raw_list)
            self.allowed_model_hashes = hashes

        # Parse require-axion-event (reason "...")
        events = re.findall(r'\(require-axion-event\s*\(reason\s*"([^"]+)"\)\)', apl_text)
        self.required_axion_events = events

    def evaluate(self, model_hash: str, required_task_marker: str = "task:assess_fixed.v1") -> Tuple[bool, str]:
        """
        Exact Axion authorization evaluation matching T81 policy_allows_single_model_hash
        and policy_requires_task_marker.
        """
        if len(self.allowed_model_hashes) != 1 or self.allowed_model_hashes[0] != model_hash:
            return False, f"policy must admit exactly one allowed-ternary-model-hashes entry matching model checksum={model_hash}"

        if required_task_marker not in self.required_axion_events:
            return False, f"policy is missing require-axion-event reason \"{required_task_marker}\""

        return True, "allow"


def normalize_task_input_text(text: str) -> str:
    """Matches T81 normalize_task_input_text string normalization semantics."""
    normalized = []
    i = 0
    n = len(text)
    while i < n:
        ch = text[i]
        if ch == '\r':
            if i + 1 < n and text[i + 1] == '\n':
                i += 1
            normalized.append('\n')
        else:
            normalized.append(ch)
        i += 1

    res = "".join(normalized)
    while res and res[-1] in ('\n', '\r'):
        res = res[:-1]
    return res


# Vocabulary mapping matching kAssessFixedVocabulary / T81 task specification
VOCABULARY_MAP = {
    42: ("ALLOW", "GREETING_PAIR"),
    9: ("DENY", "WORLD_SEQUENCE"),
    7: ("REVIEW", "PREFIX_ONLY"),
}


def history_seed_window_start(token_history: List[int], vocab_size: int) -> int:
    """Matches T81 history_seed_window_start calculation for candidate sampling window."""
    if vocab_size == 0:
        return 0
    seed_text = ",".join(str(t) for t in token_history)
    digest = hashlib.sha3_512(seed_text.encode('utf-8')).hexdigest()
    seed = int(digest[:16], 16)
    return seed % vocab_size


def select_prompt_seeded_token_ids(prompt: str, vocab_size: int, sample_window: int) -> Tuple[int, List[int]]:
    """Matches T81 select_prompt_seeded_token_ids when tokenizer produces no tokens."""
    text = prompt if prompt else "deterministic prompt"
    digest = hashlib.sha3_512(text.encode('utf-8')).hexdigest()
    seed = int(digest[:16], 16)
    window_start = seed % vocab_size
    sampled = [(window_start + i) % vocab_size for i in range(sample_window)]
    return window_start, sampled


class ConventionalTaskRunner:
    """
    Conventional task runner performing independently-implemented, computationally equivalent
    execution to T81's assess-fixed task.
    """
    def __init__(self, cas_root: str):
        self.cas = ConventionalCAS(cas_root)

    def _tokenize(self, prompt: str, tokenizer_data: Optional[Dict[str, Any]]) -> List[int]:
        """Independent tokenizer matching T81 BPE / sentencepiece fallback lookup."""
        if not tokenizer_data or "model" not in tokenizer_data or "vocab" not in tokenizer_data["model"]:
            return []

        vocab = tokenizer_data["model"]["vocab"]
        words = prompt.strip().split()
        matched_tokens = []

        for word in words:
            if word in vocab:
                matched_tokens.append(vocab[word])
            elif f"▁{word}" in vocab:
                matched_tokens.append(vocab[f"▁{word}"])

        if matched_tokens:
            return matched_tokens

        # Fallback direct prompt match
        if prompt in vocab:
            return [vocab[prompt]]
        if f"▁{prompt}" in vocab:
            return [vocab[f"▁{prompt}"]]

        return []

    def run_assess_fixed(
        self,
        model_bytes: bytes,
        input_text: str,
        policy_input: Any,
        tokenizer_data: Optional[Dict[str, Any]] = None,
        mode: str = "strict_deterministic"
    ) -> Dict[str, Any]:
        start_time = time.perf_counter_ns()

        # Mode check matching T81 CLI contract
        if mode != "strict_deterministic":
            raise ValueError("error: assess-fixed requires --mode strict_deterministic")

        # 1. Parse T81W2 model and verify checksum
        try:
            model = T81W2Model(model_bytes)
        except Exception as e:
            raise ValueError(f"Failed to load model: {e}")

        # Check model architecture (llama-dense-v1 requirement)
        required_tensors = [
            "model.embed_tokens.weight",
            "model.layers.0.self_attn.q_proj.weight",
            "model.layers.0.self_attn.k_proj.weight",
            "model.layers.0.self_attn.v_proj.weight",
            "model.layers.1.self_attn.q_proj.weight",
            "model.layers.1.self_attn.k_proj.weight",
            "model.layers.1.self_attn.v_proj.weight",
            "lm_head.weight"
        ]
        for req_t in required_tensors:
            if req_t not in model.tensors:
                raise ValueError("error: assess-fixed supports only llama-dense-v1 narrow greedy decode")

        lm_head_tensor = model.tensors["lm_head.weight"]
        if len(lm_head_tensor["shape"]) != 2 or lm_head_tensor["shape"][1] != 16:
            raise ValueError("error: assess-fixed supports only llama-dense-v1 narrow greedy decode")

        model_hash = f"sha3-512:{model.file_checksum}"
        model_ref = self.cas.put(model_bytes)

        # 2. Authorization check
        policy_engine = ConventionalPolicyEngine(policy_input)
        authorized, reason = policy_engine.evaluate(model_hash, "task:assess_fixed.v1")
        if not authorized:
            raise PermissionError(f"SecurityFault: assess-fixed denied ({reason})")

        # 3. Text normalization and Tokenization
        normalized_input = normalize_task_input_text(input_text)
        input_hash = canon_sha3_256_ref(normalized_input)

        if isinstance(policy_input, str):
            policy_raw_text = policy_input
        else:
            policy_raw_text = json.dumps(policy_input, sort_keys=True)
        policy_hash = canon_sha3_256_ref(policy_raw_text)

        prompt_tokens = self._tokenize(normalized_input, tokenizer_data)

        # 4. Model execution: 2-layer attention slice with ternary snapping and integer accumulation
        embed_t = model.tensors["model.embed_tokens.weight"]
        embed_trits = T81W2Model.unpack_trits(embed_t)
        hidden_dim = embed_t["shape"][1] # 16
        vocab_size = lm_head_tensor["shape"][0] # 64
        sample_window_size = 8

        if prompt_tokens:
            seed_token_id = prompt_tokens[-1]
            emb = [embed_trits[seed_token_id * hidden_dim + i] for i in range(hidden_dim)]
            context_tokens = prompt_tokens[:-1]
            for ctx_tok in context_tokens:
                ctx_piece = [embed_trits[ctx_tok * hidden_dim + i] for i in range(hidden_dim)]
                emb = [emb[i] + ctx_piece[i] for i in range(hidden_dim)]

            if len(prompt_tokens) > 1:
                window_start = history_seed_window_start(prompt_tokens, vocab_size)
                candidate_selection_mode = "tokenizer_prompt_history"
                candidate_selection_basis = "tokenizer_prompt_history_contiguous_window.v1"
            else:
                window_start = seed_token_id % vocab_size
                candidate_selection_mode = "tokenizer_prompt_token"
                candidate_selection_basis = "tokenizer_prompt_token_contiguous_window.v1"
            sampled_token_ids = [(window_start + i) % vocab_size for i in range(sample_window_size)]
        else:
            candidate_selection_mode = "prompt_seeded"
            candidate_selection_basis = "prompt_sha3_seeded_contiguous_window.v1"
            window_start, sampled_token_ids = select_prompt_seeded_token_ids(normalized_input, vocab_size, sample_window_size)
            seed_token_id = sampled_token_ids[0]
            emb = [embed_trits[seed_token_id * hidden_dim + i] for i in range(hidden_dim)]

        # Layer 0 Attention matmuls (q0, k0, v0)
        q0_trits = T81W2Model.unpack_trits(model.tensors["model.layers.0.self_attn.q_proj.weight"])
        k0_trits = T81W2Model.unpack_trits(model.tensors["model.layers.0.self_attn.k_proj.weight"])
        v0_trits = T81W2Model.unpack_trits(model.tensors["model.layers.0.self_attn.v_proj.weight"])

        q0 = [sum(emb[i] * q0_trits[j * hidden_dim + i] for i in range(hidden_dim)) for j in range(hidden_dim)]
        k0 = [sum(emb[i] * k0_trits[j * hidden_dim + i] for i in range(hidden_dim)) for j in range(hidden_dim)]
        v0 = [sum(emb[i] * v0_trits[j * hidden_dim + i] for i in range(hidden_dim)) for j in range(hidden_dim)]
        attn0 = v0 # 1D vector attention reduction returns v0

        # Layer 1 Attention matmuls (q1, k1, v1)
        q1_trits = T81W2Model.unpack_trits(model.tensors["model.layers.1.self_attn.q_proj.weight"])
        k1_trits = T81W2Model.unpack_trits(model.tensors["model.layers.1.self_attn.k_proj.weight"])
        v1_trits = T81W2Model.unpack_trits(model.tensors["model.layers.1.self_attn.v_proj.weight"])

        q1 = [sum(attn0[i] * q1_trits[j * hidden_dim + i] for i in range(hidden_dim)) for j in range(hidden_dim)]
        k1 = [sum(attn0[i] * k1_trits[j * hidden_dim + i] for i in range(hidden_dim)) for j in range(hidden_dim)]
        v1 = [sum(attn0[i] * v1_trits[j * hidden_dim + i] for i in range(hidden_dim)) for j in range(hidden_dim)]
        attn1 = v1

        # lm_head logit scores reduction for sampled candidate tokens
        lm_head_trits = T81W2Model.unpack_trits(lm_head_tensor)

        candidate_scores = []
        for tok_id in sampled_token_ids:
            row_trits = lm_head_trits[tok_id * hidden_dim : (tok_id + 1) * hidden_dim]
            score = float(sum(row_trits[i] * attn1[i] for i in range(hidden_dim)))
            candidate_scores.append((score, tok_id))

        # Selection policy: greedy argmax with lowest token ID tie-breaking
        best = max(candidate_scores, key=lambda x: (x[0], -x[1]))
        selected_score, selected_token_id = best

        # Decision mapping
        decision, reason_code = VOCABULARY_MAP.get(selected_token_id, ("UNKNOWN", "UNMAPPED_TOKEN"))

        # 5. Result Artifact & Provenance Artifact creation matching T81 schema exact format
        result_artifact_str = json.dumps({
            "schema": "t81.ai.task.assess-fixed.v1",
            "task": "assess_fixed",
            "input_hash": input_hash,
            "model_hash": model_hash,
            "decision": decision,
            "reason_code": reason_code,
            "decision_token_ids": [selected_token_id],
            "termination_reason": "single_step_max_score"
        }, indent=2) + "\n"
        result_bytes = result_artifact_str.encode('utf-8')
        result_ref = self.cas.put(result_bytes)

        provenance_artifact_str = json.dumps({
            "schema": "t81.ai.task-run.assess-fixed.provenance.v1",
            "task": "assess_fixed",
            "result_ref": result_ref,
            "input_hash": input_hash,
            "model_hash": model_hash,
            "policy_result": "allowed",
            "policy_hash": policy_hash,
            "determinism_class": "strict_deterministic",
            "backend": "conventional_baseline_comparator",
            "termination_reason": "single_step_max_score",
            "execution_evidence": {
                "input_normalized_sha3_256": input_hash,
                "input_preview": normalized_input,
                "step": 0,
                "transition_kind": "prompt_seed_to_bounded_decode_state.v1",
                "decode_mode_kind": "initial_probe_decode_mode.v1",
                "selection_policy_kind": "greedy_argmax_lowest_token_id.v1",
                "candidate_selection_mode": candidate_selection_mode,
                "candidate_selection_basis": candidate_selection_basis,
                "selected_token_id": selected_token_id,
                "sampled_token_ids": sampled_token_ids,
                "sampled_token_scores": [s[0] for s in candidate_scores],
                "prompt_token_history": prompt_tokens,
                "generated_token_history": [selected_token_id]
            }
        }, indent=2) + "\n"
        provenance_bytes = provenance_artifact_str.encode('utf-8')
        provenance_ref = self.cas.put(provenance_bytes)

        # Bundle Creation
        bundle_artifact_str = json.dumps({
            "schema": "t81.ai.task.assess-fixed.bundle.v1",
            "task": "assess_fixed",
            "source_result_ref": result_ref,
            "source_provenance_ref": provenance_ref,
            "model_hash": model_hash,
            "input_hash": input_hash,
            "decision": decision,
            "reason_code": reason_code,
            "decision_token_ids": [selected_token_id]
        }, indent=2) + "\n"
        bundle_bytes = bundle_artifact_str.encode('utf-8')
        bundle_ref = self.cas.put(bundle_bytes)

        end_time = time.perf_counter_ns()

        return {
            "schema": "t81.ai.task-run.assess-fixed.v1",
            "task_kind": "assess_fixed.v1",
            "model_hash": model_hash,
            "requested_mode": "strict_deterministic",
            "selected_backend": "conventional_baseline_comparator",
            "output_preview": f"{decision}:{reason_code}",
            "result_ref": result_ref,
            "provenance_ref": provenance_ref,
            "bundle_ref": bundle_ref,
            "policy_result": "allowed",
            "policy_hash": policy_hash,
            "input_hash": input_hash,
            "decision": decision,
            "reason_code": reason_code,
            "decision_token_ids": [selected_token_id],
            "selected_token_score": selected_score,
            "candidate_scores": candidate_scores,
            "status": "pass",
            "latency_ns": end_time - start_time
        }

    def verify_and_replay(self, bundle_ref: str, policy_input: Any, tokenizer_data: Optional[Dict[str, Any]] = None, model_bytes_override: Optional[bytes] = None) -> Dict[str, Any]:
        """
        Distinguishes artifact retrieval & integrity checking from re-execution replay.
        """
        t_lookup_0 = time.perf_counter_ns()
        bundle_bytes = self.cas.get(bundle_ref)
        if not bundle_bytes:
            return {"valid": False, "lookup_latency_ns": 0, "replay_latency_ns": 0}
        bundle = json.loads(bundle_bytes.decode('utf-8'))

        result_bytes = self.cas.get(bundle["source_result_ref"])
        provenance_bytes = self.cas.get(bundle["source_provenance_ref"])
        if not result_bytes or not provenance_bytes:
            return {"valid": False, "lookup_latency_ns": 0, "replay_latency_ns": 0}

        result = json.loads(result_bytes.decode('utf-8'))
        provenance = json.loads(provenance_bytes.decode('utf-8'))

        model_bytes = model_bytes_override
        if not model_bytes:
            raw_hash = bundle['model_hash'].split(':', 1)[1]
            model_bytes = self.cas.get(f"sha3-256:{raw_hash}")
        if not model_bytes:
            return {"valid": False, "lookup_latency_ns": 0, "replay_latency_ns": 0}
        t_lookup_1 = time.perf_counter_ns()

        t_replay_0 = time.perf_counter_ns()
        input_text = provenance["execution_evidence"]["input_preview"]
        replayed = self.run_assess_fixed(
            model_bytes=model_bytes,
            input_text=input_text,
            policy_input=policy_input,
            tokenizer_data=tokenizer_data,
            mode="strict_deterministic"
        )
        t_replay_1 = time.perf_counter_ns()

        valid = (
            replayed["decision"] == result["decision"] and
            replayed["reason_code"] == result["reason_code"] and
            replayed["decision_token_ids"] == result["decision_token_ids"]
        )
        return {
            "valid": valid,
            "lookup_latency_ns": t_lookup_1 - t_lookup_0,
            "replay_latency_ns": t_replay_1 - t_replay_0
        }
