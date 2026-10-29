#!/usr/bin/env python3
"""
Conventional Baseline Stack for Investigation 08 (Equivalent Conventional Comparator).
Provides a bounded, independently implemented comparator for the tested T81 task:
1. CAS using the reference truncated SHA3-512/Base-81 object identity.
2. Axion APL Policy Authorization Engine with exact rule matching & single-hash authorization semantics.
3. Pure Python / Conventional T81W2 binary weight parsing and model checksum validation.
4. Pure-Python integer computation for a bounded two-layer attention slice.
5. Exact task-result serialization plus comparator-specific recovery evidence.
6. Verification and tamper rejection mechanics separating lookup from re-execution.
"""

import hashlib
import json
import os
import re
import struct
import time
from typing import Dict, Any, Tuple, Optional, List, Set

MANDATORY_TENSORS: Set[str] = {
    "model.embed_tokens.weight",
    "model.norm.weight",
    "model.layers.0.self_attn.q_proj.weight",
    "model.layers.0.self_attn.k_proj.weight",
    "model.layers.0.self_attn.v_proj.weight",
    "model.layers.0.self_attn.o_proj.weight",
    "model.layers.0.mlp.gate_proj.weight",
    "model.layers.0.mlp.up_proj.weight",
    "model.layers.0.mlp.down_proj.weight",
    "model.layers.1.self_attn.q_proj.weight",
    "model.layers.1.self_attn.k_proj.weight",
    "model.layers.1.self_attn.v_proj.weight",
    "lm_head.weight",
}


def validate_model_architecture(model_bytes: bytes) -> Set[str]:
    """
    Parses .t81w binary payload, verifies header checksum,
    and validates that all mandatory architecture admission tensors are present.
    Returns the set of tensor names.
    Raises ValueError if format/checksum is invalid or any mandatory tensor is missing.
    """
    model = T81W2Model(model_bytes)
    return set(model.tensors.keys())


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


def canon_sha3_256_ref(text: str | bytes) -> str:
    """Matches T81 canon_sha3_256_ref: sha3-512 truncated to 32 bytes and encoded in Base-81."""
    digest32 = hashlib.sha3_512(text.encode('utf-8') if isinstance(text, str) else text).digest()[:32]
    return "sha3-256:" + encode_base81(digest32)


def render_result(data):
    return "{\n" + ",\n".join("  " + json.dumps(k) + ": " + json.dumps(v, ensure_ascii=False)
                                  for k, v in data.items()) + "\n}\n"

