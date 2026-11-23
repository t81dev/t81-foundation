// Extracted from vm/vm.cpp (Tensor hash loading, metadata reflection, and LLM math opcodes (TLoadHash..TRoPE)).
// Included directly inside Interpreter::step() switch (insn.opcode) to preserve zero-overhead dispatch.

        case t81::tisc::Opcode::TLoadHash: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          auto symbol = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
          if (!symbol.has_value()) {
            trap = Trap::DecodeFault;
            break;
          }
          std::string hash_str = std::string(*symbol);

          auto verdict =
              eval_axion_call(t81::axion::reasons::kStep, current_pc, insn.opcode, hash_str);
          if (verdict.kind == t81::axion::VerdictKind::Deny) {
            record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), 0, verdict);
            trap = Trap::SecurityFault;
            break;
          }

          if (!canonfs_driver_) {
            // No CanonFS driver: validate hash format first, then treat as a miss.
            if (!t81::vm::internal::parse_canon_tensor_ref(hash_str).has_value()) {
              trap = Trap::DecodeFault;
              break;
            }
            t81::axion::Verdict miss_verdict;
            miss_verdict.kind = t81::axion::VerdictKind::Allow;
            miss_verdict.reason = "TLOADHASH canonfs_miss hash=" + hash_str;
            record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), 0, miss_verdict);
            trap = Trap::BoundsFault;
            break;
          }
          auto load_res = t81::vm::internal::load_canon_tensor_by_hash(*canonfs_driver_, hash_str);
          if (load_res.status == t81::vm::internal::TensorLoadHashStatus::InvalidHash) {
            trap = Trap::DecodeFault;
            break;
          }
          if (load_res.status == t81::vm::internal::TensorLoadHashStatus::CanonFsMiss) {
            t81::axion::Verdict miss_verdict;
            miss_verdict.kind = t81::axion::VerdictKind::Allow;
            miss_verdict.reason = "TLOADHASH canonfs_miss hash=" + hash_str;
            record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), 0, miss_verdict);
            trap = Trap::BoundsFault;
            break;
          }
          if (load_res.status != t81::vm::internal::TensorLoadHashStatus::Ok ||
              !load_res.tensor.has_value()) {
            trap = Trap::DecodeFault;
            break;
          }
          auto res = alloc_tensor(std::move(*load_res.tensor));
          if (!res) {
            trap = res.error();
            break;
          }
          ctx.registers[insn.a] = *res;
          ctx.register_tags[insn.a] = ValueTag::TensorHandle;
          tensor_canon_hashes_[*res] = hash_str;

          t81::axion::Verdict success_verdict;
          success_verdict.kind = t81::axion::VerdictKind::Allow;
          success_verdict.reason = "TLOADHASH success hash=" + hash_str +
                                   " handle=" + std::to_string(ctx.registers[insn.a]);
          record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), ctx.registers[insn.a],
                             success_verdict);
          break;
        }
        case t81::tisc::Opcode::TExp: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.b] == ValueTag::WeightsTensorHandle) {
            const auto* native = weights_tensor(ctx.registers[insn.b]);
            if (native == nullptr) {
              trap = Trap::DecodeFault;
              break;
            }
            if (auto fast = t81::vm::internal::native_tensor_unary_exp_direct(*native);
                fast.has_value()) {
              auto res_handle = alloc_tensor(std::move(*fast));
              if (!res_handle) {
                trap = res_handle.error();
                break;
              }
              ctx.registers[insn.a] = *res_handle;
              ctx.register_tags[insn.a] = ValueTag::TensorHandle;
              break;
            }
          }
          if (auto res = promote_to_tensor(insn.b); !res) {
            trap = res.error();
            break;
          }
          auto* tensor = tensor_ptr(ctx.registers[insn.b]);
          if (tensor == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          auto res_handle = alloc_tensor(t81::vm::internal::tensor_unary_exp(*tensor));
          if (!res_handle) {
            trap = res_handle.error();
            break;
          }
          ctx.registers[insn.a] = *res_handle;
          ctx.register_tags[insn.a] = ValueTag::TensorHandle;
          break;
        }
        case t81::tisc::Opcode::MetaRead: {
          if (!reg_ok(insn.a) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          MemorySegmentKind segment = static_cast<MemorySegmentKind>(insn.b);
          std::int64_t addr = ctx.registers[insn.c];
          auto verdict = eval_axion_call(t81::axion::reasons::kMetaRead, current_pc, insn.opcode);
          if (verdict.kind == t81::axion::VerdictKind::Deny) {
            trap = Trap::SecurityFault;
            break;
          }
          if (segment == MemorySegmentKind::Registers) {
            if (!reg_ok(static_cast<int>(addr))) {
              trap = Trap::BoundsFault;
              break;
            }
            ctx.registers[insn.a] = ctx.registers[addr];
            ctx.register_tags[insn.a] = ctx.register_tags[addr];
          } else if (segment == MemorySegmentKind::Code) {
            if (addr < 0 || static_cast<size_t>(addr) >= program_.insns.size()) {
              trap = Trap::BoundsFault;
              break;
            }
            ctx.registers[insn.a] = static_cast<std::int64_t>(program_.insns[addr].opcode);
            ctx.register_tags[insn.a] = ValueTag::Int;
          } else {
            std::size_t physical_addr = 0;
            bool ok = false;
            const auto& layout = state_.layout;
            switch (segment) {
              case MemorySegmentKind::Stack:
                if (layout.stack.contains(addr)) {
                  physical_addr = addr;
                  ok = true;
                }
                break;
              case MemorySegmentKind::Heap:
                if (layout.heap.contains(addr)) {
                  physical_addr = addr;
                  ok = true;
                }
                break;
              case MemorySegmentKind::Tensor:
                if (layout.tensor.contains(addr)) {
                  physical_addr = addr;
                  ok = true;
                }
                break;
              case MemorySegmentKind::Meta:
                if (layout.meta.contains(addr)) {
                  physical_addr = addr;
                  ok = true;
                }
                break;
              default:
                break;
            }
            if (!ok) {
              trap = Trap::BoundsFault;
              break;
            }
            ctx.registers[insn.a] = state_.memory[physical_addr];
            ctx.register_tags[insn.a] = state_.memory_tags[physical_addr];
          }
          update_flags(ctx.registers[insn.a]);
          t81::vm::internal::apply_segment_reason(verdict, "MetaRead reflection", segment,
                                                  static_cast<size_t>(addr));
          record_axion_event(insn.opcode, static_cast<int32_t>(segment), addr, verdict);
          break;
        }
        case t81::tisc::Opcode::MetaWrite: {
          if (!reg_ok(insn.a) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          MemorySegmentKind segment = static_cast<MemorySegmentKind>(insn.b);
          std::int64_t addr = ctx.registers[insn.c];
          std::int64_t val = ctx.registers[insn.a];
          ValueTag tag = ctx.register_tags[insn.a];
          auto verdict = eval_axion_call(t81::axion::reasons::kMetaWrite, current_pc, insn.opcode);
          if (verdict.kind == t81::axion::VerdictKind::Deny) {
            trap = Trap::SecurityFault;
            break;
          }
          if (segment == MemorySegmentKind::Registers) {
            if (!reg_ok(static_cast<int>(addr))) {
              trap = Trap::BoundsFault;
              break;
            }
            ctx.registers[addr] = val;
            ctx.register_tags[addr] = tag;
          } else if (segment == MemorySegmentKind::Code) {
            if (addr < 0 || static_cast<size_t>(addr) >= program_.insns.size()) {
              trap = Trap::BoundsFault;
              break;
            }
            // Code segment is protected: Writable only via privileged loader
            t81::axion::Verdict code_verdict;
            code_verdict.kind = t81::axion::VerdictKind::Deny;
            code_verdict.reason = "MetaWrite to Code segment denied (protected)";
            record_axion_event(insn.opcode, static_cast<int32_t>(segment), addr, code_verdict);
            trap = Trap::SecurityFault;
            break;
          } else {
            std::size_t physical_addr = 0;
            bool ok = false;
            const auto& layout = state_.layout;
            switch (segment) {
              case MemorySegmentKind::Stack:
                if (layout.stack.contains(addr)) {
                  physical_addr = addr;
                  ok = true;
                }
                break;
              case MemorySegmentKind::Heap:
                if (layout.heap.contains(addr)) {
                  physical_addr = addr;
                  ok = true;
                }
                break;
              case MemorySegmentKind::Tensor:
                if (layout.tensor.contains(addr)) {
                  physical_addr = addr;
                  ok = true;
                }
                break;
              case MemorySegmentKind::Meta:
                if (layout.meta.contains(addr)) {
                  physical_addr = addr;
                  ok = true;
                }
                break;
              default:
                break;
            }
            if (!ok) {
              trap = Trap::BoundsFault;
              break;
            }
            state_.memory[physical_addr] = val;
            state_.memory_tags[physical_addr] = tag;
          }
          t81::vm::internal::apply_segment_reason(verdict, "MetaWrite reflection", segment,
                                                  static_cast<size_t>(addr));
          record_axion_event(insn.opcode, static_cast<int32_t>(segment), addr, verdict);
          break;
        }
        case t81::tisc::Opcode::MetaReflect: {
          if (!reg_ok(insn.a)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (state_.reflection_count >= kMaxReflectionsPerEpoch) {
            trap = Trap::SecurityFault;
            break;
          }

          auto verdict =
              eval_axion_call(t81::axion::reasons::kMetaReflect, current_pc, insn.opcode);
          if (verdict.kind == t81::axion::VerdictKind::Deny) {
            trap = Trap::SecurityFault;
            break;
          }

          ReflectionSnapshot snapshot;
          snapshot.pc = current_pc;
          snapshot.registers = ctx.registers;
          snapshot.register_tags = ctx.register_tags;
          snapshot.flags = ctx.flags;

          // Capture recent trace (up to 81 entries)
          std::size_t trace_start = (state_.trace.size() > 81) ? (state_.trace.size() - 81) : 0;
          for (std::size_t i = trace_start; i < state_.trace.size(); ++i) {
            snapshot.recent_trace.push_back(state_.trace[i]);
          }

          // Improved hash of code segment including operands
          uint64_t h = 0;
          for (const auto& pi : program_.insns) {
            auto combine = [&](uint64_t v) { h ^= v + 0x9e3779b9 + (h << 6) + (h >> 2); };
            combine(static_cast<uint64_t>(pi.opcode));
            combine(static_cast<uint64_t>(pi.a));
            combine(static_cast<uint64_t>(pi.b));
            combine(static_cast<uint64_t>(pi.c));
          }
          snapshot.code_hash = h;

          state_.reflection_snapshots.push_back(std::move(snapshot));
          state_.reflection_count++;

          std::int64_t handle = static_cast<std::int64_t>(state_.reflection_snapshots.size());
          set_reg(insn.a, handle, ValueTag::ReflectionHandle);
          update_flags(handle);
          record_axion_event(insn.opcode, insn.b, handle, verdict);
          break;
        }
        case t81::tisc::Opcode::MetaRefine: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          // RS1 (insn.b) = memory address of commands
          // RS2 (insn.c) = number of commands

          auto verdict = eval_axion_call(t81::axion::reasons::kMetaRefine, current_pc, insn.opcode);
          if (verdict.kind == t81::axion::VerdictKind::Deny) {
            trap = Trap::SecurityFault;
            break;
          }

          std::int64_t cmd_addr = ctx.registers[insn.b];
          std::int64_t cmd_count = ctx.registers[insn.c];

          if (cmd_count < 0 || cmd_count > static_cast<int64_t>(kMaxMetaWritesPerEpoch)) {
            trap = Trap::BoundsFault;
            break;
          }

          // Read commands into a temporary list for all-or-nothing atomicity
          std::vector<RefinementCommand> commands;
          bool read_ok = true;
          for (int i = 0; i < cmd_count; ++i) {
            std::size_t base = static_cast<std::size_t>(cmd_addr + i * 4);
            if (!mem_ok(static_cast<int>(base)) || !mem_ok(static_cast<int>(base + 3))) {
              read_ok = false;
              break;
            }
            RefinementCommand cmd;
            cmd.op = static_cast<RefinementCommand::Op>(state_.memory[base]);
            cmd.target = state_.memory[base + 1];
            cmd.value = state_.memory[base + 2];
            cmd.tag = static_cast<ValueTag>(state_.memory[base + 3]);
            commands.push_back(cmd);
          }

          if (!read_ok) {
            trap = Trap::BoundsFault;
            break;
          }

          // VALIDATION PASS (Atomicity check)
          for (const auto& cmd : commands) {
            switch (cmd.op) {
              case RefinementCommand::Op::WriteCode:
                // Code segment is protected: Writable only via privileged loader
                {
                  t81::axion::Verdict code_verdict;
                  code_verdict.kind = t81::axion::VerdictKind::Deny;
                  code_verdict.reason = "MetaRefine WriteCode denied (protected)";
                  record_axion_event(insn.opcode, insn.b, 0, code_verdict);
                }
                trap = Trap::SecurityFault;
                break;
              case RefinementCommand::Op::WriteReg:
                if (!reg_ok(static_cast<int>(cmd.target))) {
                  trap = Trap::BoundsFault;
                  break;
                }
                break;
              case RefinementCommand::Op::WriteMem:
                if (!mem_ok(static_cast<int>(cmd.target))) {
                  trap = Trap::BoundsFault;
                  break;
                }
                break;
              case RefinementCommand::Op::Noop:
                break;
            }
            if (trap != Trap::None) {
              break;
            }
          }

          if (trap != Trap::None) {
            break;
          }

          // APPLICATION PASS
          for (const auto& cmd : commands) {
            switch (cmd.op) {
              case RefinementCommand::Op::WriteCode:
                // Unreachable if validation works, but kept for safety or reverted?
                // Reverting to original logic for consistency if we ever allow it via policy
                program_.insns[cmd.target].opcode = static_cast<t81::tisc::Opcode>(cmd.value);
                state_.meta_write_count++;
                compiled_traces_.clear();  // Invalidate JIT cache
                break;
              case RefinementCommand::Op::WriteReg:
                ctx.registers[cmd.target] = cmd.value;
                ctx.register_tags[cmd.target] = cmd.tag;
                break;
              case RefinementCommand::Op::WriteMem:
                state_.memory[cmd.target] = cmd.value;
                state_.memory_tags[cmd.target] = cmd.tag;
                break;
              case RefinementCommand::Op::Noop:
                break;
            }
          }

          set_reg(insn.a, 1, ValueTag::Int);  // Success
          update_flags(1);
          record_axion_event(insn.opcode, insn.b, 1, verdict);
          break;
        }
        case t81::tisc::Opcode::TSqrt: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (auto res = promote_to_tensor(insn.b); !res) {
            trap = res.error();
            break;
          }
          auto* tensor = tensor_ptr(ctx.registers[insn.b]);
          if (tensor == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          auto res_handle = alloc_tensor(t81::vm::internal::tensor_unary_sqrt(*tensor));
          if (!res_handle) {
            trap = res_handle.error();
            break;
          }
          ctx.registers[insn.a] = *res_handle;
          ctx.register_tags[insn.a] = ValueTag::TensorHandle;
          break;
        }
        case t81::tisc::Opcode::TSiLU: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.b] == ValueTag::WeightsTensorHandle) {
            const auto* native = weights_tensor(ctx.registers[insn.b]);
            if (native == nullptr) {
              trap = Trap::DecodeFault;
              break;
            }
            t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "TSiLU kernel execution"};
            record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.b),
                               ctx.registers[insn.b], verdict);
            if (auto fast = t81::vm::internal::native_tensor_unary_silu_direct(*native);
                fast.has_value()) {
              auto res_handle = alloc_tensor(std::move(*fast));
              if (!res_handle) {
                trap = res_handle.error();
                break;
              }
              ctx.registers[insn.a] = *res_handle;
              ctx.register_tags[insn.a] = ValueTag::TensorHandle;
              break;
            }
          }
          if (auto res = promote_to_tensor(insn.b); !res) {
            trap = res.error();
            break;
          }
          auto* tensor = tensor_ptr(ctx.registers[insn.b]);
          if (tensor == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "TSiLU kernel execution"};
          record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.b), ctx.registers[insn.b],
                             verdict);
          auto res_handle = alloc_tensor(t81::vm::internal::tensor_unary_silu(*tensor));
          if (!res_handle) {
            trap = res_handle.error();
            break;
          }
          ctx.registers[insn.a] = *res_handle;
          ctx.register_tags[insn.a] = ValueTag::TensorHandle;
          break;
        }
        case t81::tisc::Opcode::TSoftmax: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.b] == ValueTag::WeightsTensorHandle) {
            const auto* native = weights_tensor(ctx.registers[insn.b]);
            if (native == nullptr) {
              trap = Trap::DecodeFault;
              break;
            }
            t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow,
                                        "TSoftmax kernel execution"};
            record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.b),
                               ctx.registers[insn.b], verdict);
            if (auto fast = t81::vm::internal::native_tensor_unary_softmax_direct(*native);
                fast.has_value()) {
              auto res_handle = alloc_tensor(std::move(*fast));
              if (!res_handle) {
                trap = res_handle.error();
                break;
              }
              ctx.registers[insn.a] = *res_handle;
              ctx.register_tags[insn.a] = ValueTag::TensorHandle;
              break;
            }
          }
          if (auto res = promote_to_tensor(insn.b); !res) {
            trap = res.error();
            break;
          }
          auto* tensor = tensor_ptr(ctx.registers[insn.b]);
          if (tensor == nullptr || !t81::vm::internal::tensor_softmax_compatible(*tensor)) {
            trap = Trap::DecodeFault;
            break;
          }
          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "TSoftmax kernel execution"};
          record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.b), ctx.registers[insn.b],
                             verdict);
          auto res_handle = alloc_tensor(t81::vm::internal::tensor_unary_softmax(*tensor));
          if (!res_handle) {
            trap = res_handle.error();
            break;
          }
          ctx.registers[insn.a] = *res_handle;
          ctx.register_tags[insn.a] = ValueTag::TensorHandle;
          break;
        }
        case t81::tisc::Opcode::TRMSNorm: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (auto res = promote_to_tensor(insn.c); !res) {
            trap = res.error();
            break;
          }
          if (ctx.register_tags[insn.b] == ValueTag::WeightsTensorHandle) {
            const auto* native = weights_tensor(ctx.registers[insn.b]);
            auto* weights_tensor_ptr = tensor_ptr(ctx.registers[insn.c]);
            if (native != nullptr && weights_tensor_ptr != nullptr) {
              if (auto direct =
                      t81::vm::internal::native_tensor_rmsnorm_direct(*native, *weights_tensor_ptr);
                  direct.has_value()) {
                t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow,
                                            "TRMSNorm kernel execution"};
                record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.b),
                                   ctx.registers[insn.b], verdict);
                auto res_handle = alloc_tensor(std::move(*direct));
                if (!res_handle) {
                  trap = res_handle.error();
                  break;
                }
                ctx.registers[insn.a] = *res_handle;
                ctx.register_tags[insn.a] = ValueTag::TensorHandle;
                break;
              }
            }
          }
          if (auto res = promote_to_tensor(insn.b); !res) {
            trap = res.error();
            break;
          }
          auto* tensor = tensor_ptr(ctx.registers[insn.b]);
          auto* w = tensor_ptr(ctx.registers[insn.c]);
          if (tensor == nullptr || w == nullptr ||
              !t81::vm::internal::tensor_rmsnorm_compatible(*tensor, *w)) {
            log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor, 0, "TRMSNorm shape mismatch");
            trap = Trap::ShapeFault;
            break;
          }
          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "TRMSNorm kernel execution"};
          record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.b), ctx.registers[insn.b],
                             verdict);
          auto res_handle = alloc_tensor(t81::vm::internal::tensor_rmsnorm(*tensor, *w));
          if (!res_handle) {
            trap = res_handle.error();
            break;
          }
          ctx.registers[insn.a] = *res_handle;
          ctx.register_tags[insn.a] = ValueTag::TensorHandle;
          break;
        }
        case t81::tisc::Opcode::TRoPE: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.b] == ValueTag::WeightsTensorHandle) {
            const auto* native = weights_tensor(ctx.registers[insn.b]);
            if (native != nullptr) {
              const int pos = static_cast<int>(ctx.registers[insn.c]);
              if (auto direct = t81::vm::internal::native_tensor_rope_direct(*native, pos);
                  direct.has_value()) {
                t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow,
                                            "TRoPE kernel execution"};
                record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.b),
                                   ctx.registers[insn.b], verdict);
                auto res_handle = alloc_tensor(std::move(*direct));
                if (!res_handle) {
                  trap = res_handle.error();
                  break;
                }
                ctx.registers[insn.a] = *res_handle;
                ctx.register_tags[insn.a] = ValueTag::TensorHandle;
                break;
              }
            }
          }
          if (auto res = promote_to_tensor(insn.b); !res) {
            trap = res.error();
            break;
          }
          auto* tensor = tensor_ptr(ctx.registers[insn.b]);
          if (tensor == nullptr || !t81::vm::internal::tensor_rope_compatible(*tensor)) {
            log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor, 0, "TRoPE shape mismatch");
            trap = Trap::ShapeFault;
            break;
          }
          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "TRoPE kernel execution"};
          record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.b), ctx.registers[insn.b],
                             verdict);
          int pos = static_cast<int>(ctx.registers[insn.c]);
          auto res_handle = alloc_tensor(t81::vm::internal::tensor_rope(*tensor, pos));
          if (!res_handle) {
            trap = res_handle.error();
            break;
          }
          ctx.registers[insn.a] = *res_handle;
          ctx.register_tags[insn.a] = ValueTag::TensorHandle;
          break;
        }
