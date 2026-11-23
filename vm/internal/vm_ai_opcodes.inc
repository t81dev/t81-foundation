// Extracted from vm/vm.cpp (RFC-0026 / RFC-0034 AI, quantization, and ternary-weight opcodes).
// Included directly inside Interpreter::step() switch (insn.opcode) to preserve zero-overhead dispatch.

        case t81::tisc::Opcode::QMATMUL: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          auto regs = decode_ai_packed_reg_pair(insn.c);
          if (!regs.has_value()) {
            trap = Trap::DecodeFault;
            break;
          }
          const int act_reg = insn.b;
          const int wt_reg = regs->first;
          const int scale_reg = regs->second;
          if (auto res = promote_to_tensor(act_reg); !res) {
            trap = res.error();
            break;
          }
          if (auto res = promote_to_tensor(wt_reg); !res) {
            trap = res.error();
            break;
          }
          auto* tensor_act = tensor_ptr(ctx.registers[act_reg]);
          auto* tensor_wt = tensor_ptr(ctx.registers[wt_reg]);
          if (tensor_act == nullptr || tensor_wt == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          t81::core::detail::DFixed fixed_scale = t81::core::detail::DFixed::zero();
          float scale = 0.0F;
          if (ctx.register_tags[scale_reg] == ValueTag::Int) {
            scale = static_cast<float>(ctx.registers[scale_reg]);
            fixed_scale = t81::core::detail::DFixed(static_cast<int>(ctx.registers[scale_reg]));
          } else if (ctx.register_tags[scale_reg] == ValueTag::FloatHandle) {
            auto* scale_ptr = float_ptr(ctx.registers[scale_reg]);
            if (scale_ptr == nullptr) {
              trap = Trap::DecodeFault;
              break;
            }
            if (!std::isfinite(*scale_ptr)) {
              trap = Trap::TypeFault;
              break;
            }
            scale = static_cast<float>(*scale_ptr);
            using VMFloat = t81::T81Float<72, 9>;
            fixed_scale = t81::core::detail::DFixed::from_float(VMFloat::from_double(*scale_ptr));
          } else {
            trap = Trap::TypeFault;
            break;
          }
          t81::T729DynamicTensor computed;
          try {
            if (tensor_act->has_canonical_fixed_data() && tensor_wt->has_canonical_fixed_data() &&
                tensor_act->strict_core_eligible() && tensor_wt->strict_core_eligible()) {
              computed = t81::ops::qmatmul(*tensor_act, *tensor_wt, fixed_scale);
            } else {
              computed = t81::ops::qmatmul(*tensor_act, *tensor_wt, scale);
            }
          } catch (...) {
            log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor, 0, "QMATMUL shape mismatch");
            trap = Trap::ShapeFault;
            break;
          }
          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow,
                                      "QMATMUL kernel execution (phase1 packed operand encoding)"};
          record_axion_event(insn.opcode, static_cast<int32_t>(act_reg), ctx.registers[act_reg],
                             verdict);
          auto res_handle = alloc_tensor(std::move(computed));
          if (!res_handle) {
            trap = res_handle.error();
            break;
          }
          ctx.registers[insn.a] = *res_handle;
          ctx.register_tags[insn.a] = ValueTag::TensorHandle;
          break;
        }
        case t81::tisc::Opcode::EMBED: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (auto res = promote_to_tensor(insn.b); !res) {
            trap = res.error();
            break;
          }
          if (ctx.register_tags[insn.c] != ValueTag::Int) {
            trap = Trap::TypeFault;
            break;
          }
          auto* table = tensor_ptr(ctx.registers[insn.b]);
          if (table == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          const std::int64_t index = ctx.registers[insn.c];
          auto computed = t81::vm::internal::tensor_embed_checked(*table, index);
          if (!computed.has_value()) {
            log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor, static_cast<int>(index),
                             "EMBED index/table mismatch");
            trap = computed.error();
            break;
          }
          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "EMBED kernel execution"};
          record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), ctx.registers[insn.b],
                             verdict);
          auto res_handle = alloc_tensor(std::move(*computed));
          if (!res_handle) {
            trap = res_handle.error();
            break;
          }
          ctx.registers[insn.a] = *res_handle;
          ctx.register_tags[insn.a] = ValueTag::TensorHandle;
          break;
        }
        case t81::tisc::Opcode::WLOAD: {
          // WLOAD RD, R_SRC, R_POLICY — AI-M4: CanonFS audit gate active.
          // R_POLICY register is reserved for phase-2 ambient policy dispatch.
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          const auto src_tag = ctx.register_tags[insn.b];
          const auto src_value = ctx.registers[insn.b];
          const auto* native_src =
              src_tag == ValueTag::WeightsTensorHandle ? weights_tensor(src_value) : nullptr;
          if (state_.policy.has_value() && native_src != nullptr) {
            if (!state_.policy->allowed_ternary_model_hashes.empty()) {
              const bool hash_allowed =
                  state_.weights_model != nullptr && !state_.weights_model->checksum.empty() &&
                  std::any_of(state_.policy->allowed_ternary_model_hashes.begin(),
                              state_.policy->allowed_ternary_model_hashes.end(),
                              [&](const std::string& allowed) {
                                return hash_matches_policy_entry(allowed,
                                                                 state_.weights_model->checksum);
                              });
              if (!hash_allowed) {
                const std::string checksum =
                    state_.weights_model != nullptr ? state_.weights_model->checksum : "";
                t81::axion::Verdict verdict{
                    t81::axion::VerdictKind::Deny,
                    checksum.empty()
                        ? "WLOAD denied (allowed-ternary-model-hashes: missing model checksum)"
                        : "WLOAD denied (allowed-ternary-model-hashes mismatch checksum=sha3-512:" +
                              checksum + ")"};
                record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), src_value, verdict);
                trap = Trap::SecurityFault;
                break;
              }
            }
            if (state_.policy->ternary_weight_domain_check) {
              auto decoded = t81::vm::internal::decode_native_tensor(
                  *native_src, t81::vm::internal::TensorDecodeMode::StrictCanonical);
              if (!decoded.has_value() || !is_exact_ternary_tensor(*decoded)) {
                t81::axion::Verdict verdict{t81::axion::VerdictKind::Deny,
                                            "WLOAD denied (ternary-weight-domain-check failed)"};
                record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), src_value, verdict);
                trap = Trap::SecurityFault;
                break;
              }
            }
          }
          if (auto res = promote_to_tensor(insn.b); !res) {
            trap = res.error();
            break;
          }
          auto* wload_src = tensor_ptr(ctx.registers[insn.b]);
          if (wload_src == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          auto wload_computed = t81::vm::internal::tensor_wload_checked(*wload_src);
          if (!wload_computed.has_value()) {
            trap = wload_computed.error();
            break;
          }
          auto wload_handle = alloc_tensor(std::move(*wload_computed));
          if (!wload_handle) {
            trap = wload_handle.error();
            break;
          }
          // AI-M4: CanonFS audit — "meta slot axion event segment=meta addr=<n> action=WeightLoad"
          if (canonfs_driver_) {
            t81::vm::internal::log_canonfs_operation(state_, state_.current_context, insn.opcode,
                                                     t81::axion::reasons::kWeightLoad);
          }
          // Tensor provenance record for the materialized weight handle.
          const auto wload_out_h = static_cast<std::size_t>(*wload_handle);
          if (const auto& wload_stored = state_.tensors[wload_out_h - 1];
              wload_stored.has_value()) {
            t81::vm::internal::log_tensor_provenance(state_, state_.current_context, insn.opcode,
                                                     wload_out_h, wload_stored.value(), "wload");
          }
          t81::axion::Verdict wload_verdict{t81::axion::VerdictKind::Allow,
                                            "WLOAD weight materialization"};
          record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), ctx.registers[insn.b],
                             wload_verdict);
          ctx.registers[insn.a] = *wload_handle;
          ctx.register_tags[insn.a] = ValueTag::TensorHandle;
          break;
        }
        case t81::tisc::Opcode::GATHER: {
          // GATHER RD, R_SRC, PACK(R_IDX, R_AXIS) — AI-M5: axis register now active.
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          auto gather_regs = decode_ai_packed_reg_pair(insn.c);
          if (!gather_regs.has_value()) {
            trap = Trap::DecodeFault;
            break;
          }
          const int gather_idx_reg = gather_regs->first;
          const int gather_axis_reg = gather_regs->second;
          if (ctx.register_tags[gather_idx_reg] != ValueTag::Int ||
              ctx.register_tags[gather_axis_reg] != ValueTag::Int) {
            trap = Trap::TypeFault;
            break;
          }
          if (auto res = promote_to_tensor(insn.b); !res) {
            trap = res.error();
            break;
          }
          auto* gather_src = tensor_ptr(ctx.registers[insn.b]);
          if (gather_src == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          const std::int64_t gather_index = ctx.registers[gather_idx_reg];
          const int gather_axis = static_cast<int>(ctx.registers[gather_axis_reg]);
          auto gather_computed =
              t81::vm::internal::tensor_gather_checked(*gather_src, gather_index, gather_axis);
          if (!gather_computed.has_value()) {
            log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor, static_cast<int>(gather_index),
                             "GATHER index/axis mismatch");
            trap = gather_computed.error();
            break;
          }
          t81::axion::Verdict gather_verdict{t81::axion::VerdictKind::Allow,
                                             "GATHER kernel execution"};
          record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), ctx.registers[insn.b],
                             gather_verdict);
          auto gather_handle = alloc_tensor(std::move(*gather_computed));
          if (!gather_handle) {
            trap = gather_handle.error();
            break;
          }
          ctx.registers[insn.a] = *gather_handle;
          ctx.register_tags[insn.a] = ValueTag::TensorHandle;
          break;
        }
        case t81::tisc::Opcode::SCATTER: {
          // SCATTER RD, R_DST, PACK(R_IDX, R_SRC) — AI-M5: aliasing detection active.
          // Phase-1 packed encoding convention: axis is fixed to 0.
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          auto scatter_regs = decode_ai_packed_reg_pair(insn.c);
          if (!scatter_regs.has_value()) {
            trap = Trap::DecodeFault;
            break;
          }
          const int scatter_idx_reg = scatter_regs->first;
          const int scatter_src_reg = scatter_regs->second;
          if (ctx.register_tags[scatter_idx_reg] != ValueTag::Int) {
            trap = Trap::TypeFault;
            break;
          }
          if (auto res = promote_to_tensor(insn.b); !res) {
            trap = res.error();
            break;
          }
          if (auto res = promote_to_tensor(scatter_src_reg); !res) {
            trap = res.error();
            break;
          }
          auto* scatter_dst = tensor_ptr(ctx.registers[insn.b]);
          auto* scatter_src_ptr = tensor_ptr(ctx.registers[scatter_src_reg]);
          if (scatter_dst == nullptr || scatter_src_ptr == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          const std::int64_t scatter_dst_handle = ctx.registers[insn.b];
          const std::int64_t scatter_index = ctx.registers[scatter_idx_reg];
          constexpr int scatter_axis = 0;  // Phase-1 packed encoding convention.

          // AI-M5: aliasing detection — RFC §5.15.6 MUST enforcement.
          const auto alias_key = std::make_tuple(scatter_dst_handle, scatter_axis, scatter_index);
          if (ctx.scatter_used.count(alias_key) != 0) {
            t81::axion::Verdict alias_verdict{
                t81::axion::VerdictKind::Deny,
                "SCATTER aliasing violation: same (dst, axis, index) reused in execution frame"};
            record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), scatter_dst_handle,
                               alias_verdict);
            trap = Trap::SecurityFault;
            break;
          }
          ctx.scatter_used.insert(alias_key);

          auto scatter_computed = t81::vm::internal::tensor_scatter_checked(
              *scatter_dst, scatter_index, *scatter_src_ptr, scatter_axis);
          if (!scatter_computed.has_value()) {
            log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                             static_cast<int>(scatter_index), "SCATTER index/shape mismatch");
            trap = scatter_computed.error();
            break;
          }
          t81::axion::Verdict scatter_verdict{t81::axion::VerdictKind::Allow,
                                              "SCATTER kernel execution"};
          record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), ctx.registers[insn.b],
                             scatter_verdict);
          auto scatter_handle = alloc_tensor(std::move(*scatter_computed));
          if (!scatter_handle) {
            trap = scatter_handle.error();
            break;
          }
          ctx.registers[insn.a] = *scatter_handle;
          ctx.register_tags[insn.a] = ValueTag::TensorHandle;
          break;
        }
        case t81::tisc::Opcode::BitAnd:
        case t81::tisc::Opcode::BitOr:
        case t81::tisc::Opcode::BitXor: {
          if (auto bit_trap = handle_bitwise_binary(); bit_trap.has_value()) {
            trap = *bit_trap;
          }
          break;
        }
        case t81::tisc::Opcode::BitNot: {
          if (auto bit_trap = handle_bitwise_not(); bit_trap.has_value()) {
            trap = *bit_trap;
          }
          break;
        }
        case t81::tisc::Opcode::BitShl:
        case t81::tisc::Opcode::BitShr:
        case t81::tisc::Opcode::BitUShr: {
          if (auto bit_trap = handle_bitwise_shift(); bit_trap.has_value()) {
            trap = *bit_trap;
          }
          break;
        }
        case t81::tisc::Opcode::MapNew:
        case t81::tisc::Opcode::SetNew: {
          if (!reg_ok(insn.a)) {
            trap = Trap::DecodeFault;
            break;
          }
          ctx.registers[insn.a] = alloc_string_vector();
          ctx.register_tags[insn.a] = ValueTag::StringVectorHandle;
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::MapPut: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          // map=a (in/out), key=b, value=c.
          // NOTE: T81 TISC usually uses A as destination. Here A is also the map handle source?
          // Wait, typical TISC is dst, src1, src2.
          // So: dest_reg = MapPut(map_reg, key_reg, val_reg) ??
          // The IR generator call I'm planning to write will likely emit:
          // MapPut dest, map, key ?? No, TISC is 3 operands max.
          // But Put needs Map, Key, Value.
          // Let's assume: A=Map, B=Key, C=Value. And it updates Map in place (since handles are
          // refs). Return value? Map handle.
          if (ctx.register_tags[insn.a] != ValueTag::StringVectorHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* vec = string_vector_mut(ctx.registers[insn.a]);
          auto key = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
          auto val = symbol_like_text(ctx.register_tags[insn.c], ctx.registers[insn.c]);
          if (!vec || !key.has_value() || !val.has_value()) {
            trap = Trap::DecodeFault;
            break;
          }
          bool found = false;
          for (size_t i = 0; i + 1 < vec->size(); i += 2) {
            if ((*vec)[i] == *key) {
              (*vec)[i + 1] = *val;
              found = true;
              break;
            }
          }
          if (!found) {
            vec->push_back(std::string(*key));
            vec->push_back(std::string(*val));
          }
          // Result is the map handle (A)
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::MapGet: {
          // A=Dest, B=Map, C=Key
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.b] != ValueTag::StringVectorHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* vec = string_vector_ptr(ctx.registers[insn.b]);
          auto key = symbol_like_text(ctx.register_tags[insn.c], ctx.registers[insn.c]);
          if (!vec || !key.has_value()) {
            trap = Trap::DecodeFault;
            break;
          }
          std::optional<std::string> val;
          for (size_t i = 0; i + 1 < vec->size(); i += 2) {
            if ((*vec)[i] == *key) {
              val = (*vec)[i + 1];
              break;
            }
          }
          if (val) {
            std::int64_t handle = intern_symbol(*val);
            std::int64_t opt_handle = intern_option(true, ValueTag::SymbolHandle, handle);
            ctx.registers[insn.a] = opt_handle;
            ctx.register_tags[insn.a] = ValueTag::OptionHandle;
          } else {
            std::int64_t opt_handle = intern_option(false, ValueTag::Int, 0);
            ctx.registers[insn.a] = opt_handle;
            ctx.register_tags[insn.a] = ValueTag::OptionHandle;
          }
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::MapHas: {
          // A=Dest, B=Map, C=Key
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.b] != ValueTag::StringVectorHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* vec = string_vector_ptr(ctx.registers[insn.b]);
          auto key = symbol_like_text(ctx.register_tags[insn.c], ctx.registers[insn.c]);
          if (!vec || !key.has_value()) {
            trap = Trap::DecodeFault;
            break;
          }
          bool found = false;
          for (size_t i = 0; i + 1 < vec->size(); i += 2) {
            if ((*vec)[i] == *key) {
              found = true;
              break;
            }
          }
          ctx.registers[insn.a] = found ? 1 : 0;
          ctx.register_tags[insn.a] = ValueTag::Bool;
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::MapRemove: {
          // A=Dest(Map), B=Map, C=Key -- Assume in-place update of B, returning B in A.
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.b] != ValueTag::StringVectorHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* vec = string_vector_mut(ctx.registers[insn.b]);
          auto key = symbol_like_text(ctx.register_tags[insn.c], ctx.registers[insn.c]);
          if (!vec || !key.has_value()) {
            trap = Trap::DecodeFault;
            break;
          }
          for (size_t i = 0; i + 1 < vec->size(); i += 2) {
            if ((*vec)[i] == *key) {
              // Remove key and value
              vec->erase(vec->begin() + i, vec->begin() + i + 2);
              break;
            }
          }
          ctx.registers[insn.a] = ctx.registers[insn.b];
          ctx.register_tags[insn.a] = ValueTag::StringVectorHandle;
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::MapKeys: {
          // A=Dest(Vec), B=Map
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.b] != ValueTag::StringVectorHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* vec = string_vector_ptr(ctx.registers[insn.b]);
          if (!vec) {
            trap = Trap::DecodeFault;
            break;
          }
          std::vector<std::string> keys;
          for (size_t i = 0; i + 1 < vec->size(); i += 2) {
            keys.push_back((*vec)[i]);
          }
          state_.string_vectors.push_back(std::move(keys));
          ctx.registers[insn.a] = static_cast<std::int64_t>(state_.string_vectors.size());
          ctx.register_tags[insn.a] = ValueTag::StringVectorHandle;
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::MapSize: {
          // A=Dest(Int), B=Map
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.b] != ValueTag::StringVectorHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* vec = string_vector_ptr(ctx.registers[insn.b]);
          if (!vec) {
            trap = Trap::DecodeFault;
            break;
          }
          ctx.registers[insn.a] = static_cast<std::int64_t>(vec->size() / 2);
          ctx.register_tags[insn.a] = ValueTag::Int;
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::SetAdd: {
          // A=Set(in/out), B=Key
          // TISC instructions usually have A as dest. We will use A as Set handle.
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.a] != ValueTag::StringVectorHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* vec = string_vector_mut(ctx.registers[insn.a]);
          auto key = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
          if (!vec || !key.has_value()) {
            trap = Trap::DecodeFault;
            break;
          }
          bool found = false;
          for (const auto& item : *vec) {
            if (item == *key) {
              found = true;
              break;
            }
          }
          if (!found) {
            vec->push_back(std::string(*key));
          }
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::SetRemove: {
          // A=Set(in/out), B=Key
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.a] != ValueTag::StringVectorHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* vec = string_vector_mut(ctx.registers[insn.a]);
          auto key = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
          if (!vec || !key.has_value()) {
            trap = Trap::DecodeFault;
            break;
          }
          for (auto it = vec->begin(); it != vec->end(); ++it) {
            if (*it == *key) {
              vec->erase(it);
              break;
            }
          }
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::SetHas: {
          // A=Dest(Bool), B=Set, C=Key
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.b] != ValueTag::StringVectorHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* vec = string_vector_ptr(ctx.registers[insn.b]);
          auto key = symbol_like_text(ctx.register_tags[insn.c], ctx.registers[insn.c]);
          if (!vec || !key.has_value()) {
            trap = Trap::DecodeFault;
            break;
          }
          bool found = false;
          for (const auto& item : *vec) {
            if (item == *key) {
              found = true;
              break;
            }
          }
          ctx.registers[insn.a] = found ? 1 : 0;
          ctx.register_tags[insn.a] = ValueTag::Bool;
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::SetSize: {
          // A=Dest(Int), B=Set
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.b] != ValueTag::StringVectorHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* vec = string_vector_ptr(ctx.registers[insn.b]);
          if (!vec) {
            trap = Trap::DecodeFault;
            break;
          }
          ctx.registers[insn.a] = static_cast<std::int64_t>(vec->size());
          ctx.register_tags[insn.a] = ValueTag::Int;
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::TShape: {
          // TShape A, B, C — A = shape[R[C]] of tensor R[B]
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (auto res = promote_to_tensor(insn.b); !res) {
            trap = res.error();
            break;
          }
          auto* tensor = tensor_ptr(ctx.registers[insn.b]);
          if (!tensor) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.c] != ValueTag::Int) {
            trap = Trap::TypeFault;
            break;
          }
          std::int64_t dim_idx = ctx.registers[insn.c];
          const auto& shape = tensor->shape();
          if (dim_idx < 0 || static_cast<std::size_t>(dim_idx) >= shape.size()) {
            trap = Trap::BoundsFault;
            break;
          }
          ctx.registers[insn.a] =
              static_cast<std::int64_t>(shape[static_cast<std::size_t>(dim_idx)]);
          ctx.register_tags[insn.a] = ValueTag::Int;
          update_flags(ctx.registers[insn.a]);
          break;
        }
        // -----------------------------------------------------------------------
        // RFC-0034 §5.17 — Ternary-Native Inference Operations
        // -----------------------------------------------------------------------
        case t81::tisc::Opcode::TWMATMUL: {
          // TWMATMUL RD, R_ACT, R_WT — ternary-weight matmul; T81BigInt accumulator.
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (auto res = promote_to_tensor(insn.b); !res) {
            trap = res.error();
            break;
          }
          auto* act_t = tensor_ptr(ctx.registers[insn.b]);
          if (!act_t) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.c] == ValueTag::WeightsTensorHandle) {
            const auto handle = ctx.registers[insn.c];
            auto authorized = authorize_weights_tensor(insn.c, handle);
            if (!authorized.has_value()) {
              trap = authorized.error();
              break;
            }
            if (*authorized != nullptr) {
              const std::size_t cache_idx = static_cast<std::size_t>(handle - 1);
              if (cache_idx < state_.weights_tensor_trits.size() &&
                  state_.weights_tensor_trits[cache_idx].has_value()) {
                if (auto direct = t81::vm::internal::native_tensor_twmatmul_direct(
                        *act_t, **authorized, *state_.weights_tensor_trits[cache_idx]);
                    direct.has_value()) {
                  t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow,
                                              "TWMATMUL ternary-weight matmul (RFC-0034)"};
                  record_axion_event(insn.opcode, static_cast<int32_t>(insn.b),
                                     ctx.registers[insn.b], verdict);
                  auto rh = alloc_tensor(std::move(*direct));
                  if (!rh) {
                    trap = rh.error();
                    break;
                  }
                  ctx.registers[insn.a] = *rh;
                  ctx.register_tags[insn.a] = ValueTag::TensorHandle;
                  break;
                }
              }
            }
          }
          if (auto res = promote_to_tensor(insn.c); !res) {
            trap = res.error();
            break;
          }
          auto* wt_t = tensor_ptr(ctx.registers[insn.c]);
          if (!wt_t) {
            trap = Trap::DecodeFault;
            break;
          }
          try {
            auto result = t81::ops::twmatmul(*act_t, *wt_t);
            t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow,
                                        "TWMATMUL ternary-weight matmul (RFC-0034)"};
            record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), ctx.registers[insn.b],
                               verdict);
            auto rh = alloc_tensor(std::move(result));
            if (!rh) {
              trap = rh.error();
              break;
            }
            ctx.registers[insn.a] = *rh;
            ctx.register_tags[insn.a] = ValueTag::TensorHandle;
          } catch (...) {
            trap = Trap::ShapeFault;
          }
          break;
        }
        case t81::tisc::Opcode::TQUANT: {
          // TQUANT RD, R_SRC, R_THR — quantize T729DynamicTensor to ternary {-1,0,+1}.
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          // R_THR is a float handle or raw int (converted to threshold).
          float threshold = 0.5f;
          if (ctx.register_tags[insn.c] == ValueTag::FloatHandle) {
            auto* fp = float_ptr(ctx.registers[insn.c]);
            if (!fp) {
              trap = Trap::DecodeFault;
              break;
            }
            threshold = static_cast<float>(*fp);
          } else {
            threshold = static_cast<float>(ctx.registers[insn.c]);
          }
          if (ctx.register_tags[insn.b] == ValueTag::WeightsTensorHandle) {
            const auto handle = ctx.registers[insn.b];
            auto authorized = authorize_weights_tensor(insn.b, handle);
            if (!authorized.has_value()) {
              trap = authorized.error();
              break;
            }
            if (*authorized != nullptr) {
              const std::size_t cache_idx = static_cast<std::size_t>(handle - 1);
              if (cache_idx < state_.weights_tensor_trits.size() &&
                  state_.weights_tensor_trits[cache_idx].has_value()) {
                if (auto direct = t81::vm::internal::native_tensor_quant_direct(
                        **authorized, *state_.weights_tensor_trits[cache_idx], threshold);
                    direct.has_value()) {
                  t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow,
                                              "TQUANT ternary quantization (RFC-0034)"};
                  record_axion_event(insn.opcode, static_cast<int32_t>(insn.b),
                                     ctx.registers[insn.b], verdict);
                  auto rh = alloc_tensor(std::move(*direct));
                  if (!rh) {
                    trap = rh.error();
                    break;
                  }
                  ctx.registers[insn.a] = *rh;
                  ctx.register_tags[insn.a] = ValueTag::TensorHandle;
                  break;
                }
              }
            }
          }
          if (auto res = promote_to_tensor(insn.b); !res) {
            trap = res.error();
            break;
          }
          auto* src_t = tensor_ptr(ctx.registers[insn.b]);
          if (!src_t) {
            trap = Trap::DecodeFault;
            break;
          }
          try {
            auto result = t81::ops::tquant(*src_t, threshold);
            t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow,
                                        "TQUANT ternary quantization (RFC-0034)"};
            record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), ctx.registers[insn.b],
                               verdict);
            auto rh = alloc_tensor(std::move(result));
            if (!rh) {
              trap = rh.error();
              break;
            }
            ctx.registers[insn.a] = *rh;
            ctx.register_tags[insn.a] = ValueTag::TensorHandle;
          } catch (...) {
            trap = Trap::TypeFault;
          }
          break;
        }
        case t81::tisc::Opcode::TATTN: {
          // TATTN RD, R_Q, PACK(R_K, R_V) — ternary Q/K attention.
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          auto kv_regs = decode_ai_packed_reg_pair(insn.c);
          if (!kv_regs.has_value()) {
            trap = Trap::DecodeFault;
            break;
          }
          const int q_reg = insn.b;
          const int k_reg = kv_regs->first;
          const int v_reg = kv_regs->second;
          if (auto res = promote_to_tensor(q_reg); !res) {
            trap = res.error();
            break;
          }
          if (auto res = promote_to_tensor(v_reg); !res) {
            trap = res.error();
            break;
          }
          auto* q_t = tensor_ptr(ctx.registers[q_reg]);
          auto* v_t = tensor_ptr(ctx.registers[v_reg]);
          if (!q_t || !v_t) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[k_reg] == ValueTag::WeightsTensorHandle) {
            const auto handle = ctx.registers[k_reg];
            auto authorized = authorize_weights_tensor(k_reg, handle);
            if (!authorized.has_value()) {
              trap = authorized.error();
              break;
            }
            if (*authorized != nullptr) {
              const std::size_t cache_idx = static_cast<std::size_t>(handle - 1);
              if (cache_idx < state_.weights_tensor_trits.size() &&
                  state_.weights_tensor_trits[cache_idx].has_value()) {
                if (auto direct = t81::vm::internal::native_tensor_tattn_direct(
                        *q_t, **authorized, *state_.weights_tensor_trits[cache_idx], *v_t);
                    direct.has_value()) {
                  t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow,
                                              "TATTN ternary QK attention (RFC-0034)"};
                  record_axion_event(insn.opcode, static_cast<int32_t>(q_reg), ctx.registers[q_reg],
                                     verdict);
                  auto rh = alloc_tensor(std::move(*direct));
                  if (!rh) {
                    trap = rh.error();
                    break;
                  }
                  ctx.registers[insn.a] = *rh;
                  ctx.register_tags[insn.a] = ValueTag::TensorHandle;
                  break;
                }
              }
            }
          }
          if (auto res = promote_to_tensor(k_reg); !res) {
            trap = res.error();
            break;
          }
          auto* k_t = tensor_ptr(ctx.registers[k_reg]);
          if (!k_t) {
            trap = Trap::DecodeFault;
            break;
          }
          try {
            auto result = t81::ops::tattn(*q_t, *k_t, *v_t);
            t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow,
                                        "TATTN ternary QK attention (RFC-0034)"};
            record_axion_event(insn.opcode, static_cast<int32_t>(q_reg), ctx.registers[q_reg],
                               verdict);
            auto rh = alloc_tensor(std::move(result));
            if (!rh) {
              trap = rh.error();
              break;
            }
            ctx.registers[insn.a] = *rh;
            ctx.register_tags[insn.a] = ValueTag::TensorHandle;
          } catch (...) {
            trap = Trap::ShapeFault;
          }
          break;
        }
        case t81::tisc::Opcode::TWEMBED: {
          // TWEMBED RD, R_TABLE, R_IDX — row-gather from T81Qutrit embedding table.
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.c] != ValueTag::Int) {
            trap = Trap::TypeFault;
            break;
          }
          const std::int64_t idx = ctx.registers[insn.c];
          if (ctx.register_tags[insn.b] == ValueTag::WeightsTensorHandle) {
            auto authorized = authorize_weights_tensor(insn.b, ctx.registers[insn.b]);
            if (!authorized.has_value()) {
              trap = authorized.error();
              break;
            }
            if (*authorized != nullptr) {
              if (auto direct = t81::vm::internal::native_tensor_twembed_direct(**authorized, idx);
                  direct.has_value()) {
                t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow,
                                            "TWEMBED ternary embed gather (RFC-0034)"};
                record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), ctx.registers[insn.b],
                                   verdict);
                auto rh = alloc_tensor(std::move(*direct));
                if (!rh) {
                  trap = rh.error();
                  break;
                }
                ctx.registers[insn.a] = *rh;
                ctx.register_tags[insn.a] = ValueTag::TensorHandle;
                break;
              }
            }
          }
          if (auto res = promote_to_tensor(insn.b); !res) {
            trap = res.error();
            break;
          }
          auto* table_t = tensor_ptr(ctx.registers[insn.b]);
          if (!table_t) {
            trap = Trap::DecodeFault;
            break;
          }
          try {
            auto result = t81::ops::twembed(*table_t, idx);
            t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow,
                                        "TWEMBED ternary embed gather (RFC-0034)"};
            record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), ctx.registers[insn.b],
                               verdict);
            auto rh = alloc_tensor(std::move(result));
            if (!rh) {
              trap = rh.error();
              break;
            }
            ctx.registers[insn.a] = *rh;
            ctx.register_tags[insn.a] = ValueTag::TensorHandle;
          } catch (const std::out_of_range&) {
            trap = Trap::BoundsFault;
          } catch (...) {
            trap = Trap::TypeFault;
          }
          break;
        }
        case t81::tisc::Opcode::TERNACCUM: {
          // TERNACCUM RD, R_WT, R_ACT — scalar ternary dot product → T81BigInt handle.
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (auto res = promote_to_tensor(insn.c); !res) {
            trap = res.error();
            break;
          }
          auto* act_t = tensor_ptr(ctx.registers[insn.c]);
          if (!act_t) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.b] == ValueTag::WeightsTensorHandle) {
            const auto handle = ctx.registers[insn.b];
            auto authorized = authorize_weights_tensor(insn.b, handle);
            if (!authorized.has_value()) {
              trap = authorized.error();
              break;
            }
            if (*authorized != nullptr) {
              const std::size_t cache_idx = static_cast<std::size_t>(handle - 1);
              if (cache_idx < state_.weights_tensor_trits.size() &&
                  state_.weights_tensor_trits[cache_idx].has_value()) {
                if (auto direct = t81::vm::internal::native_tensor_ternaccum_direct(
                        **authorized, *state_.weights_tensor_trits[cache_idx], *act_t);
                    direct.has_value()) {
                  t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow,
                                              "TERNACCUM ternary dot product (RFC-0034)"};
                  record_axion_event(insn.opcode, static_cast<int32_t>(insn.b),
                                     ctx.registers[insn.b], verdict);
                  const std::int64_t bh = alloc_bigint(std::move(*direct));
                  ctx.registers[insn.a] = bh;
                  ctx.register_tags[insn.a] = ValueTag::BigIntHandle;
                  break;
                }
              }
            }
          }
          if (auto res = promote_to_tensor(insn.b); !res) {
            trap = res.error();
            break;
          }
          auto* wt_t = tensor_ptr(ctx.registers[insn.b]);
          if (!wt_t) {
            trap = Trap::DecodeFault;
            break;
          }
          try {
            auto result_tensor = t81::ops::ternaccum(*wt_t, *act_t);
            // Extract scalar BigInt from 1×1 result tensor.
            auto vals = result_tensor.snapshot_values();
            const std::int64_t dot = (vals.empty() ? 0 : static_cast<std::int64_t>(vals[0]));
            t81::v1::T81BigInt bigint_result(dot);
            t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow,
                                        "TERNACCUM ternary dot product (RFC-0034)"};
            record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), ctx.registers[insn.b],
                               verdict);
            const std::int64_t bh = alloc_bigint(std::move(bigint_result));
            ctx.registers[insn.a] = bh;
            ctx.register_tags[insn.a] = ValueTag::BigIntHandle;
          } catch (...) {
            trap = Trap::TypeFault;
          }
          break;
        }
        case t81::tisc::Opcode::TACT: {
          // TACT RD, R_SRC, R_MODE — ternary activation + activation-ceiling gate.
          // §5.17.6: post-execute Axion verdict: Allow / Quarantine / Deny.
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.c] != ValueTag::Int) {
            trap = Trap::TypeFault;
            break;
          }
          const auto mode = static_cast<std::uint8_t>(ctx.registers[insn.c]);
          if (mode != t81::ops::kTActModeStep && mode != t81::ops::kTActModeTanh) {
            trap = Trap::DecodeFault;
            break;
          }
          T729DynamicTensor tact_result;
          bool have_tact_result = false;
          if (ctx.register_tags[insn.b] == ValueTag::WeightsTensorHandle) {
            const auto handle = ctx.registers[insn.b];
            auto authorized = authorize_weights_tensor(insn.b, handle);
            if (!authorized.has_value()) {
              trap = authorized.error();
              break;
            }
            if (*authorized != nullptr) {
              const std::size_t cache_idx = static_cast<std::size_t>(handle - 1);
              if (cache_idx < state_.weights_tensor_trits.size() &&
                  state_.weights_tensor_trits[cache_idx].has_value()) {
                if (auto direct = t81::vm::internal::native_tensor_tact_direct(
                        **authorized, *state_.weights_tensor_trits[cache_idx], mode);
                    direct.has_value()) {
                  tact_result = std::move(*direct);
                  have_tact_result = true;
                }
              }
            }
          }
          if (!have_tact_result) {
            if (auto res = promote_to_tensor(insn.b); !res) {
              trap = res.error();
              break;
            }
            auto* src_t = tensor_ptr(ctx.registers[insn.b]);
            if (!src_t) {
              trap = Trap::DecodeFault;
              break;
            }
            try {
              tact_result = t81::ops::tact(*src_t, mode);
            } catch (...) {
              trap = Trap::TypeFault;
              break;
            }
          }
          // Post-execute activation-ceiling gate (RFC-0034 §5.17.6).
          t81::axion::VerdictKind ceiling_verdict = t81::axion::VerdictKind::Allow;
          std::string ceiling_reason = "TACT activation-ceiling: Allow";
          if (state_.policy.has_value() &&
              state_.policy->activation_ceiling_max_nonzero_fraction.has_value()) {
            const double max_frac = *state_.policy->activation_ceiling_max_nonzero_fraction;
            const auto out_vals = tact_result.snapshot_values();
            std::size_t nonzero = 0;
            for (float v : out_vals) {
              if (v != 0.0f) ++nonzero;
            }
            const double frac = out_vals.empty() ? 0.0
                                                 : static_cast<double>(nonzero) /
                                                       static_cast<double>(out_vals.size());
            if (frac > max_frac) {
              std::ostringstream oss;
              if (ctx.activation_quarantined) {
                ceiling_verdict = t81::axion::VerdictKind::Deny;
                oss << "TACT activation-ceiling: nonzero-fraction=" << frac
                    << " exceeds max=" << max_frac << " (Deny)";
              } else {
                ceiling_verdict = t81::axion::VerdictKind::Quarantine;
                oss << "TACT activation-ceiling: nonzero-fraction=" << frac
                    << " exceeds max=" << max_frac << " (Quarantine)";
              }
              ceiling_reason = oss.str();
            }
          }
          t81::axion::Verdict verdict{ceiling_verdict, ceiling_reason};
          record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), ctx.registers[insn.b],
                             verdict);
          if (ceiling_verdict == t81::axion::VerdictKind::Quarantine) {
            // Quarantine: RD not committed; PC does not advance.
            ctx.activation_quarantined = true;
            restore_pc_on_trap = true;
            trap = Trap::SecurityFault;
            break;
          }
          if (ceiling_verdict == t81::axion::VerdictKind::Deny) {
            ctx.activation_quarantined = true;
            restore_pc_on_trap = true;
            trap = Trap::ActivationFault;
            break;
          }
          // Allow: commit result.
          ctx.activation_quarantined = false;
          auto rh = alloc_tensor(std::move(tact_result));
          if (!rh) {
            trap = rh.error();
            break;
          }
          ctx.registers[insn.a] = *rh;
          ctx.register_tags[insn.a] = ValueTag::TensorHandle;
          break;
        }
        case t81::tisc::Opcode::POLYMUL: {
          // RFC-0038 §5.1 — Negacyclic polynomial multiply in Z[x]/(x^n + 1).
          // Encoding: POLYMUL RD=insn.a, R_A=insn.b, R_B=insn.c
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (auto res = promote_to_tensor(insn.b); !res) {
            trap = res.error();
            break;
          }
          if (auto res = promote_to_tensor(insn.c); !res) {
            trap = res.error();
            break;
          }
          auto* ta = tensor_ptr(ctx.registers[insn.b]);
          auto* tb = tensor_ptr(ctx.registers[insn.c]);
          if (!ta || !tb) {
            trap = Trap::DecodeFault;
            break;
          }
          try {
            auto result = t81::ops::polymul(*ta, *tb);
            auto rh = alloc_tensor(std::move(result));
            if (!rh) {
              trap = rh.error();
              break;
            }
            ctx.registers[insn.a] = *rh;
            ctx.register_tags[insn.a] = ValueTag::TensorHandle;
          } catch (...) {
            trap = Trap::ShapeFault;
          }
          break;
        }
        case t81::tisc::Opcode::POLYMOD: {
          // RFC-0038 §5.2 — Centered coefficient reduction mod q.
          // Encoding: POLYMOD RD=insn.a, R_A=insn.b, R_Q=insn.c (q from register)
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (auto res = promote_to_tensor(insn.b); !res) {
            trap = res.error();
            break;
          }
          auto* ta = tensor_ptr(ctx.registers[insn.b]);
          if (!ta) {
            trap = Trap::DecodeFault;
            break;
          }
          const std::int64_t q = static_cast<std::int64_t>(ctx.registers[insn.c]);
          if (q <= 0) {
            trap = Trap::DecodeFault;
            break;
          }
          try {
            auto result = t81::ops::polymod(*ta, q);
            auto rh = alloc_tensor(std::move(result));
            if (!rh) {
              trap = rh.error();
              break;
            }
            ctx.registers[insn.a] = *rh;
            ctx.register_tags[insn.a] = ValueTag::TensorHandle;
          } catch (...) {
            trap = Trap::ShapeFault;
          }
          break;
        }
        case t81::tisc::Opcode::FFICall: {
          auto dispatcher = t81::vm::get_ffi_dispatcher();
          if (!dispatcher) {
            trap = Trap::FFINotInitialized;
            break;
          }

          auto result = t81::vm::ffi_call(state_, insn.a, static_cast<uint64_t>(insn.b), insn.c);
          if (!result.has_value()) {
            trap = result.error();
          }
          break;
        }
        case t81::tisc::Opcode::FFIRegister: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          auto result = t81::vm::ffi_register(state_, insn.a, static_cast<std::int32_t>(insn.b));
          if (!result.has_value()) {
            trap = result.error();
          }
          break;
        }
        case t81::tisc::Opcode::FFIPolicySet: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.a] != ValueTag::Int ||
              ctx.register_tags[insn.b] != ValueTag::Int) {
            trap = Trap::TypeFault;
            break;
          }
          uint64_t policy_type = static_cast<uint64_t>(ctx.registers[insn.a]);
          uint64_t policy_value = static_cast<uint64_t>(ctx.registers[insn.b]);

          auto result = t81::vm::ffi_policy_set(state_, policy_type, policy_value);
          if (!result.has_value()) {
            trap = result.error();
          }
          break;
        }
        // RFC-00E1 — Controlled Stochastic Inference (CSI) Opcode Extensions
        // Experimental opcodes for policy-gated stochastic operations
        case t81::tisc::Opcode::STOCHASTIC_DECODE:
        case t81::tisc::Opcode::STOCHASTIC_SAMPLE:
        case t81::tisc::Opcode::STOCHASTIC_CHAIN_BEGIN:
        case t81::tisc::Opcode::STOCHASTIC_CHAIN_STEP:
        case t81::tisc::Opcode::STOCHASTIC_CHAIN_END:
        case t81::tisc::Opcode::STOCHASTIC_CONFIG:
        case t81::tisc::Opcode::STOCHASTIC_SEED:
        case t81::tisc::Opcode::STOCHASTIC_VERIFY:
        case t81::tisc::Opcode::POLICY_EVAL_STOCHASTIC:
        case t81::tisc::Opcode::POLICY_CONSTRAIN_ENTROPY:
        case t81::tisc::Opcode::POLICY_FILTER_TOKENS:
        case t81::tisc::Opcode::POLICY_RECORD_DECISION: {
          // Delegate to CSI integration if enabled
          trap = t81::vm::csi::execute_csi_opcode_if_enabled(insn, ctx);
          break;
        }
        // RFC-00E2 — Advanced AI Opcode Extensions (0xE0-0xEF range)
        // Experimental opcodes for advanced neural network and quantization operations
        case t81::tisc::Opcode::NEURAL_FWD:
        case t81::tisc::Opcode::NEURAL_BACK:
        case t81::tisc::Opcode::NEURAL_OPT:
        case t81::tisc::Opcode::NEURAL_ACT:
        case t81::tisc::Opcode::NEURAL_NORM:
        case t81::tisc::Opcode::NEURAL_DROP:
        case t81::tisc::Opcode::NEURAL_RES:
        case t81::tisc::Opcode::NEURAL_ATTN:
        case t81::tisc::Opcode::QUANT_TERN:
        case t81::tisc::Opcode::QUANT_PRUN:
        case t81::tisc::Opcode::QUANT_DIST:
        case t81::tisc::Opcode::QUANT_COMP:
        case t81::tisc::Opcode::QUANT_DECOMP:
        case t81::tisc::Opcode::QUANT_VERIFY:
        case t81::tisc::Opcode::QUANT_ADAPT:
        case t81::tisc::Opcode::QUANT_MIXED: {
          // Delegate to advanced AI integration if enabled
          trap = t81::vm::advanced_ai::execute_advanced_ai_opcode_if_enabled(insn, ctx);
          break;
        }
