// Extracted from vm/vm.cpp (String and vector intrinsics (StrLen..StrJoin)).
// Included directly inside Interpreter::step() switch (insn.opcode) to preserve zero-overhead dispatch.

        case t81::tisc::Opcode::StrLen: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          auto symbol = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
          if (!symbol.has_value()) {
            trap = Trap::DecodeFault;
            break;
          }
          ctx.registers[insn.a] = static_cast<std::int64_t>(symbol->size());
          ctx.register_tags[insn.a] = ValueTag::Int;
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::StrEmpty: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          auto symbol = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
          if (!symbol.has_value()) {
            trap = Trap::DecodeFault;
            break;
          }
          ctx.registers[insn.a] = symbol->empty() ? 1 : 0;
          ctx.register_tags[insn.a] = ValueTag::Bool;
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::VecLen: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          std::int64_t length = 0;
          if (ctx.register_tags[insn.b] == ValueTag::StringVectorHandle) {
            auto* values = string_vector_ptr(ctx.registers[insn.b]);
            if (values == nullptr) {
              trap = Trap::DecodeFault;
              break;
            }
            length = static_cast<std::int64_t>(values->size());
          } else if (ctx.register_tags[insn.b] == ValueTag::TensorHandle) {
            auto* tensor = tensor_ptr(ctx.registers[insn.b]);
            if (tensor == nullptr) {
              trap = Trap::DecodeFault;
              break;
            }
            if (tensor->rank() != 1 || tensor->shape().empty()) {
              trap = Trap::TypeFault;
              break;
            }
            length = static_cast<std::int64_t>(tensor->shape().front());
          } else {
            trap = Trap::TypeFault;
            break;
          }
          ctx.registers[insn.a] = length;
          ctx.register_tags[insn.a] = ValueTag::Int;
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::VecEmpty: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          bool is_empty = false;
          if (ctx.register_tags[insn.b] == ValueTag::StringVectorHandle) {
            auto* values = string_vector_ptr(ctx.registers[insn.b]);
            if (values == nullptr) {
              trap = Trap::DecodeFault;
              break;
            }
            is_empty = values->empty();
          } else if (ctx.register_tags[insn.b] == ValueTag::TensorHandle) {
            auto* tensor = tensor_ptr(ctx.registers[insn.b]);
            if (tensor == nullptr) {
              trap = Trap::DecodeFault;
              break;
            }
            if (tensor->rank() != 1 || tensor->shape().empty()) {
              trap = Trap::TypeFault;
              break;
            }
            is_empty = tensor->shape().front() == 0;
          } else {
            trap = Trap::TypeFault;
            break;
          }
          ctx.registers[insn.a] = is_empty ? 1 : 0;
          ctx.register_tags[insn.a] = ValueTag::Bool;
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::VecFirst: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.b] == ValueTag::StringVectorHandle) {
            auto* values = string_vector_ptr(ctx.registers[insn.b]);
            if (values == nullptr) {
              trap = Trap::DecodeFault;
              break;
            }
            if (values->empty()) {
              trap = Trap::TypeFault;
              break;
            }
            ctx.registers[insn.a] = intern_symbol(values->front());
            ctx.register_tags[insn.a] = ValueTag::SymbolHandle;
            update_flags(ctx.registers[insn.a]);
            break;
          }
          if (ctx.register_tags[insn.b] == ValueTag::TensorHandle) {
            auto* tensor = tensor_ptr(ctx.registers[insn.b]);
            if (tensor == nullptr) {
              trap = Trap::DecodeFault;
              break;
            }
            if (tensor->rank() != 1 || tensor->shape().empty() || tensor->shape().front() <= 0) {
              trap = Trap::TypeFault;
              break;
            }
            const auto& data = tensor->data();
            if (data.empty()) {
              trap = Trap::TypeFault;
              break;
            }
            ctx.registers[insn.a] = static_cast<std::int64_t>(data.front());
            ctx.register_tags[insn.a] = ValueTag::Int;
            update_flags(ctx.registers[insn.a]);
            break;
          }
          trap = Trap::TypeFault;
          break;
        }
        case t81::tisc::Opcode::VecLast: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.b] == ValueTag::StringVectorHandle) {
            auto* values = string_vector_ptr(ctx.registers[insn.b]);
            if (values == nullptr) {
              trap = Trap::DecodeFault;
              break;
            }
            if (values->empty()) {
              trap = Trap::TypeFault;
              break;
            }
            ctx.registers[insn.a] = intern_symbol(values->back());
            ctx.register_tags[insn.a] = ValueTag::SymbolHandle;
            update_flags(ctx.registers[insn.a]);
            break;
          }
          if (ctx.register_tags[insn.b] == ValueTag::TensorHandle) {
            auto* tensor = tensor_ptr(ctx.registers[insn.b]);
            if (tensor == nullptr) {
              trap = Trap::DecodeFault;
              break;
            }
            if (tensor->rank() != 1 || tensor->shape().empty() || tensor->shape().front() <= 0) {
              trap = Trap::TypeFault;
              break;
            }
            const auto& data = tensor->data();
            if (data.empty()) {
              trap = Trap::TypeFault;
              break;
            }
            ctx.registers[insn.a] = static_cast<std::int64_t>(data.back());
            ctx.register_tags[insn.a] = ValueTag::Int;
            update_flags(ctx.registers[insn.a]);
            break;
          }
          trap = Trap::TypeFault;
          break;
        }
        case t81::tisc::Opcode::VecPush: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.b] == ValueTag::StringVectorHandle) {
            if (ctx.register_tags[insn.c] != ValueTag::SymbolHandle) {
              trap = Trap::TypeFault;
              break;
            }
            auto* values = string_vector_ptr(ctx.registers[insn.b]);
            auto* value = symbol_ptr(ctx.registers[insn.c]);
            if (values == nullptr || value == nullptr) {
              trap = Trap::DecodeFault;
              break;
            }
            std::vector<std::string> pushed = *values;
            pushed.push_back(*value);
            state_.string_vectors.push_back(std::move(pushed));
            ctx.registers[insn.a] = static_cast<std::int64_t>(state_.string_vectors.size());
            ctx.register_tags[insn.a] = ValueTag::StringVectorHandle;
            update_flags(ctx.registers[insn.a]);
            break;
          }
          if (ctx.register_tags[insn.b] == ValueTag::TensorHandle) {
            if (ctx.register_tags[insn.c] != ValueTag::Int) {
              trap = Trap::TypeFault;
              break;
            }
            auto* tensor = tensor_ptr(ctx.registers[insn.b]);
            if (tensor == nullptr) {
              trap = Trap::DecodeFault;
              break;
            }
            if (tensor->rank() != 1 || tensor->shape().empty()) {
              trap = Trap::TypeFault;
              break;
            }
            const auto old_len = tensor->shape().front();
            if (old_len < 0) {
              trap = Trap::TypeFault;
              break;
            }
            auto data = tensor->data();
            data.push_back(static_cast<float>(ctx.registers[insn.c]));
            auto res_handle = alloc_tensor(T729DynamicTensor({old_len + 1}, std::move(data)));
            if (!res_handle) {
              trap = res_handle.error();
              break;
            }
            ctx.registers[insn.a] = *res_handle;
            ctx.register_tags[insn.a] = ValueTag::TensorHandle;
            update_flags(ctx.registers[insn.a]);
            break;
          }
          trap = Trap::TypeFault;
          break;
        }
        case t81::tisc::Opcode::VecPop: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.b] == ValueTag::StringVectorHandle) {
            auto* values = string_vector_ptr(ctx.registers[insn.b]);
            if (values == nullptr) {
              trap = Trap::DecodeFault;
              break;
            }
            if (values->empty()) {
              trap = Trap::TypeFault;
              break;
            }
            std::vector<std::string> popped = *values;
            popped.pop_back();
            state_.string_vectors.push_back(std::move(popped));
            ctx.registers[insn.a] = static_cast<std::int64_t>(state_.string_vectors.size());
            ctx.register_tags[insn.a] = ValueTag::StringVectorHandle;
            update_flags(ctx.registers[insn.a]);
            break;
          }
          if (ctx.register_tags[insn.b] == ValueTag::TensorHandle) {
            auto* tensor = tensor_ptr(ctx.registers[insn.b]);
            if (tensor == nullptr) {
              trap = Trap::DecodeFault;
              break;
            }
            if (tensor->rank() != 1 || tensor->shape().empty() || tensor->shape().front() <= 0) {
              trap = Trap::TypeFault;
              break;
            }
            auto data = tensor->data();
            if (data.empty()) {
              trap = Trap::TypeFault;
              break;
            }
            data.pop_back();
            auto res_handle =
                alloc_tensor(T729DynamicTensor({tensor->shape().front() - 1}, std::move(data)));
            if (!res_handle) {
              trap = res_handle.error();
              break;
            }
            ctx.registers[insn.a] = *res_handle;
            ctx.register_tags[insn.a] = ValueTag::TensorHandle;
            update_flags(ctx.registers[insn.a]);
            break;
          }
          trap = Trap::TypeFault;
          break;
        }
        case t81::tisc::Opcode::StrConcat: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          auto lhs = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
          auto rhs = symbol_like_text(ctx.register_tags[insn.c], ctx.registers[insn.c]);
          if (!lhs.has_value() || !rhs.has_value()) {
            trap = Trap::DecodeFault;
            break;
          }
          std::string combined(*lhs);
          combined += std::string(*rhs);
          ctx.registers[insn.a] = intern_symbol(std::move(combined));
          ctx.register_tags[insn.a] = ValueTag::SymbolHandle;
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::StrStartsWith: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          auto value = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
          auto prefix = symbol_like_text(ctx.register_tags[insn.c], ctx.registers[insn.c]);
          if (!value.has_value() || !prefix.has_value()) {
            trap = Trap::DecodeFault;
            break;
          }
          const bool match =
              value->size() >= prefix->size() && value->compare(0, prefix->size(), *prefix) == 0;
          ctx.registers[insn.a] = match ? 1 : 0;
          ctx.register_tags[insn.a] = ValueTag::Bool;
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::StrEndsWith: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          auto value = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
          auto suffix = symbol_like_text(ctx.register_tags[insn.c], ctx.registers[insn.c]);
          if (!value.has_value() || !suffix.has_value()) {
            trap = Trap::DecodeFault;
            break;
          }
          const bool match =
              value->size() >= suffix->size() &&
              value->compare(value->size() - suffix->size(), suffix->size(), *suffix) == 0;
          ctx.registers[insn.a] = match ? 1 : 0;
          ctx.register_tags[insn.a] = ValueTag::Bool;
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::StrContains: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          auto value = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
          auto needle = symbol_like_text(ctx.register_tags[insn.c], ctx.registers[insn.c]);
          if (!value.has_value() || !needle.has_value()) {
            trap = Trap::DecodeFault;
            break;
          }
          const bool contains = value->find(*needle) != std::string::npos;
          ctx.registers[insn.a] = contains ? 1 : 0;
          ctx.register_tags[insn.a] = ValueTag::Bool;
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::StrIndexOf: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          auto value = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
          auto needle = symbol_like_text(ctx.register_tags[insn.c], ctx.registers[insn.c]);
          if (!value.has_value() || !needle.has_value()) {
            trap = Trap::DecodeFault;
            break;
          }
          const std::size_t pos = value->find(*needle);
          ctx.registers[insn.a] = pos == std::string::npos ? -1 : static_cast<std::int64_t>(pos);
          ctx.register_tags[insn.a] = ValueTag::Int;
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::StrReplace: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          auto source = symbol_like_text(ctx.register_tags[insn.a], ctx.registers[insn.a]);
          auto needle = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
          auto replacement = symbol_like_text(ctx.register_tags[insn.c], ctx.registers[insn.c]);
          if (!source.has_value() || !needle.has_value() || !replacement.has_value()) {
            trap = Trap::DecodeFault;
            break;
          }
          if (needle->empty()) {
            ctx.register_tags[insn.a] = ValueTag::SymbolHandle;
            update_flags(ctx.registers[insn.a]);
            break;
          }

          std::string replaced;
          replaced.reserve(source->size());
          std::size_t search_from = 0;
          while (true) {
            std::size_t pos = source->find(*needle, search_from);
            if (pos == std::string::npos) {
              replaced.append(*source, search_from, std::string::npos);
              break;
            }
            replaced.append(*source, search_from, pos - search_from);
            replaced.append(*replacement);
            search_from = pos + needle->size();
          }

          ctx.registers[insn.a] = intern_symbol(std::move(replaced));
          ctx.register_tags[insn.a] = ValueTag::SymbolHandle;
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::StrVecNew: {
          if (!reg_ok(insn.a)) {
            trap = Trap::DecodeFault;
            break;
          }
          ctx.registers[insn.a] = alloc_string_vector();
          ctx.register_tags[insn.a] = ValueTag::StringVectorHandle;
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::StrVecPush: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.a] != ValueTag::StringVectorHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* values = string_vector_mut(ctx.registers[insn.a]);
          auto value = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
          if (values == nullptr || !value.has_value()) {
            trap = Trap::DecodeFault;
            break;
          }
          values->push_back(std::string(*value));
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::StrSplit: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          auto value = symbol_like_text(ctx.register_tags[insn.b], ctx.registers[insn.b]);
          auto sep = symbol_like_text(ctx.register_tags[insn.c], ctx.registers[insn.c]);
          if (!value.has_value() || !sep.has_value()) {
            trap = Trap::DecodeFault;
            break;
          }
          if (sep->empty()) {
            trap = Trap::TypeFault;
            break;
          }
          std::vector<std::string> parts;
          std::size_t start = 0;
          while (true) {
            std::size_t pos = value->find(*sep, start);
            if (pos == std::string::npos) {
              parts.push_back(std::string(value->substr(start)));
              break;
            }
            parts.push_back(std::string(value->substr(start, pos - start)));
            start = pos + sep->size();
          }
          state_.string_vectors.push_back(std::move(parts));
          ctx.registers[insn.a] = static_cast<std::int64_t>(state_.string_vectors.size());
          ctx.register_tags[insn.a] = ValueTag::StringVectorHandle;
          {
            t81::axion::Verdict verdict;
            verdict.kind = t81::axion::VerdictKind::Allow;
            std::ostringstream reason;
            reason << t81::axion::reasons::kStringSplit << " input_len=" << value->size()
                   << " sep_len=" << sep->size()
                   << " parts=" << state_.string_vectors.back().size();
            verdict.reason = reason.str();
            record_axion_event(insn.opcode, static_cast<std::int32_t>(state_.string_vectors.size()),
                               static_cast<std::int64_t>(state_.string_vectors.back().size()),
                               verdict);
          }
          update_flags(ctx.registers[insn.a]);
          break;
        }
        case t81::tisc::Opcode::StrJoin: {
          if (!reg_ok(insn.a) || !reg_ok(insn.b) || !reg_ok(insn.c)) {
            trap = Trap::DecodeFault;
            break;
          }
          if (ctx.register_tags[insn.b] != ValueTag::StringVectorHandle) {
            trap = Trap::TypeFault;
            break;
          }
          auto* parts = string_vector_ptr(ctx.registers[insn.b]);
          auto sep = symbol_like_text(ctx.register_tags[insn.c], ctx.registers[insn.c]);
          if (parts == nullptr || !sep.has_value()) {
            trap = Trap::DecodeFault;
            break;
          }
          std::string joined;
          if (!parts->empty()) {
            joined = parts->front();
            for (std::size_t i = 1; i < parts->size(); ++i) {
              joined += *sep;
              joined += parts->at(i);
            }
          }
          ctx.registers[insn.a] = intern_symbol(std::move(joined));
          ctx.register_tags[insn.a] = ValueTag::SymbolHandle;
          {
            t81::axion::Verdict verdict;
            verdict.kind = t81::axion::VerdictKind::Allow;
            std::ostringstream reason;
            reason << t81::axion::reasons::kStringJoin << " parts=" << parts->size()
                   << " sep_len=" << sep->size();
            verdict.reason = reason.str();
            record_axion_event(insn.opcode, static_cast<std::int32_t>(parts->size()),
                               ctx.registers[insn.a], verdict);
          }
          update_flags(ctx.registers[insn.a]);
          break;
        }
