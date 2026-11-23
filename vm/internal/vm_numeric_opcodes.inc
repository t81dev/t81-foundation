// Extracted from vm/vm.cpp (Numeric conversion, floating-point, transcendental, and exact fraction opcodes (I2F..FracDiv)).
// Included directly inside Interpreter::step() switch (insn.opcode) to preserve zero-overhead dispatch.

        case t81::tisc::Opcode::I2F: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (!is_integer_like_tag(ctx.register_tags[insn.b])) {
            trap = Trap::TypeFault;
            break;
          }
          double value = 0.0;
          if (ctx.register_tags[insn.b] == ValueTag::BigIntHandle) {
            auto* bigint = bigint_ptr(ctx.registers[insn.b]);
            if (!bigint) {
              trap = Trap::DecodeFault;
              break;
            }
            value = bigint->to_float<72, 9>().to_double();
          } else {
            value = static_cast<double>(ctx.registers[insn.b]);
          }
          ctx.registers[insn.a] = alloc_float(value);
          ctx.register_tags[insn.a] = ValueTag::FloatHandle;
          break;
        }
        case t81::tisc::Opcode::F2I: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.b] != ValueTag::FloatHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* ptr_val = float_ptr(ctx.registers[insn.b]);
          if (!ptr_val) {
            trap = Trap::DecodeFault;
            break;
          }
          ctx.registers[insn.a] = static_cast<std::int64_t>(*ptr_val);
          ctx.register_tags[insn.a] = ValueTag::Int;
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::I2Frac: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (!is_integer_like_tag(ctx.register_tags[insn.b])) {
            trap = Trap::TypeFault;
            break;
          }
          t81::T81Fraction frac;
          if (ctx.register_tags[insn.b] == ValueTag::BigIntHandle) {
            auto* bigint = bigint_ptr(ctx.registers[insn.b]);
            if (!bigint) {
              trap = Trap::DecodeFault;
              break;
            }
            frac = t81::T81Fraction(*bigint, t81::T81BigInt::one());
          } else {
            frac = t81::T81Fraction::from_int(ctx.registers[insn.b]);
          }
          ctx.registers[insn.a] = alloc_fraction(std::move(frac));
          ctx.register_tags[insn.a] = ValueTag::FractionHandle;
          break;
        }
        case t81::tisc::Opcode::Int2BigInt: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (!is_integer_like_tag(ctx.register_tags[insn.b])) {
            trap = Trap::TypeFault;
            break;
          }
          if (auto value =
                  bigint_from_integer_like(ctx.register_tags[insn.b], ctx.registers[insn.b]);
              value.has_value()) {
            if (auto op_trap = store_bigint_materialized(insn.a, std::move(*value));
                op_trap.has_value()) {
              trap = *op_trap;
            }
          } else {
            trap = Trap::DecodeFault;
          }
          break;
        }
        case t81::tisc::Opcode::Frac2I: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.b] != ValueTag::FractionHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* ptr_val = fraction_ptr(ctx.registers[insn.b]);
          if (!ptr_val || !t81::T81BigInt::is_one(ptr_val->den)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (auto op_trap = store_integer_result(insn.a, ptr_val->num); op_trap.has_value()) {
            trap = *op_trap;
          }
          break;
        }
        case t81::tisc::Opcode::FAdd:
        case t81::tisc::Opcode::FSub:
        case t81::tisc::Opcode::FMul:
        case t81::tisc::Opcode::FDiv: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.b] != ValueTag::FloatHandle ||
              ctx.register_tags[insn.c] != ValueTag::FloatHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* lhs = float_ptr(ctx.registers[insn.b]);
          auto* rhs = float_ptr(ctx.registers[insn.c]);
          if (lhs == nullptr || rhs == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          double result = 0.0;
          switch (insn.opcode) {
            case t81::tisc::Opcode::FAdd:
              result = *lhs + *rhs;
              break;
            case t81::tisc::Opcode::FSub:
              result = *lhs - *rhs;
              break;
            case t81::tisc::Opcode::FMul:
              result = *lhs * *rhs;
              break;
            case t81::tisc::Opcode::FDiv:
              if (*rhs == 0.0) {
                trap = Trap::DivisionFault;
                break;
              }
              result = *lhs / *rhs;
              break;
            default:
              break;
          }
          if (trap != Trap::None) {
            break;
          }
          ctx.registers[insn.a] = alloc_float(result);
          ctx.register_tags[insn.a] = ValueTag::FloatHandle;
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::FSin:
        case t81::tisc::Opcode::FCos:
        case t81::tisc::Opcode::FTan:
        case t81::tisc::Opcode::FAsin:
        case t81::tisc::Opcode::FAcos:
        case t81::tisc::Opcode::FAtan:
        case t81::tisc::Opcode::FSinh:
        case t81::tisc::Opcode::FCosh:
        case t81::tisc::Opcode::FTanh:
        case t81::tisc::Opcode::FSqrt:
        case t81::tisc::Opcode::FExp:
        case t81::tisc::Opcode::FLog:
        case t81::tisc::Opcode::FPow: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) ||
              (insn.opcode == t81::tisc::Opcode::FPow && !reg_ok(insn.c))) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.b] != ValueTag::FloatHandle) {
            trap = Trap::TypeFault;
            break;
          }
          if (insn.opcode == t81::tisc::Opcode::FPow &&
              ctx.register_tags[insn.c] != ValueTag::FloatHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* ptr_val = float_ptr(ctx.registers[insn.b]);
          if (!ptr_val) {
            trap = Trap::DecodeFault;
            break;
          }
          using VMFloat = t81::T81Float<72, 9>;
          const VMFloat input = VMFloat::from_double(*ptr_val);
          VMFloat out = VMFloat::zero();
          if (insn.opcode == t81::tisc::Opcode::FSin) {
            out = input.sin();
          } else if (insn.opcode == t81::tisc::Opcode::FCos) {
            out = input.cos();
          } else if (insn.opcode == t81::tisc::Opcode::FTan) {
            out = input.tan();
          } else if (insn.opcode == t81::tisc::Opcode::FAsin) {
            out = input.asin();
          } else if (insn.opcode == t81::tisc::Opcode::FAcos) {
            out = input.acos();
          } else if (insn.opcode == t81::tisc::Opcode::FAtan) {
            out = input.atan();
          } else if (insn.opcode == t81::tisc::Opcode::FSinh) {
            out = input.sinh();
          } else if (insn.opcode == t81::tisc::Opcode::FCosh) {
            out = input.cosh();
          } else if (insn.opcode == t81::tisc::Opcode::FTanh) {
            out = input.tanh();
          } else if (insn.opcode == t81::tisc::Opcode::FSqrt) {
            out = input.sqrt();
          } else if (insn.opcode == t81::tisc::Opcode::FExp) {
            out = input.exp();
          } else if (insn.opcode == t81::tisc::Opcode::FLog) {
            out = input.log();
          } else {
            auto* exponent = float_ptr(ctx.registers[insn.c]);
            if (!exponent) {
              trap = Trap::DecodeFault;
              break;
            }
            out = input.pow(VMFloat::from_double(*exponent));
          }
          ctx.registers[insn.a] = alloc_float(out.to_double());
          ctx.register_tags[insn.a] = ValueTag::FloatHandle;
          break;
        }
        case t81::tisc::Opcode::FracAdd:
        case t81::tisc::Opcode::FracSub:
        case t81::tisc::Opcode::FracMul:
        case t81::tisc::Opcode::FracDiv: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.b] != ValueTag::FractionHandle ||
              ctx.register_tags[insn.c] != ValueTag::FractionHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* lhs = fraction_ptr(ctx.registers[insn.b]);
          auto* rhs = fraction_ptr(ctx.registers[insn.c]);
          if (lhs == nullptr || rhs == nullptr) {
            trap = Trap::DecodeFault;
            break;
          }
          try {
            t81::T81Fraction result;
            switch (insn.opcode) {
              case t81::tisc::Opcode::FracAdd:
                result = t81::T81Fraction::add(*lhs, *rhs);
                break;
              case t81::tisc::Opcode::FracSub:
                result = t81::T81Fraction::sub(*lhs, *rhs);
                break;
              case t81::tisc::Opcode::FracMul:
                result = t81::T81Fraction::mul(*lhs, *rhs);
                break;
              case t81::tisc::Opcode::FracDiv:
                if (t81::T81BigInt::is_zero(rhs->num)) {
                  trap = Trap::DivisionFault;
                  break;
                }
                result = t81::T81Fraction::div(*lhs, *rhs);
                break;
              default:
                break;
            }
            if (trap != Trap::None) {
              break;
            }
            ctx.registers[insn.a] = alloc_fraction(std::move(result));
            ctx.register_tags[insn.a] = ValueTag::FractionHandle;
            update_flags(ctx.registers[insn.a]);
          } catch (...) {
            trap = Trap::DecodeFault;
          }
          break;
        }
