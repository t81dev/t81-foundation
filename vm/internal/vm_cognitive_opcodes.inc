// Extracted from vm/vm.cpp (Cognitive Tier 1-5 symbolic, reflection, infinite, and distributed opcodes (SymLoad..DistSeal)).
// Included directly inside Interpreter::step() switch (insn.opcode) to preserve zero-overhead dispatch.

        // Cognitive Tier Stubs
        case t81::tisc::Opcode::SymLoad: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          t81::cog::v1::SymbolicGraph graph;
          if (ctx.register_tags[insn.b] == ValueTag::SymbolHandle) {
            auto* sym = symbol_ptr(ctx.registers[insn.b]);
            if (sym) {
              graph.add_node(t81::cog::v1::SymbolicAtom::create(*sym));
            }
          }
          auto res_handle = alloc_symbolic_graph(std::move(graph));
          if (!res_handle) {
            trap = res_handle.error();
            break;
          }
          ctx.registers[insn.a] = *res_handle;
          ctx.register_tags[insn.a] = ValueTag::SymbolicGraphHandle;
          {
            telemetry.symbolic_rewrites += 1;
            t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "SymLoad"};
            record_axion_event(insn.opcode, 0, ctx.registers[insn.a], verdict);
          }
          break;
        }
        case t81::tisc::Opcode::SymRewrite: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.a] != ValueTag::SymbolicGraphHandle ||
              ctx.register_tags[insn.b] != ValueTag::SymbolHandle ||
              ctx.register_tags[insn.c] != ValueTag::SymbolHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* graph = symbolic_graph_ptr(ctx.registers[insn.a]);
          if (!graph) {
            trap = Trap::BoundsFault;
            break;
          }
          auto* match_str = symbol_ptr(ctx.registers[insn.b]);
          auto* replace_str = symbol_ptr(ctx.registers[insn.c]);

          if (match_str && replace_str) {
            t81::cog::v1::RewriteRule rule;
            rule.match_node = t81::T81Symbol::intern(*match_str);
            rule.replace_node = t81::T81Symbol::intern(*replace_str);
            std::size_t old_nodes = graph->nodes.size();
            graph->apply_rewrite(rule);
            std::size_t new_nodes = graph->nodes.size();
            if (new_nodes >= old_nodes) {
              state_.metrics.total_symbolic_nodes += (new_nodes - old_nodes);
            } else {
              state_.metrics.total_symbolic_nodes -= (old_nodes - new_nodes);
            }
          }
          {
            telemetry.symbolic_rewrites += 3;
            t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "SymRewrite"};
            record_axion_event(insn.opcode, 0, 0, verdict);
          }
          break;
        }
        case t81::tisc::Opcode::SymConfluence: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.b] != ValueTag::SymbolicGraphHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* graph = symbolic_graph_ptr(ctx.registers[insn.b]);
          bool conf = graph ? graph->is_confluent() : false;
          set_reg(insn.a, conf ? 1 : 0, ValueTag::Bool);
          {
            telemetry.symbolic_rewrites += 1;
            t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "SymConfluence"};
            record_axion_event(insn.opcode, 0, conf ? 1 : 0, verdict);
          }
          break;
        }
        case t81::tisc::Opcode::SymCanon: {
          if (!reg_ok(insn.a)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.a] != ValueTag::SymbolicGraphHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* graph = symbolic_graph_ptr(ctx.registers[insn.a]);
          if (graph) graph->canonicalize();
          {
            telemetry.symbolic_rewrites += 2;
            t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "SymCanon"};
            record_axion_event(insn.opcode, 0, 0, verdict);
          }
          break;
        }
        case t81::tisc::Opcode::SymBind: {
          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "SymBind"};
          record_axion_event(insn.opcode, 0, 0, verdict);
          break;
        }
        case t81::tisc::Opcode::ReflCap: {
          if (!ensure_min_tier(t81::cog::TierId::Tier2, "reflective-opcode")) {
            trap = Trap::TierFault;
            break;
          }
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          std::string description;
          if (ctx.register_tags[insn.b] == ValueTag::SymbolHandle) {
            auto* sym = symbol_ptr(ctx.registers[insn.b]);
            if (sym) description = *sym;
          }

          t81::cog::v2::ReflectiveFrame frame;
          // Make a copy of registers. std::array to std::vector.
          std::vector<std::int64_t> regs(ctx.registers.begin(), ctx.registers.end());
          frame.capture_state(current_pc, regs, description);

          ctx.tier2_frames.push_back(std::move(frame));
          ctx.registers[insn.a] = static_cast<std::int64_t>(ctx.tier2_frames.size());
          ctx.register_tags[insn.a] = ValueTag::Tier2FrameHandle;

          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow,
                                      frame.axion_trace_event("capture")};
          record_axion_event(insn.opcode, static_cast<std::int32_t>(ctx.tier2_frames.size()), 0,
                             verdict);
          break;
        }
        case t81::tisc::Opcode::ReflJustify: {
          if (!ensure_min_tier(t81::cog::TierId::Tier2, "reflective-opcode")) {
            trap = Trap::TierFault;
            break;
          }
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.a] != ValueTag::Tier2FrameHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* frame = tier2_frame_ptr(ctx.registers[insn.a]);
          if (!frame) {
            trap = Trap::BoundsFault;
            break;
          }
          std::string text;
          if (auto s = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
              s.has_value()) {
            text = *s;
          }
          std::vector<std::int64_t> regs_snapshot(ctx.registers.begin(), ctx.registers.end());
          frame->add_justification_step(text, current_pc, regs_snapshot);

          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow,
                                      frame->axion_trace_event("justify")};
          record_axion_event(insn.opcode, 0, 0, verdict);
          break;
        }
        case t81::tisc::Opcode::ReflCheck: {
          if (!ensure_min_tier(t81::cog::TierId::Tier2, "reflective-opcode")) {
            trap = Trap::TierFault;
            break;
          }
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.b] != ValueTag::Tier2FrameHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* frame = tier2_frame_ptr(ctx.registers[insn.b]);
          if (!frame) {
            trap = Trap::BoundsFault;
            break;
          }
          std::string criteria;
          if (auto s = symbol_like_text(ctx.register_tags[insn.c], ctx.registers[insn.c]);
              s.has_value()) {
            criteria = *s;
          } else {
            trap = Trap::TypeFault;
            break;
          }

          bool result = frame->check(criteria);
          set_reg(insn.a, result ? 1 : 0, ValueTag::Bool);
          update_flags(ctx.registers[insn.a]);

          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "ReflCheck"};
          record_axion_event(insn.opcode, 0, result, verdict);
          break;
        }
        case t81::tisc::Opcode::ReflTrace: {
          if (!ensure_min_tier(t81::cog::TierId::Tier2, "reflective-opcode")) {
            trap = Trap::TierFault;
            break;
          }
          if (!reg_ok(insn.a)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.a] != ValueTag::Tier2FrameHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* frame = tier2_frame_ptr(ctx.registers[insn.a]);
          if (!frame) {
            trap = Trap::BoundsFault;
            break;
          }
          // Capture current registers
          std::vector<int64_t> current_regs(ctx.registers.begin(), ctx.registers.end());
          frame->trace(current_pc, current_regs);

          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow,
                                      frame->axion_trace_event("trace")};
          record_axion_event(insn.opcode, 0, 0, verdict);
          break;
        }
        case t81::tisc::Opcode::ReflSeal: {
          if (!ensure_min_tier(t81::cog::TierId::Tier2, "reflective-opcode")) {
            trap = Trap::TierFault;
            break;
          }
          if (!reg_ok(insn.a)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.a] != ValueTag::Tier2FrameHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* frame = tier2_frame_ptr(ctx.registers[insn.a]);
          if (!frame) {
            trap = Trap::BoundsFault;
            break;
          }
          frame->seal();

          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow,
                                      frame->axion_trace_event("seal")};
          record_axion_event(insn.opcode, 0, static_cast<int64_t>(frame->hash), verdict);
          break;
        }
        case t81::tisc::Opcode::Recurse: {
          if (!ensure_min_tier(t81::cog::TierId::Tier3, "recurse-opcode")) {
            trap = Trap::TierFault;
            break;
          }
          ctx.tier3_recursor.max_depth =
              static_cast<int>(recursion_limit_for_tier(ctx.tier_status.current));
          if (!ctx.tier3_recursor.can_recurse()) {
            record_tier_fault("tier3-recursor-limit", "Tier 3 recursion limit exceeded",
                              static_cast<std::int64_t>(ctx.tier3_recursor.current_depth));
            trap = Trap::TierFault;
            break;
          }
          const double seed_entropy =
              static_cast<double>((state_.layout.stack.limit - ctx.sp) +
                                  (state_.heap_ptr - state_.layout.heap.start) + ctx.call_depth);
          t81::cog::v3::ContractionProof proof{true, seed_entropy, seed_entropy};
          if (!ctx.tier3_recursor.push_frame(proof)) {
            record_tier_fault("tier3-depth-proof", "Tier 3 depth proof rejected",
                              static_cast<std::int64_t>(ctx.tier3_recursor.current_depth));
            trap = Trap::TierFault;
            break;
          }
          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "Recurse: depth increased"};
          record_axion_event(insn.opcode, 0,
                             static_cast<std::int64_t>(ctx.tier3_recursor.current_depth), verdict);
          break;
        }
        case t81::tisc::Opcode::Contract: {
          if (!ensure_min_tier(t81::cog::TierId::Tier3, "contract-opcode")) {
            trap = Trap::TierFault;
            break;
          }
          if (!reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          // Read entropy value (assumed integer or float handle?)
          double current_entropy = 0.0;
          if (ctx.register_tags[insn.b] == ValueTag::Int) {
            current_entropy = static_cast<double>(ctx.registers[insn.b]);
          } else if (ctx.register_tags[insn.b] == ValueTag::FloatHandle) {
            auto* ptr = float_ptr(ctx.registers[insn.b]);
            if (ptr) current_entropy = *ptr;
          }

          if (!ctx.tier3_recursor.contract_top(current_entropy)) {
            record_tier_fault("tier3-contraction", "Contract: non-contractive entropy update",
                              static_cast<std::int64_t>(current_entropy));
            trap = Trap::TierFault;
            break;
          }

          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow,
                                      "Contract: entropy contracted"};
          record_axion_event(insn.opcode, 0, static_cast<std::int64_t>(current_entropy), verdict);
          break;
        }
        case t81::tisc::Opcode::Entropy: {
          if (!ensure_min_tier(t81::cog::TierId::Tier3, "entropy-opcode")) {
            trap = Trap::TierFault;
            break;
          }
          if (!reg_ok(insn.a)) {
            trap = Trap::DecodeFault;
            break;
          }
          // Calculate system entropy (stack usage + heap usage + depth)
          std::int64_t entropy = (state_.layout.stack.limit - ctx.sp) +
                                 (state_.heap_ptr - state_.layout.heap.start) + ctx.call_depth;
          set_reg(insn.a, entropy, ValueTag::Int);
          update_flags(entropy);
          break;
        }
        case t81::tisc::Opcode::Depth: {
          if (!ensure_min_tier(t81::cog::TierId::Tier3, "depth-opcode")) {
            trap = Trap::TierFault;
            break;
          }
          if (!reg_ok(insn.a)) {
            trap = Trap::DecodeFault;
            break;
          }
          std::int64_t depth = static_cast<std::int64_t>(ctx.tier3_recursor.current_depth);
          set_reg(insn.a, depth, ValueTag::Int);
          update_flags(depth);
          break;
        }
        case t81::tisc::Opcode::Terminate: {
          if (!ensure_min_tier(t81::cog::TierId::Tier3, "terminate-opcode")) {
            trap = Trap::TierFault;
            break;
          }
          ctx.tier3_recursor.pop_frame();
          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "Terminate: depth decreased"};
          record_axion_event(insn.opcode, 0,
                             static_cast<std::int64_t>(ctx.tier3_recursor.current_depth), verdict);
          break;
        }
        case t81::tisc::Opcode::InfSeed: {
          if (!ensure_min_tier(t81::cog::TierId::Tier5, "infinite-opcode")) {
            trap = Trap::TierFault;
            break;
          }
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }

          t81::T81Fraction start_val;
          if (ctx.register_tags[insn.b] == ValueTag::Int) {
            start_val = t81::T81Fraction::from_int(ctx.registers[insn.b]);
          } else if (ctx.register_tags[insn.b] == ValueTag::FractionHandle) {
            auto* f = fraction_ptr(ctx.registers[insn.b]);
            if (f) start_val = *f;
          } else {
            trap = Trap::TypeFault;
            break;
          }

          t81::cog::v5::InfiniteCanonicalForm form;
          form.first_term = start_val;
          form.type = t81::cog::v5::SeriesType::Geometric;
          form.seed_lazy_prefix();

          auto res = alloc_infinite_form(std::move(form));
          if (!res) {
            trap = res.error();
            break;
          }
          ctx.registers[insn.a] = *res;
          ctx.register_tags[insn.a] = ValueTag::InfiniteHandle;

          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "InfSeed"};
          record_axion_event(insn.opcode, 0, ctx.registers[insn.a], verdict);
          break;
        }
        case t81::tisc::Opcode::InfExpand: {
          if (!ensure_min_tier(t81::cog::TierId::Tier5, "infinite-opcode")) {
            trap = Trap::TierFault;
            break;
          }
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.a] != ValueTag::InfiniteHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* form = infinite_form_ptr(ctx.registers[insn.a]);
          if (!form) {
            trap = Trap::BoundsFault;
            break;
          }

          t81::T81Fraction ratio_val;
          if (ctx.register_tags[insn.b] == ValueTag::Int) {
            ratio_val = t81::T81Fraction::from_int(ctx.registers[insn.b]);
          } else if (ctx.register_tags[insn.b] == ValueTag::FractionHandle) {
            auto* f = fraction_ptr(ctx.registers[insn.b]);
            if (f) ratio_val = *f;
          } else {
            trap = Trap::TypeFault;
            break;
          }

          form->ratio = ratio_val;
          form->expand_lazy(1);

          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "InfExpand"};
          record_axion_event(insn.opcode, 0, ctx.registers[insn.a], verdict);
          break;
        }
        case t81::tisc::Opcode::InfCollapse: {
          if (!ensure_min_tier(t81::cog::TierId::Tier5, "infinite-opcode")) {
            trap = Trap::TierFault;
            break;
          }
          if (!reg_ok(insn.a)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.a] != ValueTag::InfiniteHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* form = infinite_form_ptr(ctx.registers[insn.a]);
          if (!form) {
            trap = Trap::BoundsFault;
            break;
          }

          form->collapse();

          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "InfCollapse"};
          record_axion_event(insn.opcode, 0, ctx.registers[insn.a], verdict);
          break;
        }
        case t81::tisc::Opcode::InfConverge: {
          if (!ensure_min_tier(t81::cog::TierId::Tier5, "infinite-opcode")) {
            trap = Trap::TierFault;
            break;
          }
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.b] != ValueTag::InfiniteHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* form = infinite_form_ptr(ctx.registers[insn.b]);
          if (!form) {
            trap = Trap::BoundsFault;
            break;
          }

          set_reg(insn.a, form->is_convergent ? 1 : 0, ValueTag::Bool);
          update_flags(ctx.registers[insn.a]);

          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "InfConverge"};
          record_axion_event(insn.opcode, 0, form->is_convergent, verdict);
          break;
        }
        case t81::tisc::Opcode::InfSignature: {
          if (!ensure_min_tier(t81::cog::TierId::Tier5, "infinite-opcode")) {
            trap = Trap::TierFault;
            break;
          }
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.b] != ValueTag::InfiniteHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* form = infinite_form_ptr(ctx.registers[insn.b]);
          if (!form) {
            trap = Trap::BoundsFault;
            break;
          }

          auto sig = t81::cog::v5::CollapseSignature::generate(*form);
          ctx.registers[insn.a] = intern_symbol(sig.hash);
          ctx.register_tags[insn.a] = ValueTag::SymbolHandle;

          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "InfSignature"};
          record_axion_event(insn.opcode, 0, ctx.registers[insn.a], verdict);
          break;
        }
        case t81::tisc::Opcode::Gossip: {
          if (!ensure_min_tier(t81::cog::TierId::Tier4, "distributed-opcode")) {
            trap = Trap::TierFault;
            break;
          }
          if (!reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          std::int64_t val = ctx.registers[insn.b];
          std::int32_t tag = static_cast<std::int32_t>(ctx.register_tags[insn.b]);
          state_.tier4_state.gossip(val, tag, instruction_count_);
          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "Gossip: broadcast state"};
          record_axion_event(insn.opcode, tag, val, verdict);
          break;
        }
        case t81::tisc::Opcode::Merge: {
          if (!ensure_min_tier(t81::cog::TierId::Tier4, "distributed-opcode")) {
            trap = Trap::TierFault;
            break;
          }
          if (!reg_ok(insn.a)) {
            trap = Trap::DecodeFault;
            break;
          }
          auto msg = state_.tier4_state.merge();
          std::int64_t handle = 0;
          if (msg) {
            handle = intern_option(true, static_cast<ValueTag>(msg->tag), msg->payload);
          } else {
            handle = intern_option(false, ValueTag::Int, 0);
          }
          set_reg(insn.a, handle, ValueTag::OptionHandle);
          update_flags(handle);

          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "Merge: process inbox"};
          record_axion_event(insn.opcode, msg ? 1 : 0, handle, verdict);
          break;
        }
        case t81::tisc::Opcode::TickSync: {
          if (!ensure_min_tier(t81::cog::TierId::Tier4, "distributed-opcode")) {
            trap = Trap::TierFault;
            break;
          }
          if (!reg_ok(insn.a)) {
            trap = Trap::DecodeFault;
            break;
          }
          // Operand A contains remote tick
          uint64_t remote = static_cast<uint64_t>(ctx.registers[insn.a]);
          state_.tier4_state.sync_tick(remote);
          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "TickSync"};
          record_axion_event(insn.opcode, 0,
                             static_cast<int64_t>(state_.tier4_state.vector.global_tick), verdict);
          break;
        }
        case t81::tisc::Opcode::Coherence: {
          if (!ensure_min_tier(t81::cog::TierId::Tier4, "distributed-opcode")) {
            trap = Trap::TierFault;
            break;
          }
          if (!reg_ok(insn.a)) {
            trap = Trap::DecodeFault;
            break;
          }
          // Returns coherence status (e.g., drift from global tick)
          int64_t drift = static_cast<int64_t>(state_.tier4_state.vector.global_tick) -
                          static_cast<int64_t>(instruction_count_);
          set_reg(insn.a, drift, ValueTag::Int);
          update_flags(drift);
          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "Coherence check"};
          record_axion_event(insn.opcode, 0, drift, verdict);
          break;
        }
        case t81::tisc::Opcode::DistSeal: {
          if (!reg_ok(insn.a)) {
            trap = Trap::DecodeFault;
            break;
          }
          // "Seal" the distributed state (e.g. generate a hash of outbox/inbox state)
          // For simulation, we just sum up outbox payloads
          int64_t seal = 0;
          for (const auto& m : state_.tier4_state.outbox) {
            seal ^= m.payload;
          }
          set_reg(insn.a, seal, ValueTag::Int);
          update_flags(seal);
          t81::axion::Verdict verdict{t81::axion::VerdictKind::Allow, "DistSeal"};
          record_axion_event(insn.opcode, 0, seal, verdict);
          break;
        }
