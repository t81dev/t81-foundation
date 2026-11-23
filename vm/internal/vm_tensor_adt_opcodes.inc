// Extracted from vm/vm.cpp (Vector, runtime, ADT (Option/Result/Enum/Complex), SWAR, and core tensor opcodes (ChkShape..Assert)).
// Included directly inside Interpreter::step() switch (insn.opcode) to preserve zero-overhead dispatch.

        case t81::tisc::Opcode::ChkShape: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.b] != ValueTag::TensorHandle ||
              ctx.register_tags[insn.c] != ValueTag::ShapeHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* tensor = tensor_ptr(ctx.registers[insn.b]);
          const auto* expected = shape_ptr(ctx.registers[insn.c]);
          if (tensor == nullptr) {
            log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                             static_cast<int>(ctx.registers[insn.b]), "tensor handle access");
            trap = Trap::DecodeFault;
            break;
          }
          if (expected == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          bool match = tensor->shape() == *expected;
          set_reg(insn.a, match ? 1 : 0, ValueTag::Int);
          update_flags(ctx.registers[insn.a]);
          break;
        }

        // -----------------------------------------------------------------
        // RFC-0005 v0.4 vector helpers
        // -----------------------------------------------------------------
        case t81::tisc::Opcode::ReadIsaVersion: {
          if (!reg_ok(insn.a)) {
            trap = Trap::DecodeFault;
            break;
          }
          // TISC version 0.4 constant.
          set_reg(insn.a, 4, ValueTag::Int);
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::GcSafepoint: {
          // RFC-0006 §2.3 — Explicit GC safepoint.
          // Triggers a DGC cycle immediately; Axion policy may veto (SecurityFault).
          if (auto gc_trap = run_gc_cycle_("safepoint"); gc_trap.has_value()) {
            trap = gc_trap.value();
          }
          break;
        }
        case t81::tisc::Opcode::AgentInvoke: {
          // RFC-0015 §3.2 — tier-tagged agent behavior invocation.
          // Semantics are identical to Call but Axion observes every invocation
          // for tier-check and policy enforcement before execution proceeds.
          if (!reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          {
            // Emit Axion audit event before dispatching so policy can veto.
            t81::axion::Verdict av;
            av.kind = t81::axion::VerdictKind::Allow;
            av.reason = "agent-invoke";
            record_axion_event(insn.opcode, insn.b, static_cast<std::int64_t>(ctx.call_depth), av);
          }
          // Reuse Call dispatch: tier + ceiling checks + stack push + jump.
          std::size_t next_depth = ctx.call_depth + 1;
          while (next_depth > recursion_limit_for_tier(ctx.tier_status.current) &&
                 ctx.tier_status.current != t81::cog::TierId::Tier5) {
            if (!ensure_min_tier(
                    static_cast<t81::cog::TierId>(tier_rank(ctx.tier_status.current) + 1),
                    "agent-invoke")) {
              trap = Trap::TierFault;
              break;
            }
          }
          if (trap != Trap::None) break;
          if (next_depth > recursion_limit_for_tier(ctx.tier_status.current)) {
            std::ostringstream reason;
            reason << "TierFault agent-invoke recursion depth exceeded depth=" << next_depth
                   << " tier=" << static_cast<int>(ctx.tier_status.current)
                   << " limit=" << recursion_limit_for_tier(ctx.tier_status.current);
            record_tier_fault("agent-invoke-recursion-limit", reason.str(),
                              static_cast<std::int64_t>(next_depth));
            trap = Trap::TierFault;
            break;
          }
          if (ctx.call_depth >= kHardRecursionCeiling) {
            ++state_.contradiction_events;
            t81::axion::Verdict recursion_verdict;
            recursion_verdict.kind = t81::axion::VerdictKind::Deny;
            std::ostringstream reason;
            reason << t81::axion::reasons::kRecursionCeiling
                   << " agent-invoke depth=" << ctx.call_depth
                   << " limit=" << kHardRecursionCeiling;
            recursion_verdict.reason = reason.str();
            record_axion_event(insn.opcode, insn.b, static_cast<std::int64_t>(ctx.call_depth),
                               recursion_verdict);
            trap = Trap::SecurityFault;
            break;
          }
          auto target = ctx.registers[insn.b];
          if (!check_mem(insn.opcode, static_cast<int>(target), "agent-invoke", true)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (!push_stack(static_cast<std::int64_t>(ctx.pc), ValueTag::Int)) {
            log_bounds_fault(insn.opcode, MemorySegmentKind::Stack, static_cast<int>(ctx.sp),
                             "stack agent-invoke");
            trap = Trap::StackFault;
            break;
          }
          ++ctx.call_depth;
          ctx.pc = static_cast<std::size_t>(target);
          break;
        }
        case t81::tisc::Opcode::VAdd: {
          // VAdd RD, RS1, RS2 — elementwise add on tensor handles.
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
          auto* t1 = tensor_ptr(ctx.registers[insn.b]);
          auto* t2 = tensor_ptr(ctx.registers[insn.c]);
          if (!t1 || !t2) {
            trap = Trap::DecodeFault;
            break;
          }
          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "VAdd kernel execution"};
          record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), ctx.registers[insn.b],
                             verdict);
          auto computed =
              t81::vm::internal::tensor_vec_binary_checked(*t1, *t2, /*multiply=*/false);
          if (!computed) {
            trap = computed.error();
            break;
          }
          auto rh = alloc_tensor(std::move(*computed));
          if (!rh) {
            trap = rh.error();
            break;
          }
          ctx.registers[insn.a] = *rh;
          ctx.register_tags[insn.a] = ValueTag::TensorHandle;
          break;
        }
        case t81::tisc::Opcode::VFma: {
          // VFma RD, RS1, RS2 — RD = RS1 * RS2 + RD (fused multiply-accumulate).
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.a] != ValueTag::TensorHandle) {
            trap = Trap::TypeFault;
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
          auto* accum = tensor_ptr(ctx.registers[insn.a]);
          auto* t1 = tensor_ptr(ctx.registers[insn.b]);
          auto* t2 = tensor_ptr(ctx.registers[insn.c]);
          if (!accum || !t1 || !t2) {
            trap = Trap::DecodeFault;
            break;
          }
          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "VFma kernel execution"};
          record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), ctx.registers[insn.b],
                             verdict);
          auto computed = t81::vm::internal::tensor_vfma_checked(*accum, *t1, *t2);
          if (!computed) {
            trap = computed.error();
            break;
          }
          auto rh = alloc_tensor(std::move(*computed));
          if (!rh) {
            trap = rh.error();
            break;
          }
          ctx.registers[insn.a] = *rh;
          ctx.register_tags[insn.a] = ValueTag::TensorHandle;
          break;
        }
        case t81::tisc::Opcode::VLoad: {
          // VLoad RD, RS_SRC, RS_SHAPE — reshape RS_SRC tensor to the shape in RS_SHAPE.
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.c] != ValueTag::ShapeHandle) {
            trap = Trap::TypeFault;
            break;
          }
          if (auto res = promote_to_tensor(insn.b); !res) {
            trap = res.error();
            break;
          }
          auto* src = tensor_ptr(ctx.registers[insn.b]);
          const auto* new_shape = shape_ptr(ctx.registers[insn.c]);
          if (!src || !new_shape) {
            trap = Trap::DecodeFault;
            break;
          }
          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "VLoad kernel execution"};
          record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), ctx.registers[insn.b],
                             verdict);
          auto computed = t81::vm::internal::tensor_vload_checked(*src, *new_shape);
          if (!computed) {
            trap = computed.error();
            break;
          }
          auto rh = alloc_tensor(std::move(*computed));
          if (!rh) {
            trap = rh.error();
            break;
          }
          ctx.registers[insn.a] = *rh;
          ctx.register_tags[insn.a] = ValueTag::TensorHandle;
          break;
        }
        case t81::tisc::Opcode::VStore: {
          // VStore RD, RS_SRC, RS_SHAPE — shape-validated copy of RS_SRC.
          // Faults if RS_SRC shape != RS_SHAPE; stores validated copy handle in RD.
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.c] != ValueTag::ShapeHandle) {
            trap = Trap::TypeFault;
            break;
          }
          if (auto res = promote_to_tensor(insn.b); !res) {
            trap = res.error();
            break;
          }
          auto* src = tensor_ptr(ctx.registers[insn.b]);
          const auto* expected_shape = shape_ptr(ctx.registers[insn.c]);
          if (!src || !expected_shape) {
            trap = Trap::DecodeFault;
            break;
          }
          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "VStore kernel execution"};
          record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), ctx.registers[insn.b],
                             verdict);
          auto computed = t81::vm::internal::tensor_vstore_checked(*src, *expected_shape);
          if (!computed) {
            trap = computed.error();
            break;
          }
          auto rh = alloc_tensor(std::move(*computed));
          if (!rh) {
            trap = rh.error();
            break;
          }
          ctx.registers[insn.a] = *rh;
          ctx.register_tags[insn.a] = ValueTag::TensorHandle;
          break;
        }

        case t81::tisc::Opcode::MakeOptionSome: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          auto handle = intern_option(true, ctx.register_tags[insn.b], ctx.registers[insn.b]);
          ctx.registers[insn.a] = handle;
          ctx.register_tags[insn.a] = ValueTag::OptionHandle;
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::MakeOptionNone: {
          if (!reg_ok(insn.a)) {
            trap = Trap::DecodeFault;
            break;
          }
          auto handle = intern_option(false, ValueTag::Int, 0);
          ctx.registers[insn.a] = handle;
          ctx.register_tags[insn.a] = ValueTag::OptionHandle;
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::MakeResultOk: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          auto handle = intern_result(true, ctx.register_tags[insn.b], ctx.registers[insn.b]);
          ctx.registers[insn.a] = handle;
          ctx.register_tags[insn.a] = ValueTag::ResultHandle;
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::MakeResultErr: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          auto handle = intern_result(false, ctx.register_tags[insn.b], ctx.registers[insn.b]);
          ctx.registers[insn.a] = handle;
          ctx.register_tags[insn.a] = ValueTag::ResultHandle;
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::MakeEnumVariant: {
          if (!reg_ok(insn.a)) {
            trap = Trap::DecodeFault;
            break;
          }
          auto handle = intern_enum(static_cast<int>(insn.b), false, ValueTag::Int, 0);
          ctx.registers[insn.a] = handle;
          ctx.register_tags[insn.a] = ValueTag::EnumHandle;
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::MakeEnumVariantPayload: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (insn.c < 0) {
            trap = Trap::DecodeFault;
            break;
          }
          auto handle = intern_enum(static_cast<int>(insn.c), true, ctx.register_tags[insn.b],
                                    ctx.registers[insn.b]);
          ctx.registers[insn.a] = handle;
          ctx.register_tags[insn.a] = ValueTag::EnumHandle;
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::MakeComplex: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.b] != ValueTag::Int ||
              ctx.register_tags[insn.c] != ValueTag::Int) {
            trap = Trap::TypeFault;
            break;
          }
          auto handle = intern_complex(ctx.registers[insn.b], ctx.registers[insn.c]);
          ctx.registers[insn.a] = handle;
          ctx.register_tags[insn.a] = ValueTag::ComplexHandle;
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::OptionIsSome: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.b] != ValueTag::OptionHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* opt = option_ptr(ctx.registers[insn.b]);
          if (opt == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          set_reg(insn.a, opt->has_value ? 1 : 0, ValueTag::Int);
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::OptionUnwrap: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.b] != ValueTag::OptionHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* opt = option_ptr(ctx.registers[insn.b]);
          if (opt == nullptr || !opt->has_value) {
            trap = Trap::DecodeFault;
            break;
          }
          set_reg(insn.a, opt->payload, opt->payload_tag);
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::ResultIsOk: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.b] != ValueTag::ResultHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* res = result_ptr(ctx.registers[insn.b]);
          if (res == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          set_reg(insn.a, res->is_ok ? 1 : 0, ValueTag::Int);
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::ResultUnwrapOk: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.b] != ValueTag::ResultHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* res = result_ptr(ctx.registers[insn.b]);
          if (res == nullptr || !res->is_ok) {
            trap = Trap::DecodeFault;
            break;
          }
          set_reg(insn.a, res->payload, res->payload_tag);
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::ResultUnwrapErr: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.b] != ValueTag::ResultHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* res = result_ptr(ctx.registers[insn.b]);
          if (res == nullptr || res->is_ok) {
            trap = Trap::DecodeFault;
            break;
          }
          set_reg(insn.a, res->payload, res->payload_tag);
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::EnumIsVariant: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.b] != ValueTag::EnumHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* val = enum_ptr(ctx.registers[insn.b]);
          if (val == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          bool matches = (val->variant_id == insn.c);
          set_reg(insn.a, matches ? 1 : 0, ValueTag::Int);
          update_flags(ctx.registers[insn.a]);
          {
            t81::axion::Verdict verdict;
            verdict.kind = t81::axion::VerdictKind::Allow;
            std::ostringstream reason;
            const int guard_variant_id = static_cast<int>(insn.c);  // explicit cast
            const int guard_enum_id = t81::enum_meta::decode_enum_id(guard_variant_id);
            const int guard_local_variant = t81::enum_meta::decode_variant_id(guard_variant_id);
            const auto* meta = enum_metadata_for(guard_enum_id);
            const auto* variant_meta = variant_metadata(meta, guard_local_variant);
            reason << "enum guard";
            if (meta != nullptr) {
              reason << " enum=" << meta->name;
            }
            if (variant_meta != nullptr) {
              reason << " variant=" << variant_meta->name;
              if (variant_meta->payload.has_value()) {
                reason << " payload=" << *variant_meta->payload;
              }
            }
            reason << " match=" << (matches ? "pass" : "fail");
            verdict.reason = reason.str();
            record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.c), matches ? 1 : 0,
                               verdict);
          }
          break;
        }
        case t81::tisc::Opcode::EnumUnwrapPayload: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.b] != ValueTag::EnumHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* val = enum_ptr(ctx.registers[insn.b]);
          if (val == nullptr || !val->has_payload) {
            trap = Trap::DecodeFault;
            break;
          }
          set_reg(insn.a, val->payload, val->payload_tag);
          update_flags(ctx.registers[insn.a]);
          {
            t81::axion::Verdict verdict;
            verdict.kind = t81::axion::VerdictKind::Allow;
            std::ostringstream reason;
            const int global_variant_id = val->variant_id;
            const int enum_id = t81::enum_meta::decode_enum_id(global_variant_id);
            const int local_variant = t81::enum_meta::decode_variant_id(global_variant_id);
            const auto* meta = enum_metadata_for(enum_id);
            const auto* variant_meta = variant_metadata(meta, local_variant);
            reason << "enum payload";
            if (meta != nullptr) {
              reason << " enum=" << meta->name;
            }
            if (variant_meta != nullptr) {
              reason << " variant=" << variant_meta->name;
              if (variant_meta->payload.has_value()) {
                reason << " payload=" << *variant_meta->payload;
              }
            }
            verdict.reason = reason.str();
            record_axion_event(insn.opcode, static_cast<std::int32_t>(global_variant_id),
                               val->payload, verdict);
          }
          break;
        }
        case t81::tisc::Opcode::TVecAdd:
        case t81::tisc::Opcode::TVecMul: {
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
          auto* tensor_a = tensor_ptr(ctx.registers[insn.b]);
          if (tensor_a == nullptr) {
            log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                             static_cast<int>(ctx.registers[insn.b]), "tensor handle access");
            trap = Trap::DecodeFault;
            break;
          }
          auto* tensor_b = tensor_ptr(ctx.registers[insn.c]);
          if (tensor_b == nullptr) {
            log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                             static_cast<int>(ctx.registers[insn.c]), "tensor handle access");
            trap = Trap::DecodeFault;
            break;
          }
          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow,
                                      insn.opcode == t81::tisc::Opcode::TVecAdd
                                          ? "TVecAdd kernel execution"
                                          : "TVecMul kernel execution"};
          record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), ctx.registers[insn.b],
                             verdict);

          const bool multiply = insn.opcode == t81::tisc::Opcode::TVecMul;
          auto computed =
              t81::vm::internal::tensor_vec_binary_checked(*tensor_a, *tensor_b, multiply);
          if (!computed.has_value()) {
            trap = computed.error();
            break;
          }
          auto res_handle = alloc_tensor(std::move(*computed));
          if (!res_handle) {
            trap = res_handle.error();
            break;
          }
          ctx.registers[insn.a] = *res_handle;
          ctx.register_tags[insn.a] = ValueTag::TensorHandle;
          break;
        }
        case t81::tisc::Opcode::TVecSub: {
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
          auto* tensor_a = tensor_ptr(ctx.registers[insn.b]);
          if (tensor_a == nullptr) {
            log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                             static_cast<int>(ctx.registers[insn.b]), "tensor handle access");
            trap = Trap::DecodeFault;
            break;
          }
          auto* tensor_b = tensor_ptr(ctx.registers[insn.c]);
          if (tensor_b == nullptr) {
            log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                             static_cast<int>(ctx.registers[insn.c]), "tensor handle access");
            trap = Trap::DecodeFault;
            break;
          }
          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "TVecSub kernel execution"};
          record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), ctx.registers[insn.b],
                             verdict);
          auto computed = t81::vm::internal::tensor_vec_sub_checked(*tensor_a, *tensor_b);
          if (!computed.has_value()) {
            trap = computed.error();
            break;
          }
          auto res_handle = alloc_tensor(std::move(*computed));
          if (!res_handle) {
            trap = res_handle.error();
            break;
          }
          ctx.registers[insn.a] = *res_handle;
          ctx.register_tags[insn.a] = ValueTag::TensorHandle;
          break;
        }
        case t81::tisc::Opcode::TTranspose: {
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
            trap = Trap::ShapeFault;
            break;
          }
          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow,
                                      "TTranspose kernel execution"};
          record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.b), ctx.registers[insn.b],
                             verdict);
          auto computed = t81::vm::internal::tensor_transpose_checked(*tensor);
          if (!computed.has_value()) {
            trap = computed.error();
            break;
          }
          auto res_handle = alloc_tensor(std::move(*computed));
          if (!res_handle) {
            trap = res_handle.error();
            break;
          }
          ctx.registers[insn.a] = *res_handle;
          ctx.register_tags[insn.a] = ValueTag::TensorHandle;
          break;
        }
        case t81::tisc::Opcode::TNOT_SWAR: {
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
            log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                             static_cast<int>(ctx.registers[insn.b]), "tensor handle access");
            trap = Trap::DecodeFault;
            break;
          }
          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "TNOT_SWAR kernel execution"};
          record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), ctx.registers[insn.b],
                             verdict);
          auto computed = t81::vm::internal::tensor_swar_not_checked(*tensor);
          if (!computed.has_value()) {
            trap = computed.error();
            break;
          }
          auto res_handle = alloc_tensor(std::move(*computed));
          if (!res_handle) {
            trap = res_handle.error();
            break;
          }
          ctx.registers[insn.a] = *res_handle;
          ctx.register_tags[insn.a] = ValueTag::TensorHandle;
          break;
        }
        case t81::tisc::Opcode::TAND_SWAR:
        case t81::tisc::Opcode::TOR_SWAR: {
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
          auto* tensor_a = tensor_ptr(ctx.registers[insn.b]);
          if (tensor_a == nullptr) {
            log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                             static_cast<int>(ctx.registers[insn.b]), "tensor handle access");
            trap = Trap::DecodeFault;
            break;
          }
          auto* tensor_b = tensor_ptr(ctx.registers[insn.c]);
          if (tensor_b == nullptr) {
            log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                             static_cast<int>(ctx.registers[insn.c]), "tensor handle access");
            trap = Trap::DecodeFault;
            break;
          }
          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow,
                                      insn.opcode == t81::tisc::Opcode::TAND_SWAR
                                          ? "TAND_SWAR kernel execution"
                                          : "TOR_SWAR kernel execution"};
          record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), ctx.registers[insn.b],
                             verdict);
          auto computed = insn.opcode == t81::tisc::Opcode::TAND_SWAR
                              ? t81::vm::internal::tensor_swar_and_checked(*tensor_a, *tensor_b)
                              : t81::vm::internal::tensor_swar_or_checked(*tensor_a, *tensor_b);
          if (!computed.has_value()) {
            trap = computed.error();
            break;
          }
          auto res_handle = alloc_tensor(std::move(*computed));
          if (!res_handle) {
            trap = res_handle.error();
            break;
          }
          ctx.registers[insn.a] = *res_handle;
          ctx.register_tags[insn.a] = ValueTag::TensorHandle;
          break;
        }
        case t81::tisc::Opcode::TMatMul: {
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
          auto* tensor_a = tensor_ptr(ctx.registers[insn.b]);
          if (tensor_a == nullptr) {
            log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                             static_cast<int>(ctx.registers[insn.b]), "tensor handle access");
            trap = Trap::DecodeFault;
            break;
          }
          auto* tensor_b = tensor_ptr(ctx.registers[insn.c]);
          if (tensor_b == nullptr) {
            log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                             static_cast<int>(ctx.registers[insn.c]), "tensor handle access");
            trap = Trap::DecodeFault;
            break;
          }
          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "TMatMul kernel execution"};
          record_axion_event(insn.opcode, static_cast<int32_t>(insn.b), ctx.registers[insn.b],
                             verdict);
          std::string_view rhs_canon_hash;
          if (auto hit = tensor_canon_hashes_.find(ctx.registers[insn.c]);
              hit != tensor_canon_hashes_.end()) {
            rhs_canon_hash = hit->second;
          }
          auto computed =
              t81::vm::internal::tensor_matmul_checked_cached(*tensor_a, *tensor_b, rhs_canon_hash);
          if (!computed.has_value()) {
            log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor, 0, "TMatMul shape mismatch");
            trap = computed.error();
            break;
          }
          auto res_handle = alloc_tensor(std::move(*computed));
          if (!res_handle) {
            trap = res_handle.error();
            break;
          }
          ctx.registers[insn.a] = *res_handle;
          ctx.register_tags[insn.a] = ValueTag::TensorHandle;
          break;
        }
        case t81::tisc::Opcode::TTenDot: {
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
          if (ctx.register_tags[insn.b] != ValueTag::TensorHandle ||
              ctx.register_tags[insn.c] != ValueTag::TensorHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* tensor_a = tensor_ptr(ctx.registers[insn.b]);
          if (tensor_a == nullptr) {
            log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                             static_cast<int>(ctx.registers[insn.b]), "tensor handle access");
            trap = Trap::DecodeFault;
            break;
          }
          auto* tensor_b = tensor_ptr(ctx.registers[insn.c]);
          if (tensor_b == nullptr) {
            log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                             static_cast<int>(ctx.registers[insn.c]), "tensor handle access");
            trap = Trap::DecodeFault;
            break;
          }
          auto computed = t81::vm::internal::tensor_contract_dot_checked(*tensor_a, *tensor_b);
          if (!computed.has_value()) {
            trap = computed.error();
            break;
          }
          auto res_handle = alloc_tensor(std::move(*computed));
          if (!res_handle) {
            trap = res_handle.error();
            break;
          }
          ctx.registers[insn.a] = *res_handle;
          ctx.register_tags[insn.a] = ValueTag::TensorHandle;
          break;
        }
        case t81::tisc::Opcode::TGet: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (auto res = promote_to_tensor(insn.b); !res) {
            trap = res.error();
            break;
          }

          auto* tensor = tensor_ptr(ctx.registers[insn.b]);
          if (tensor == nullptr) {
            log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor,
                             static_cast<int>(ctx.registers[insn.b]), "tensor handle access");
            trap = Trap::DecodeFault;
            break;
          }

          if (ctx.register_tags[insn.c] != ValueTag::Int) {
            trap = Trap::TypeFault;
            break;
          }
          std::int64_t index = ctx.registers[insn.c];
          auto value = t81::vm::internal::tensor_get_checked(*tensor, index);
          if (!value.has_value()) {
            log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor, static_cast<int>(index),
                             "tensor index out of bounds");
            trap = value.error();
            break;
          }
          ctx.registers[insn.a] = alloc_float(static_cast<double>(*value));
          ctx.register_tags[insn.a] = ValueTag::FloatHandle;

          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "TGet kernel execution"};
          record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.b), ctx.registers[insn.b],
                             verdict);
          break;
        }
        case t81::tisc::Opcode::TNew: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.b] != ValueTag::Int) {
            trap = Trap::TypeFault;
            break;
          }
          std::int64_t size = ctx.registers[insn.b];
          auto tensor = t81::vm::internal::tensor_new_1d(size);
          if (!tensor.has_value()) {
            trap = Trap::BoundsFault;
            break;
          }
          auto res_handle = alloc_tensor(std::move(*tensor));
          if (!res_handle) {
            trap = res_handle.error();
            break;
          }
          ctx.registers[insn.a] = *res_handle;
          ctx.register_tags[insn.a] = ValueTag::TensorHandle;

          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "TNew"};
          record_axion_event(insn.opcode, static_cast<std::int32_t>(size), ctx.registers[insn.a],
                             verdict);
          break;
        }
        case t81::tisc::Opcode::TSet: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.a] != ValueTag::TensorHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* tensor = tensor_ptr(ctx.registers[insn.a]);
          if (tensor == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }

          if (ctx.register_tags[insn.b] != ValueTag::Int) {
            trap = Trap::TypeFault;
            break;
          }
          std::int64_t idx = ctx.registers[insn.b];

          float val = 0.0F;
          auto val_tag = ctx.register_tags[insn.c];
          if (val_tag == ValueTag::FloatHandle) {
            auto* ptr_val = float_ptr(ctx.registers[insn.c]);
            if (ptr_val) val = static_cast<float>(*ptr_val);
          } else if (val_tag == ValueTag::Int) {
            val = static_cast<float>(ctx.registers[insn.c]);
          } else {
            trap = Trap::TypeFault;
            break;
          }

          const auto write_kind = val_tag == ValueTag::FloatHandle
                                      ? t81::tensor_mutation::ScalarWriteKind::FloatValue
                                      : t81::tensor_mutation::ScalarWriteKind::IntValue;
          auto set_res = t81::vm::internal::tensor_set_checked(*tensor, idx, val, write_kind);
          if (!set_res.has_value()) {
            log_bounds_fault(insn.opcode, MemorySegmentKind::Tensor, static_cast<int>(idx),
                             "TSet OOB");
            trap = set_res.error();
            break;
          }
          tensor_canon_hashes_.erase(ctx.registers[insn.a]);
          t81::vm::internal::log_tensor_provenance(state_, state_.current_context, insn.opcode,
                                                   static_cast<std::size_t>(ctx.registers[insn.a]),
                                                   *tensor, "set");

          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "TSet"};
          record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.b), 0, verdict);
          break;
        }
        case t81::tisc::Opcode::TID: {
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
          auto res_handle = alloc_tensor(t81::vm::internal::tensor_identity_copy(*tensor));
          if (!res_handle) {
            trap = res_handle.error();
            break;
          }
          ctx.registers[insn.a] = *res_handle;
          ctx.register_tags[insn.a] = ValueTag::TensorHandle;

          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "TID (Identity/Copy)"};
          record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.b), ctx.registers[insn.a],
                             verdict);
          break;
        }
        case t81::tisc::Opcode::NSend: {
          if (!reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          t81::axion::Verdict verdict{t81::axion::VerdictKind::Deny,
                                      "Blocked: unimplemented async/network opcode"};
          record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.b), 0, verdict);
          trap = Trap::SecurityFault;
          break;
        }
        case t81::tisc::Opcode::NRecv: {
          if (!reg_ok(insn.a)) {
            trap = Trap::DecodeFault;
            break;
          }
          t81::axion::Verdict verdict{t81::axion::VerdictKind::Deny,
                                      "Blocked: unimplemented async/network opcode"};
          record_axion_event(insn.opcode, 0, 0, verdict);
          trap = Trap::SecurityFault;
          break;
        }
        case t81::tisc::Opcode::VWait: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          t81::axion::Verdict verdict{t81::axion::VerdictKind::Deny,
                                      "Blocked: unimplemented async/network opcode"};
          record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.b), 0, verdict);
          trap = Trap::SecurityFault;
          break;
        }
        case t81::tisc::Opcode::VYield: {
          if (!reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          t81::axion::Verdict verdict{t81::axion::VerdictKind::Deny,
                                      "Blocked: unimplemented async/network opcode"};
          record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.b), 0, verdict);
          trap = Trap::SecurityFault;
          break;
        }
        case t81::tisc::Opcode::TNorm: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          int t = clamp_trit(ctx.registers[insn.b]);
          set_reg(insn.a, t, ValueTag::Int);
          update_flags(t);
          break;
        }
        case t81::tisc::Opcode::Canon: {
          if (!reg_ok(insn.a)) {
            trap = Trap::DecodeFault;
            break;
          }
          // Canonicalize memory/register (placeholder logic)
          // In full implementation, this would enforce canonical representation
          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "Canon"};
          record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.a), ctx.registers[insn.a],
                             verdict);
          break;
        }
        case t81::tisc::Opcode::MemZero: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          std::size_t addr = static_cast<std::size_t>(ctx.registers[insn.a]);
          std::size_t size = static_cast<std::size_t>(ctx.registers[insn.b]);
          for (std::size_t i = 0; i < size; ++i) {
            if (!mem_ok(static_cast<int>(addr + i))) {
              trap = Trap::BoundsFault;
              break;
            }
            state_.memory[addr + i] = 0;
            state_.memory_tags[addr + i] = ValueTag::Int;
          }
          if (trap == Trap::None) {
            log_memory_segment_access(insn.opcode,
                                      t81::vm::internal::segment_for_address(state_, addr), addr,
                                      size, "MemZero");
          }
          break;
        }
        case t81::tisc::Opcode::Copy: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          std::size_t dst = static_cast<std::size_t>(ctx.registers[insn.a]);
          std::size_t src = static_cast<std::size_t>(ctx.registers[insn.b]);
          std::size_t size = static_cast<std::size_t>(ctx.registers[insn.c]);
          for (std::size_t i = 0; i < size; ++i) {
            if (!mem_ok(static_cast<int>(src + i)) || !mem_ok(static_cast<int>(dst + i))) {
              trap = Trap::BoundsFault;
              break;
            }
            // Check for overlap if needed, but TISC spec says COPY is guaranteed non-overlapping
            // usually, or we handle it safely.
          }
          if (trap == Trap::None) {
            // Perform copy
            for (std::size_t i = 0; i < size; ++i) {
              state_.memory[dst + i] = state_.memory[src + i];
              state_.memory_tags[dst + i] = state_.memory_tags[src + i];
            }
            log_memory_segment_access(insn.opcode,
                                      t81::vm::internal::segment_for_address(state_, dst), dst,
                                      size, "Copy");
          }
          break;
        }
        case t81::tisc::Opcode::Assert: {
          if (!reg_ok(insn.a)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.registers[insn.a] == 0) {
            t81::axion::Verdict verdict;
            verdict.kind = t81::axion::VerdictKind::Deny;
            verdict.reason = "ASSERT failed";
            record_axion_event(insn.opcode, static_cast<std::int32_t>(insn.a), 0, verdict);
            trap = Trap::AssertionFailed;
          }
          break;
        }