def snap_trit(value):
    return 1 if value > 0.5 else (-1 if value < -0.5 else 0)

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
            bytes_len = read_le64() if self.format_magic == "T81W2" else ((trits + 47) // 48) * 8
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

        missing = MANDATORY_TENSORS - set(self.tensors.keys())
        if missing:
            raise ValueError(f"Model architecture admission rejected: missing mandatory tensor(s) {sorted(missing)}")

    @staticmethod
    def unpack_trits(tensor_dict: Dict[str, Any]) -> List[int]:
        """Unpack balanced ternary trits from 64-bit limbs into list of int8 trits (-1, 0, 1)."""
        data_bytes = tensor_dict["data_bytes"]
        total_trits = tensor_dict["trits"]
        if tensor_dict["format"] != 0 or len(data_bytes) % 8 or len(data_bytes) < ((total_trits + 47) // 48) * 8:
            raise ValueError("unsupported or truncated balanced-ternary tensor")
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
        ref = canon_sha3_256_ref(bytes(data))
        digest = ref.split(":", 1)[1]
        if not digest or any(ch not in BASE81_ALPHABET for ch in digest):
            raise ValueError("invalid CAS reference")
        obj_path = os.path.join(self.objects_dir, digest)
        if os.path.exists(obj_path):
            if self.get(ref) != data:
                raise ValueError("existing CAS object differs")
        else:
            with open(obj_path, "xb") as f:
                f.write(data)
        return ref

    def get(self, ref: str) -> Optional[bytes]:
        if not ref.startswith("sha3-256:"):
            return None
        digest = ref.split(":", 1)[1]
        if not digest or any(ch not in BASE81_ALPHABET for ch in digest):
            raise ValueError("invalid CAS reference")
        obj_path = os.path.join(self.objects_dir, digest)
        if not os.path.exists(obj_path):
            return None
        with open(obj_path, "rb") as f:
            data = f.read()
        # Verify content hash on read
        actual_ref = canon_sha3_256_ref(data)
        if actual_ref != ref:
            raise ValueError("CAS corruption / tamper detected on read")
        return data


class ConventionalPolicyEngine:
    """Axion APL Policy Engine parser and authorization evaluator."""
    def __init__(self, policy_input: Any):
        self.raw_text = ""
        self.allowed_model_hashes: List[str] = []
        self.required_axion_events: List[str] = []

        if not isinstance(policy_input, str):
            raise TypeError("bounded comparator requires original APL text")
        self.raw_text = policy_input
        self._parse_apl(policy_input)

    def _parse_apl(self, apl_text: str):
        # Deliberately bounded independent parser. Unsupported clauses fail explicitly;
        # this is not an implementation of all Axion policy semantics.
        tokens = re.findall(r'"(?:\\.|[^"\\])*"|[()\[\]]|[^\s()\[\]]+', apl_text)
        cursor = 0
        class Symbol(str):
            pass
        def parse():
            nonlocal cursor
            if cursor >= len(tokens):
                raise ValueError("unterminated APL")
            token = tokens[cursor]
            cursor += 1
            if token in ("(", "["):
                end = ")" if token == "(" else "]"
                values = []
                while cursor < len(tokens) and tokens[cursor] != end:
                    values.append(parse())
                if cursor == len(tokens):
                    raise ValueError("unterminated APL list")
                cursor += 1
                return values
            if token in (")", "]"):
                raise ValueError("unexpected APL delimiter")
            return json.loads(token) if token.startswith('"') else Symbol(token)
        tree = parse()
        if cursor != len(tokens) or not isinstance(tree, list) or not tree or tree[0] != "policy":
            raise ValueError("expected one policy expression")
        for clause in tree[1:]:
            if not isinstance(clause, list) or not clause:
                raise ValueError("invalid APL clause")
            if clause[0] == "tier" and len(clause) == 2 and str(clause[1]).isdigit():
                continue
            if clause[0] == "allowed-ternary-model-hashes" and len(clause) == 2 and isinstance(clause[1], list):
                if any(type(value) is not str for value in clause[1]):
                    raise ValueError("model hashes require quoted strings")
                self.allowed_model_hashes.extend(clause[1])
            elif clause[0] == "require-axion-event" and len(clause) == 2 and isinstance(clause[1], list) and len(clause[1]) == 2 and clause[1][0] == "reason":
                if type(clause[1][1]) is not str:
                    raise ValueError("reason requires quoted string")
                self.required_axion_events.append(clause[1][1])
            else:
                raise ValueError("unsupported APL clause: " + str(clause[0]))

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
    Pure-Python implementation of the supported assess-fixed slice.
    Equivalence is tested on the declared corpus, not assumed for arbitrary models/policies.
    """
    def __init__(self, cas_root: str):
        self.cas = ConventionalCAS(cas_root)

    def _tokenize(self, prompt: str, tokenizer_data: Optional[Dict[str, Any] | str]) -> List[int]:
        """Independent tokenizer matching T81 BPE / sentencepiece fallback lookup."""
        if isinstance(tokenizer_data, str):
            # The reference's small tokenizer reader removes a backslash and keeps
            # the next character; it does not decode JSON Unicode escapes. Preserve
            # lexical input so escaped and literal vocab/merge spellings match it.
            raw = re.sub(r'"((?:\\.|[^"\\])*)"',
                         lambda m: json.dumps(re.sub(r"\\(.)", r"\1", m.group(1)), ensure_ascii=False),
                         tokenizer_data)
            tokenizer_data = json.loads(raw)
        if not tokenizer_data or "model" not in tokenizer_data or "vocab" not in tokenizer_data["model"]:
            return []

        vocab = tokenizer_data["model"]["vocab"]
        ranks = {pair: index for index, pair in enumerate(tokenizer_data["model"].get("merges", []))}
        normalized = "▁" + prompt.replace(" ", "▁")
        matched = []
        # Mirror reference split_normalized_prompt_words (whitespace only).
        for word in filter(None, re.split(r"(?=▁)", normalized)):
            pieces = list(word)
            while len(pieces) >= 2 and ranks:
                pairs = [(ranks[pieces[i]+" "+pieces[i+1]], i) for i in range(len(pieces)-1)
                         if pieces[i]+" "+pieces[i+1] in ranks]
                if not pairs:
                    break
                _, index = min(pairs)
                lhs, rhs = pieces[index:index+2]
                merged = []
                i = 0
                while i < len(pieces):
                    if i+1 < len(pieces) and pieces[i:i+2] == [lhs, rhs]:
                        merged.append(lhs+rhs); i += 2
                    else:
                        merged.append(pieces[i]); i += 1
                pieces = merged
            matched.extend(vocab[piece] for piece in pieces if piece in vocab)
        if matched:
            return matched
        if prompt in vocab:
            return [vocab[prompt]]
        if "▁"+prompt in vocab:
            return [vocab["▁"+prompt]]
        for word in filter(None, re.split(r'[ \t\r\n\v\f]+', prompt)):
            if word in vocab:
                matched.append(vocab[word])
            elif "▁"+word in vocab:
                matched.append(vocab["▁"+word])
        return matched

    def run_assess_fixed(
        self,
        model_bytes: bytes,
        input_text: str,
        policy_input: Any,
        tokenizer_data: Optional[Dict[str, Any] | str] = None,
        mode: str = "strict_deterministic"
    ) -> Dict[str, Any]:
        start_time = time.perf_counter_ns()
        timings = {}
        policy_start = time.perf_counter_ns()
        if not input_text:
            raise ValueError("assess-fixed requires exactly one nonempty input")

        # Mode check matching T81 CLI contract
        if mode != "strict_deterministic":
            raise ValueError("error: assess-fixed requires --mode strict_deterministic")

        model_hash = "sha3-512:" + hashlib.sha3_512(model_bytes).hexdigest()
        policy_engine = ConventionalPolicyEngine(policy_input)
        authorized, reason = policy_engine.evaluate(model_hash, "task:assess_fixed.v1")
        if not authorized:
            raise PermissionError(f"SecurityFault: assess-fixed denied ({reason})")
        timings["policy_evaluation_ns"] = time.perf_counter_ns() - policy_start
        load_start = time.perf_counter_ns()
        # Parse T81W2 model and verify checksum
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

        timings["model_loading_ns"] = time.perf_counter_ns() - load_start
        compute_start = time.perf_counter_ns()
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
        hidden_dim = embed_t["shape"][1] # bounded reference task width
        if hidden_dim != 16 or embed_t["shape"][0] <= 0:
            raise ValueError("unsupported embedding shape")
        for name in required_tensors[1:-1]:
            if model.tensors[name]["shape"] != [16, 16]:
                raise ValueError("unsupported projection shape")
        for tensor in model.tensors.values():
            if tensor["format"] != 0:
                raise ValueError("unsupported model tensor format")
        vocab_size = lm_head_tensor["shape"][0] # 64
        sample_window_size = min(8, vocab_size)
        if vocab_size <= 0:
            raise ValueError("empty output vocabulary")

        if prompt_tokens:
            seed_token_id = prompt_tokens[-1]
            if any(t < 0 or t >= embed_t["shape"][0] for t in prompt_tokens):
                raise ValueError("embedding token out of range")
            emb = [embed_trits[seed_token_id * hidden_dim + i] for i in range(hidden_dim)]
            context_tokens = prompt_tokens[:-1]
            if len(context_tokens) > 3:
                anchor = prompt_tokens[0]
                recent = [anchor] + [t for t in context_tokens[-2:] if t != anchor]
                for token in context_tokens[-3:]:
                    if len(recent) < 3 and token not in recent:
                        recent.append(token)
                context_tokens = recent
            if any(t < 0 or t >= embed_t["shape"][0] for t in prompt_tokens):
                raise ValueError("embedding token out of range")
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

        q0 = [sum(snap_trit(emb[i]) * q0_trits[i * hidden_dim + j] for i in range(hidden_dim)) for j in range(hidden_dim)]
        k0 = [sum(snap_trit(emb[i]) * k0_trits[i * hidden_dim + j] for i in range(hidden_dim)) for j in range(hidden_dim)]
        v0 = [sum(snap_trit(emb[i]) * v0_trits[i * hidden_dim + j] for i in range(hidden_dim)) for j in range(hidden_dim)]
        attn0 = v0 # 1D vector attention reduction returns v0

        # Layer 1 Attention matmuls (q1, k1, v1)
        q1_trits = T81W2Model.unpack_trits(model.tensors["model.layers.1.self_attn.q_proj.weight"])
        k1_trits = T81W2Model.unpack_trits(model.tensors["model.layers.1.self_attn.k_proj.weight"])
        v1_trits = T81W2Model.unpack_trits(model.tensors["model.layers.1.self_attn.v_proj.weight"])

        q1 = [sum(snap_trit(attn0[i]) * q1_trits[i * hidden_dim + j] for i in range(hidden_dim)) for j in range(hidden_dim)]
        k1 = [sum(snap_trit(attn0[i]) * k1_trits[i * hidden_dim + j] for i in range(hidden_dim)) for j in range(hidden_dim)]
        v1 = [sum(snap_trit(attn0[i]) * v1_trits[i * hidden_dim + j] for i in range(hidden_dim)) for j in range(hidden_dim)]
        attn1 = v1

        # lm_head logit scores reduction for sampled candidate tokens
        lm_head_trits = T81W2Model.unpack_trits(lm_head_tensor)

        candidate_scores = []
        for tok_id in sampled_token_ids:
            row_trits = lm_head_trits[tok_id * hidden_dim : (tok_id + 1) * hidden_dim]
            score = float(sum(row_trits[i] * snap_trit(attn1[i]) for i in range(hidden_dim)))
            candidate_scores.append((score, tok_id))

        # assess-fixed uses initial_probe/max_score.v1: first sampled candidate wins ties.
        best = max(candidate_scores, key=lambda x: x[0])
        selected_score, selected_token_id = best

        # Decision mapping
        decision, reason_code = VOCABULARY_MAP.get(selected_token_id, ("UNKNOWN", "UNMAPPED_TOKEN"))

        stages = dict(embedding=emb, q0=q0, k0=k0, v0=v0, attn0=attn0,
                      q1=q1, k1=k1, v1=v1, attn1=attn1)
        timings["computational_execution_ns"] = time.perf_counter_ns() - compute_start
        persistence_start = time.perf_counter_ns()
        model_ref = self.cas.put(model_bytes)
        input_ref = self.cas.put(normalized_input.encode("utf-8"))
        policy_ref = self.cas.put(policy_raw_text.encode("utf-8"))
        tokenizer_ref = self.cas.put(json.dumps(tokenizer_data, ensure_ascii=False, sort_keys=True).encode("utf-8"))
        # Result bytes match the CLI; provenance/bundle have comparator-specific contents.
        result_artifact_str = render_result({
            "schema": "t81.ai.task.assess-fixed.v1",
            "task": "assess_fixed",
            "input_hash": input_hash,
            "model_hash": model_hash,
            "decision": decision,
            "reason_code": reason_code,
            "decision_token_ids": [selected_token_id],
            "termination_reason": "single_step_max_score"
        })
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
                "selection_policy_kind": "max_score.v1",
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
            "model_ref": model_ref,
            "input_ref": input_ref,
            "policy_ref": policy_ref,
            "tokenizer_ref": tokenizer_ref,
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
        timings["artifact_persistence_ns"] = end_time - persistence_start

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
            "stages": stages,
            "timings_ns": timings,
            "model_ref": model_ref,
            "status": "pass",
            "latency_ns": end_time - start_time
        }

    def verify_and_replay(self, bundle_ref: str) -> Dict[str, Any]:
        """Recover all inputs by persisted references, verify links, then re-execute.

        A hash-valid but semantically inconsistent bundle is rejected too. No caller
        supplied model, tokenizer or policy can conceal a missing persisted input.
        """
        lookup_start = time.perf_counter_ns()
        try:
            def get_json(ref):
                data = self.cas.get(ref)
                if data is None:
                    raise ValueError("missing CAS object")
                return json.loads(data)
            bundle = get_json(bundle_ref)
            result = get_json(bundle["source_result_ref"])
            provenance = get_json(bundle["source_provenance_ref"])
            model_bytes = self.cas.get(bundle["model_ref"])
            input_bytes = self.cas.get(bundle["input_ref"])
            policy_bytes = self.cas.get(bundle["policy_ref"])
            if any(data is None for data in (model_bytes, input_bytes, policy_bytes)):
                raise ValueError("missing replay input")
            tokenizer = get_json(bundle["tokenizer_ref"])
            if "sha3-512:" + hashlib.sha3_512(model_bytes).hexdigest() != bundle["model_hash"]:
                raise ValueError("model identity mismatch")
            if canon_sha3_256_ref(input_bytes) != bundle["input_hash"]:
                raise ValueError("input identity mismatch")
            if provenance["result_ref"] != bundle["source_result_ref"]:
                raise ValueError("result linkage mismatch")
            for key in ("model_hash", "input_hash", "decision", "reason_code", "decision_token_ids"):
                if bundle[key] != result[key]:
                    raise ValueError("bundle/result mismatch: " + key)
            for key in ("model_hash", "input_hash"):
                if provenance[key] != result[key]:
                    raise ValueError("provenance mismatch: " + key)
            if provenance["policy_hash"] != canon_sha3_256_ref(policy_bytes):
                raise ValueError("policy identity mismatch")
            if provenance["policy_result"] != "allowed":
                raise ValueError("invalid policy decision")
            lookup_end = time.perf_counter_ns()
            replayed = self.run_assess_fixed(model_bytes, input_bytes.decode("utf-8"),
                                             policy_bytes.decode("utf-8"), tokenizer)
            valid = all(replayed[k] == bundle[k] for k in
                        ("model_hash", "input_hash", "decision", "reason_code", "decision_token_ids"))
            valid = valid and replayed["result_ref"] == bundle["source_result_ref"]
            valid = valid and replayed["provenance_ref"] == bundle["source_provenance_ref"]
            return dict(valid=valid, lookup_latency_ns=lookup_end-lookup_start,
                        replay_latency_ns=time.perf_counter_ns()-lookup_end)
        except (ValueError, KeyError, TypeError, PermissionError, IndexError) as error:
            return dict(valid=False, error=str(error), lookup_latency_ns=time.perf_counter_ns()-lookup_start,
                        replay_latency_ns=None)
