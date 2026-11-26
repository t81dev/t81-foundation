#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <bit>
#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <sstream>
#include <string>
#include <utility>
#include "crypto/ed25519_bundle.hpp"
#include "provider/controlled_provider.hpp"
#include "provider/provider_governor.hpp"
#include "provider/t81_vm_provider.hpp"
#include "t81/axion/policy.hpp"
#include "t81/axion/policy_engine.hpp"
#include "t81/canonfs/canon_driver.hpp"
#include "t81/canonfs/canon_types.hpp"
#include "t81/crypto/sha3.hpp"
#include "t81/frontend/ir_generator.hpp"
#include "t81/frontend/lexer.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"
#include "t81/isa/binary_emitter.hpp"
#include "t81/isa/program.hpp"
#include "t81/tensor.hpp"
#include "t81/types/T81BigInt.hpp"
#include "t81/types/T81Float.hpp"
#include "t81/types/T81Fraction.hpp"
#include "t81/types/T81Int.hpp"
#include "t81/types/T81Tensor.hpp"
#include "t81/vm/vm.hpp"

namespace py = pybind11;
using namespace t81;

namespace {

constexpr std::size_t kDefaultStepLimit = 100000;

class StepLimitExceeded : public std::runtime_error {
public:
  using std::runtime_error::runtime_error;
};

void run_until_halted(vm::IVirtualMachine& machine, std::size_t max_steps) {
  if (!machine.run_to_halt(max_steps)) throw std::runtime_error("VM Trap during execution");
  if (!machine.state().halted) throw StepLimitExceeded("VM step limit exhausted before halt");
}

tisc::Program compile_program(const std::string& source) {
  frontend::Lexer lexer(source);
  frontend::Parser parser(lexer);
  auto stmts = parser.parse();
  if (parser.had_error()) throw std::runtime_error("Parser error");

  frontend::SemanticAnalyzer analyzer(stmts);
  analyzer.analyze();
  if (analyzer.had_error()) throw std::runtime_error("Semantic Analyzer error");

  frontend::IRGenerator ir_gen;
  ir_gen.attach_semantic_analyzer(&analyzer);
  auto ir = ir_gen.generate(stmts);

  tisc::BinaryEmitter emitter;
  return emitter.emit(ir);
}

std::size_t tensor_index(py::handle index, py::ssize_t extent) {
  const auto value = PyNumber_AsSsize_t(index.ptr(), PyExc_IndexError);
  if (value == -1 && PyErr_Occurred()) throw py::error_already_set();
  const auto normalized = value < 0 ? value + extent : value;
  if (normalized < 0 || normalized >= extent) throw py::index_error("tensor index out of range");
  return static_cast<std::size_t>(normalized);
}

template <typename Tensor, typename Element>
void replace_tensor_data(Tensor& tensor, std::vector<Element> values) {
  // Construct first to apply the usual shape validation and storage classification.
  tensor = Tensor(tensor.shape(), std::move(values));
}

py::object exact_python_int(const T81Int<81>& number) {
  py::object result = py::int_(0);
  const auto radix = py::int_(3);
  for (auto remaining = number.significant_trits(); remaining != 0; --remaining) {
    result = result * radix + py::int_(trit_to_int(number[remaining - 1]));
  }
  return result;
}

py::object exact_bigint_python_int(const v1::T81BigInt& number) {
  const std::string digits = number.str();
  const bool negative = number.is_negative();
  py::object result = py::int_(0);
  const auto radix = py::int_(3);
  for (std::size_t i = negative ? 1 : 0; i < digits.size(); ++i) {
    const char digit = digits[i];
    result = result * radix + py::int_(digit == '+' ? 1 : digit == '-' ? -1 : 0);
  }
  return negative ? py::int_(0) - result : result;
}

}  // namespace

PYBIND11_MODULE(_t81, m) {
  m.doc() = "T81 Foundation Python Bindings";

  py::register_exception<StepLimitExceeded>(m, "StepLimitExceeded", PyExc_RuntimeError);

  // Bind T81Int<81>
  py::class_<T81Int<81>>(m, "T81Int")
      .def(py::init<int64_t>())
      .def("__add__", [](const T81Int<81>& a, const T81Int<81>& b) { return a + b; })
      .def("__sub__", [](const T81Int<81>& a, const T81Int<81>& b) { return a - b; })
      .def("__mul__", [](const T81Int<81>& a, const T81Int<81>& b) { return a * b; })
      .def("__repr__",
           [](const T81Int<81>& a) -> std::string {
             try {
               return std::string("<t81.T81Int value=") + std::to_string(a.to_int64()) + ">";
             } catch (...) {
               return std::string("<t81.T81Int value=(large)>");
             }
           })
      .def("__int__", &exact_python_int)
      .def("__index__", &exact_python_int)
      .def_static("max_value", []() { return T81Int<81>::kMaxValue; })
      .def_static("min_value", []() { return T81Int<81>::kMinValue; });

  // Bind T81BigInt
  py::class_<v1::T81BigInt>(m, "BigInt")
      .def(py::init<int64_t>())
      .def("__add__", [](const v1::T81BigInt& a, const v1::T81BigInt& b) { return a + b; })
      .def("__sub__", [](const v1::T81BigInt& a, const v1::T81BigInt& b) { return a - b; })
      .def("__mul__", [](const v1::T81BigInt& a, const v1::T81BigInt& b) { return a * b; })
      .def("__repr__", &v1::T81BigInt::str)
      .def("to_int", &v1::T81BigInt::to_int64)
      .def("__int__", &exact_bigint_python_int)
      .def("__index__", &exact_bigint_python_int);

  // Bind T81Float27_9
  py::class_<v1::T81Float27_9>(m, "Float")
      .def(py::init<double>())
      .def("__add__", [](const v1::T81Float27_9& a, const v1::T81Float27_9& b) { return a + b; })
      .def("__sub__", [](const v1::T81Float27_9& a, const v1::T81Float27_9& b) { return a - b; })
      .def("__mul__", [](const v1::T81Float27_9& a, const v1::T81Float27_9& b) { return a * b; })
      .def("__truediv__",
           [](const v1::T81Float27_9& a, const v1::T81Float27_9& b) { return a / b; })
      .def("__repr__", [](const v1::T81Float27_9& f) { return std::to_string(f.to_double()); })
      .def("to_double", &v1::T81Float27_9::to_double)
      .def_static("from_double", &v1::T81Float27_9::from_double);

  // Bind T81Frac81
  py::class_<v1::T81Frac81>(m, "Fraction")
      .def(py::init<int64_t>())
      .def(py::init(
          [](int64_t n, int64_t d) { return v1::T81Frac81(T81Int<81>(n), T81Int<81>(d)); }))
      .def("__add__", [](const v1::T81Frac81& a, const v1::T81Frac81& b) { return a + b; })
      .def("__sub__", [](const v1::T81Frac81& a, const v1::T81Frac81& b) { return a - b; })
      .def("__mul__", [](const v1::T81Frac81& a, const v1::T81Frac81& b) { return a * b; })
      .def("__truediv__", [](const v1::T81Frac81& a, const v1::T81Frac81& b) { return a / b; })
      .def("__repr__",
           [](const v1::T81Frac81& f) {
             return std::to_string(f.num().to_int64()) + "/" + std::to_string(f.den().to_int64());
           })
      .def("to_double", &v1::T81Frac81::to_double)
      .def_static("from_double", &v1::T81Frac81::from_double, py::arg("value"),
                  py::arg("max_iterations") = 64);

  // Bind T81Tensor<T81Int<81>, 1, 3>
  using Tensor1D3 = T81Tensor<T81Int<81>, 1, 3>;
  py::class_<Tensor1D3>(m, "Tensor1D3")
      .def(py::init<>())
      .def(py::init<T81Int<81>>())
      .def(
          "__getitem__",
          [](Tensor1D3& t, py::handle i) -> T81Int<81>& { return t(tensor_index(i, 3)); },
          py::return_value_policy::reference_internal)
      .def("__setitem__",
           [](Tensor1D3& t, py::handle i, const T81Int<81>& v) { t(tensor_index(i, 3)) = v; })
      .def("size", [](const Tensor1D3&) { return Tensor1D3::size(); })
      .def("rank", [](const Tensor1D3&) { return Tensor1D3::rank(); })
      .def("__add__", [](const Tensor1D3& a, const Tensor1D3& b) { return a + b; })
      .def("__matmul__", [](const Tensor1D3& a, const Tensor1D3& b) {
        int64_t acc = 0;
        for (std::size_t i = 0; i < 3; ++i) acc += a(i).to_int64() * b(i).to_int64();
        return T81Int<81>(acc);
      });

  // Bind T81Tensor<T81Int<81>, 2, 3, 3>
  using Tensor2D33 = T81Tensor<T81Int<81>, 2, 3, 3>;
  py::class_<Tensor2D33>(m, "Tensor2D33")
      .def(py::init<>())
      .def(py::init<T81Int<81>>())
      .def(
          "__getitem__",
          [](Tensor2D33& t, py::tuple idx) -> T81Int<81>& {
            if (idx.size() != 2) throw py::value_error("Index must be a 2-tuple");
            return t(tensor_index(idx[0], 3), tensor_index(idx[1], 3));
          },
          py::return_value_policy::reference_internal)
      .def("__setitem__",
           [](Tensor2D33& t, py::tuple idx, const T81Int<81>& v) {
             if (idx.size() != 2) throw py::value_error("Index must be a 2-tuple");
             t(tensor_index(idx[0], 3), tensor_index(idx[1], 3)) = v;
           })
      .def("size", [](const Tensor2D33&) { return Tensor2D33::size(); })
      .def("rank", [](const Tensor2D33&) { return Tensor2D33::rank(); })
      .def("__add__", [](const Tensor2D33& a, const Tensor2D33& b) { return a + b; })
      .def("__matmul__",
           [](const Tensor2D33& a, const Tensor2D33& b) {
             Tensor2D33 out;
             for (std::size_t i = 0; i < 3; ++i) {
               for (std::size_t j = 0; j < 3; ++j) {
                 int64_t acc = 0;
                 for (std::size_t k = 0; k < 3; ++k) acc += a(i, k).to_int64() * b(k, j).to_int64();
                 out(i, j) = T81Int<81>(acc);
               }
             }
             return out;
           })
      .def("__matmul__", [](const Tensor2D33& a, const Tensor1D3& v) {
        Tensor1D3 out;
        for (std::size_t i = 0; i < 3; ++i) {
          int64_t acc = 0;
          for (std::size_t k = 0; k < 3; ++k) acc += a(i, k).to_int64() * v(k).to_int64();
          out(i) = T81Int<81>(acc);
        }
        return out;
      });

  // Bind T729DynamicTensor (float) with zero-copy Python buffer protocol & __array_interface__
  py::class_<T729DynamicTensor>(m, "T729DynamicTensor", py::buffer_protocol())
      .def(py::init<std::vector<int>>())
      .def(py::init<std::vector<int>, std::vector<float>>())
      .def_buffer([](T729DynamicTensor& t) -> py::buffer_info {
        const auto& sh = t.shape();
        std::vector<py::ssize_t> shape(sh.begin(), sh.end());
        std::vector<py::ssize_t> strides(sh.size(), static_cast<py::ssize_t>(sizeof(float)));
        for (int i = static_cast<int>(sh.size()) - 2; i >= 0; --i) {
          strides[i] = strides[i + 1] * shape[i + 1];
        }
        return py::buffer_info(const_cast<float*>(t.data().data()), sizeof(float),
                               py::format_descriptor<float>::format(),
                               static_cast<py::ssize_t>(sh.size()), shape, strides);
      })
      .def_property_readonly("__array_interface__",
                             [](const T729DynamicTensor& t) {
                               py::dict d;
                               py::tuple sh(t.shape().size());
                               for (std::size_t i = 0; i < t.shape().size(); ++i)
                                 sh[i] = py::int_(t.shape()[i]);
                               d["shape"] = sh;
                               d["typestr"] = "<f4";
                               d["data"] = py::make_tuple(
                                   reinterpret_cast<std::uintptr_t>(t.data().data()), true);
                               d["version"] = 3;
                               return d;
                             })
      .def_property_readonly("shape", &T729DynamicTensor::shape)
      .def_property("data", py::overload_cast<>(&T729DynamicTensor::data, py::const_),
                    &replace_tensor_data<T729DynamicTensor, float>)
      .def("rank", &T729DynamicTensor::rank)
      .def("size", &T729DynamicTensor::size);

  // Bind T729IntTensor with exact Dual-Rail / int64 __matmul__ (@)
  py::class_<T729IntTensor>(m, "T729IntTensor")
      .def(py::init<std::vector<int>>())
      .def(py::init<std::vector<int>, std::vector<T81Int<81>>>())
      .def_property_readonly("shape", &T729IntTensor::shape)
      .def_property("data", py::overload_cast<>(&T729IntTensor::data, py::const_),
                    &replace_tensor_data<T729IntTensor, T81Int<81>>)
      .def("rank", &T729IntTensor::rank)
      .def("size", &T729IntTensor::size)
      .def("__matmul__", [](const T729IntTensor& a, const T729IntTensor& b) {
        if (a.shape().size() != 2 || b.shape().size() != 2 || a.shape()[1] != b.shape()[0]) {
          throw std::invalid_argument("T729IntTensor @ requires 2D shapes (M, K) @ (K, N)");
        }
        const std::size_t M = static_cast<std::size_t>(a.shape()[0]);
        const std::size_t K = static_cast<std::size_t>(a.shape()[1]);
        const std::size_t N = static_cast<std::size_t>(b.shape()[1]);
        const auto& ad = a.data();
        const auto& bd = b.data();
        bool all_ternary = true;
        for (const auto& v : ad) {
          int64_t x = v.to_int64();
          if (x < -1 || x > 1) {
            all_ternary = false;
            break;
          }
        }
        if (all_ternary) {
          for (const auto& v : bd) {
            int64_t x = v.to_int64();
            if (x < -1 || x > 1) {
              all_ternary = false;
              break;
            }
          }
        }
        std::vector<T81Int<81>> out(M * N, T81Int<81>(0));
        if (all_ternary) {
          // Exact Dual-Rail (P, N) bitplane popcount accumulation (max_abs_err == 0).
          const std::size_t words = (K + 63) / 64;
          std::vector<uint64_t> ap(M * words, 0), an(M * words, 0);
          std::vector<uint64_t> bp(N * words, 0), bn(N * words, 0);
          for (std::size_t i = 0; i < M; ++i) {
            for (std::size_t k = 0; k < K; ++k) {
              int64_t x = ad[i * K + k].to_int64();
              if (x == 1)
                ap[i * words + (k >> 6)] |= (1ULL << (k & 63));
              else if (x == -1)
                an[i * words + (k >> 6)] |= (1ULL << (k & 63));
            }
          }
          for (std::size_t j = 0; j < N; ++j) {
            for (std::size_t k = 0; k < K; ++k) {
              int64_t x = bd[k * N + j].to_int64();
              if (x == 1)
                bp[j * words + (k >> 6)] |= (1ULL << (k & 63));
              else if (x == -1)
                bn[j * words + (k >> 6)] |= (1ULL << (k & 63));
            }
          }
          for (std::size_t i = 0; i < M; ++i) {
            for (std::size_t j = 0; j < N; ++j) {
              int64_t acc = 0;
              for (std::size_t w = 0; w < words; ++w) {
                const uint64_t pa = ap[i * words + w], na = an[i * words + w];
                if ((pa | na) == 0ULL) continue;
                const uint64_t pb = bp[j * words + w], nb = bn[j * words + w];
                acc += static_cast<int64_t>(std::popcount((pa & pb) | (na & nb))) -
                       static_cast<int64_t>(std::popcount((pa & nb) | (na & pb)));
              }
              out[i * N + j] = T81Int<81>(acc);
            }
          }
        } else {
          for (std::size_t i = 0; i < M; ++i) {
            for (std::size_t j = 0; j < N; ++j) {
              int64_t acc = 0;
              for (std::size_t k = 0; k < K; ++k) {
                acc += ad[i * K + k].to_int64() * bd[k * N + j].to_int64();
              }
              out[i * N + j] = T81Int<81>(acc);
            }
          }
        }
        return T729IntTensor({static_cast<int>(M), static_cast<int>(N)}, std::move(out));
      });

  // Bind TISC Program
  py::class_<tisc::Program>(m, "Program").def(py::init<>());

  py::enum_<axion::VerdictKind>(m, "VerdictKind")
      .value("Allow", axion::VerdictKind::Allow)
      .value("Deny", axion::VerdictKind::Deny)
      .value("Defer", axion::VerdictKind::Defer)
      .value("Warn", axion::VerdictKind::Warn)
      .value("Quarantine", axion::VerdictKind::Quarantine);

  py::class_<vm::AxionEvent>(m, "AxionEvent")
      .def_property_readonly("pc", [](const vm::AxionEvent& event) { return event.structured.pc; })
      .def_property_readonly(
          "opcode", [](const vm::AxionEvent& event) { return static_cast<int>(event.opcode); })
      .def_property_readonly("verdict",
                             [](const vm::AxionEvent& event) { return event.verdict.kind; })
      .def_property_readonly("reason",
                             [](const vm::AxionEvent& event) { return event.verdict.reason; })
      .def_property_readonly(
          "reason_code", [](const vm::AxionEvent& event) { return event.structured.reason_code; });

  // Bind HanoiVM
  py::class_<vm::IVirtualMachine>(m, "HanoiVM")
      .def("load_program", &vm::IVirtualMachine::load_program)
      .def("step",
           [](vm::IVirtualMachine& self) {
             auto res = self.step();
             if (!res) throw std::runtime_error("VM Trap");
           })
      .def("run_to_halt", &run_until_halted, py::arg("max_steps") = kDefaultStepLimit)
      .def_property_readonly("halted",
                             [](const vm::IVirtualMachine& self) { return self.state().halted; })
      .def("get_register",
           [](vm::IVirtualMachine& self, int idx) {
             if (idx < 0 || idx >= 243) throw py::index_error();
             auto& state = self.state();
             if (state.contexts.empty())
               throw std::runtime_error("VM not initialized (no contexts)");
             return state.contexts[state.current_context].registers[idx];
           })
      .def("set_register",
           [](vm::IVirtualMachine& self, int idx, int64_t val) {
             if (idx < 0 || idx >= 243) throw py::index_error();
             self.set_register(idx, val);
           })
      .def_property_readonly("axion_events",
                             [](const vm::IVirtualMachine& self) { return self.state().axion_log; })
      .def_property_readonly("axion_log",
                             [](vm::IVirtualMachine& self) {
                               std::vector<std::string> logs;
                               for (const auto& event : self.state().axion_log) {
                                 logs.push_back(event.verdict.reason);
                               }
                               return logs;
                             })
      .def_property_readonly("trace", [](vm::IVirtualMachine& self) {
        std::vector<std::string> traces;
        for (const auto& entry : self.state().trace) {
          std::string op_name = std::to_string(static_cast<int>(entry.opcode));
          traces.push_back(std::string("PC=") + std::to_string(entry.pc) + " OP=" + op_name);
        }
        return traces;
      });

  m.def("make_interpreter_vm", []() { return vm::make_interpreter_vm(); });

  m.def("compile", &compile_program, py::arg("source"));

  m.def(
      "compile_and_run",
      [](const std::string& source, std::size_t max_steps) {
        auto vm = vm::make_interpreter_vm();
        vm->load_program(compile_program(source));
        run_until_halted(*vm, max_steps);

        const auto& state = vm->state();
        if (state.contexts.empty()) throw std::runtime_error("VM state invalid");
        // The TISC calling convention places main()'s return value in R2.
        return state.contexts[state.current_context].registers[2];
      },
      py::arg("source"), py::kw_only(), py::arg("max_steps") = kDefaultStepLimit);

  // Bind CanonFS
  py::enum_<canonfs::ObjectType>(m, "ObjectType")
      .value("RawBlock", canonfs::ObjectType::RawBlock)
      .value("FileNode", canonfs::ObjectType::FileNode)
      .value("Directory", canonfs::ObjectType::Directory)
      .value("Snapshot", canonfs::ObjectType::Snapshot)
      .value("CapabilityGrant", canonfs::ObjectType::CapabilityGrant)
      .value("CapabilityRevoke", canonfs::ObjectType::CapabilityRevoke)
      .value("CompressedBlock", canonfs::ObjectType::CompressedBlock)
      .value("CanonParity", canonfs::ObjectType::CanonParity)
      .value("CanonIndex", canonfs::ObjectType::CanonIndex)
      .value("CanonMeta", canonfs::ObjectType::CanonMeta)
      .value("CanonSeal", canonfs::ObjectType::CanonSeal)
      .value("CanonLink", canonfs::ObjectType::CanonLink)
      .value("CanonExec", canonfs::ObjectType::CanonExec)
      .value("CanonTensor", canonfs::ObjectType::CanonTensor)
      .export_values();

  auto canonfs_error_name = [](canonfs::Error err) -> const char* {
    switch (err) {
      case canonfs::Error::None:
        return "None";
      case canonfs::Error::NotFound:
        return "NotFound";
      case canonfs::Error::InvalidObject:
        return "InvalidObject";
      case canonfs::Error::CapabilityError:
        return "CapabilityError";
      case canonfs::Error::ParityFailure:
        return "ParityFailure";
      case canonfs::Error::DecodeError:
        return "DecodeError";
    }
    return "Unknown";
  };

  auto canonref_to_hex = [](const canonfs::CanonRef& ref) -> std::string {
    constexpr char kHexDigits[] = "0123456789abcdef";
    std::string out;
    out.reserve(ref.hash.h.bytes.size() * 2);
    for (std::uint8_t byte : ref.hash.h.bytes) {
      out.push_back(kHexDigits[byte >> 4]);
      out.push_back(kHexDigits[byte & 0x0F]);
    }
    return out;
  };

  py::class_<canonfs::CanonRef>(m, "CanonRef")
      .def(py::init<>())
      .def_static(
          "from_bytes",
          [](py::bytes digest) {
            std::string raw = digest;
            canonfs::CanonRef ref{};
            if (raw.size() != ref.hash.h.bytes.size()) {
              throw py::value_error("CanonRef digest must be exactly 32 bytes");
            }
            std::memcpy(ref.hash.h.bytes.data(), raw.data(), ref.hash.h.bytes.size());
            return ref;
          },
          py::arg("digest"))
      .def_static(
          "from_hex",
          [](py::str digest_obj) {
            const std::string digest = digest_obj.cast<std::string>();
            canonfs::CanonRef ref{};
            constexpr std::size_t kExpectedHexLen = 64;
            if (digest.size() != kExpectedHexLen) {
              throw py::value_error(
                  "CanonRef hex digest must be exactly 64 hexadecimal characters");
            }
            auto hex_nibble = [](unsigned char c) -> int {
              if (c >= '0' && c <= '9') return c - '0';
              if (c >= 'a' && c <= 'f') return c - 'a' + 10;
              if (c >= 'A' && c <= 'F') return c - 'A' + 10;
              return -1;
            };
            for (std::size_t i = 0; i < ref.hash.h.bytes.size(); ++i) {
              const int hi = hex_nibble(static_cast<unsigned char>(digest[2 * i]));
              const int lo = hex_nibble(static_cast<unsigned char>(digest[2 * i + 1]));
              if (hi < 0 || lo < 0) {
                throw py::value_error("CanonRef hex digest contains non-hexadecimal character");
              }
              ref.hash.h.bytes[i] = static_cast<std::uint8_t>((hi << 4) | lo);
            }
            return ref;
          },
          py::arg("digest"))
      .def_property_readonly("hash_bytes",
                             [](const canonfs::CanonRef& ref) {
                               // CanonHash81 is 32 bytes (SHA3-512 truncated to 256 bits)
                               return py::bytes(
                                   reinterpret_cast<const char*>(ref.hash.h.bytes.data()),
                                   ref.hash.h.bytes.size());
                             })
      .def("hex", [canonref_to_hex](const canonfs::CanonRef& ref) { return canonref_to_hex(ref); })
      .def(
          "to_cli_handle",
          [](const canonfs::CanonRef& ref) { return "sha3-256:" + ref.hash.h.to_string(); },
          "Return the canonical sha3-256:<Base-81 hash> accepted by the native CanonFS CLI.")
      .def("__eq__",
           [](const canonfs::CanonRef& self, const py::object& other) -> py::object {
             if (!py::isinstance<canonfs::CanonRef>(other)) {
               return py::reinterpret_borrow<py::object>(Py_NotImplemented);
             }
             return py::bool_(self.hash == other.cast<const canonfs::CanonRef&>().hash);
           })
      .def("__hash__",
           [](const canonfs::CanonRef& ref) {
             py::bytes b(reinterpret_cast<const char*>(ref.hash.h.bytes.data()),
                         ref.hash.h.bytes.size());
             return py::hash(b);
           })
      .def("__repr__", [canonref_to_hex](const canonfs::CanonRef& ref) {
        return std::string("<CanonRef ") + canonref_to_hex(ref) + ">";
      });

  py::class_<canonfs::Driver>(m, "CanonDriver")  // Abstract base
      .def("write_object",
           [canonfs_error_name](canonfs::Driver& self, canonfs::ObjectType type, py::bytes data) {
             std::string s = data;  // Copy from python bytes
             std::span<const std::byte> sp(reinterpret_cast<const std::byte*>(s.data()), s.size());
             auto res = self.write_object(type, sp);
             if (!res)
               throw std::runtime_error("CanonFS Write Error: " + std::to_string((int)res.error()) +
                                        " (" + canonfs_error_name(res.error()) + ")");
             return *res;
           })
      .def("read_object_bytes",
           [canonfs_error_name](canonfs::Driver& self, const canonfs::CanonRef& ref) {
             auto res = self.read_object_bytes(ref);
             if (!res)
               throw std::runtime_error("CanonFS Read Error: " + std::to_string((int)res.error()) +
                                        " (" + canonfs_error_name(res.error()) + ")");
             return py::bytes(reinterpret_cast<const char*>(res->data()), res->size());
           });

  m.def("make_in_memory_driver", &canonfs::make_in_memory_driver);
  m.def("make_persistent_driver", [](std::string path) {
    return canonfs::make_persistent_driver(std::filesystem::path(path));
  });

  // ── v2.1.0 Primitives: SHA3-512, Axion Policy, Dual-Rail Pack/Dot/Diff/Adapter ──
  m.def("sha3_512_hex", [](py::bytes data) {
    std::string s = data;
    std::span<const uint8_t> sp(reinterpret_cast<const uint8_t*>(s.data()), s.size());
    return std::string("sha3-512:") + crypto::sha3_512_hex(sp);
  });

  m.def("validate_policy", [](const std::string& apl_text) {
    auto res = axion::parse_policy(apl_text);
    py::dict d;
    if (!res) {
      d["valid"] = false;
      d["tier"] = 0;
      d["error"] = res.error();
    } else {
      d["valid"] = true;
      d["tier"] = res.value().tier;
      d["error"] = "";
    }
    return d;
  });

  m.def("dualrail_pack", [](const std::vector<int>& trits) {
    const size_t words = (trits.size() + 63) / 64;
    std::vector<uint64_t> pos(words, 0ULL), neg(words, 0ULL);
    for (size_t i = 0; i < trits.size(); ++i) {
      if (trits[i] == 1) {
        pos[i >> 6] |= (1ULL << (i & 63));
      } else if (trits[i] == -1) {
        neg[i >> 6] |= (1ULL << (i & 63));
      } else if (trits[i] != 0) {
        throw std::invalid_argument("dualrail_pack expects values in {-1, 0, +1}");
      }
    }
    return py::make_tuple(py::bytes(reinterpret_cast<const char*>(pos.data()), words * 8),
                          py::bytes(reinterpret_cast<const char*>(neg.data()), words * 8));
  });

  m.def("dualrail_dot", [](py::bytes ap_b, py::bytes an_b, py::bytes bp_b, py::bytes bn_b) {
    std::string ap = ap_b, an = an_b, bp = bp_b, bn = bn_b;
    if (ap.size() != an.size() || ap.size() != bp.size() || ap.size() != bn.size() ||
        (ap.size() % 8) != 0) {
      throw std::invalid_argument("Dual-Rail planes must have equal sizes divisible by 8");
    }
    const size_t words = ap.size() / 8;
    int64_t acc = 0;
    for (size_t w = 0; w < words; ++w) {
      uint64_t pa = 0, na = 0, pb = 0, nb = 0;
      std::memcpy(&pa, ap.data() + w * 8, 8);
      std::memcpy(&na, an.data() + w * 8, 8);
      std::memcpy(&pb, bp.data() + w * 8, 8);
      std::memcpy(&nb, bn.data() + w * 8, 8);
      if (((pa & na) | (pb & nb)) != 0ULL) {
        throw std::invalid_argument("Dual-Rail disjoint plane invariant violated (P & N != 0)");
      }
      if ((pa | na) == 0ULL) continue;
      acc += static_cast<int64_t>(std::popcount((pa & pb) | (na & nb))) -
             static_cast<int64_t>(std::popcount((pa & nb) | (na & pb)));
    }
    return acc;
  });

  m.def(
      "dualrail_diff_check",
      [](py::bytes bp_b, py::bytes bn_b, py::bytes cp_b, py::bytes cn_b, double max_flip_pct) {
        std::string bp = bp_b, bn = bn_b, cp = cp_b, cn = cn_b;
        if (bp.empty() || bp.size() != bn.size() || bp.size() != cp.size() ||
            bp.size() != cn.size() || (bp.size() % 8) != 0) {
          throw std::invalid_argument("Dual-Rail planes must be non-empty, equal, and % 8 == 0");
        }
        const size_t words = bp.size() / 8;
        const uint64_t total_weights = static_cast<uint64_t>(words) * 64ULL;
        uint64_t flips = 0;
        for (size_t w = 0; w < words; ++w) {
          uint64_t p0 = 0, n0 = 0, p1 = 0, n1 = 0;
          std::memcpy(&p0, bp.data() + w * 8, 8);
          std::memcpy(&n0, bn.data() + w * 8, 8);
          std::memcpy(&p1, cp.data() + w * 8, 8);
          std::memcpy(&n1, cn.data() + w * 8, 8);
          if (((p0 & n0) | (p1 & n1)) != 0ULL) {
            throw std::invalid_argument("Dual-Rail disjoint plane invariant violated");
          }
          flips += static_cast<uint64_t>(std::popcount((p0 ^ p1) | (n0 ^ n1)));
        }
        const double flip_pct =
            100.0 * static_cast<double>(flips) / static_cast<double>(total_weights);
        std::string b_cat = bp + bn;
        std::string c_cat = cp + cn;
        py::dict d;
        d["allowed"] = (flip_pct <= max_flip_pct);
        d["total_weights"] = total_weights;
        d["flipped_weights"] = flips;
        d["flip_pct"] = flip_pct;
        d["base_sha3_512"] =
            "sha3-512:" + crypto::sha3_512_hex(std::span<const uint8_t>(
                              reinterpret_cast<const uint8_t*>(b_cat.data()), b_cat.size()));
        d["candidate_sha3_512"] =
            "sha3-512:" + crypto::sha3_512_hex(std::span<const uint8_t>(
                              reinterpret_cast<const uint8_t*>(c_cat.data()), c_cat.size()));
        return d;
      },
      py::arg("base_pos"), py::arg("base_neg"), py::arg("cand_pos"), py::arg("cand_neg"),
      py::arg("max_flip_pct") = 1.0);

  m.def(
      "dualrail_adapter_apply",
      [](py::bytes bp_b, py::bytes bn_b, py::bytes dp_b, py::bytes dn_b, double max_flip_pct) {
        std::string bp = bp_b, bn = bn_b, dp = dp_b, dn = dn_b;
        if (bp.empty() || bp.size() != bn.size() || bp.size() != dp.size() ||
            bp.size() != dn.size() || (bp.size() % 8) != 0) {
          throw std::invalid_argument("Dual-Rail planes must be non-empty, equal, and % 8 == 0");
        }
        const size_t words = bp.size() / 8;
        const uint64_t total_weights = static_cast<uint64_t>(words) * 64ULL;
        std::string op(bp.size(), '\0'), on(bn.size(), '\0');
        uint64_t flips = 0;
        for (size_t w = 0; w < words; ++w) {
          uint64_t wp = 0, wn = 0, ap = 0, an = 0;
          std::memcpy(&wp, bp.data() + w * 8, 8);
          std::memcpy(&wn, bn.data() + w * 8, 8);
          std::memcpy(&ap, dp.data() + w * 8, 8);
          std::memcpy(&an, dn.data() + w * 8, 8);
          if (((wp & wn) | (ap & an)) != 0ULL) {
            throw std::invalid_argument("Dual-Rail disjoint plane invariant violated");
          }
          const uint64_t rp = (wp & ~an) | (ap & ~wn);
          const uint64_t rn = (wn & ~ap) | (an & ~wp);
          std::memcpy(op.data() + w * 8, &rp, 8);
          std::memcpy(on.data() + w * 8, &rn, 8);
          flips += static_cast<uint64_t>(std::popcount((wp ^ rp) | (wn ^ rn)));
        }
        const double flip_pct =
            100.0 * static_cast<double>(flips) / static_cast<double>(total_weights);
        const bool applied = (flip_pct <= max_flip_pct);
        std::string m_cat = op + on;
        py::dict d;
        d["applied"] = applied;
        d["total_weights"] = total_weights;
        d["effective_flips"] = flips;
        d["flip_pct"] = flip_pct;
        d["merged_pos"] = applied ? py::bytes(op) : py::bytes();
        d["merged_neg"] = applied ? py::bytes(on) : py::bytes();
        d["merged_sha3_512"] =
            "sha3-512:" + crypto::sha3_512_hex(std::span<const uint8_t>(
                              reinterpret_cast<const uint8_t*>(m_cat.data()), m_cat.size()));
        return d;
      },
      py::arg("base_pos"), py::arg("base_neg"), py::arg("delta_pos"), py::arg("delta_neg"),
      py::arg("max_flip_pct") = 100.0);

  // ── Wave 3 M3: Native C++ Ed25519 / verify_dag & ProviderGovernor bindings ──
  m.def("ed25519_generate_keypair", []() {
    auto kp = crypto::ed25519_generate_keypair();
    py::dict d;
    d["private_key_pem"] = crypto::export_private_key_pem(kp.seed);
    d["public_key_pem"] = crypto::export_public_key_pem(kp.public_key);
    return d;
  });

  m.def(
      "sign_rfc00d4_payload",
      [](const std::string& private_key_pem, const std::string& meaning_json,
         const std::string& evidence_json, const std::string& authority_json) {
        auto seed = crypto::import_private_key_pem(private_key_pem);
        if (!seed.has_value()) throw std::invalid_argument("invalid Ed25519 private key PEM");
        auto kp = crypto::ed25519_keypair_from_seed(*seed);
        auto payload =
            crypto::rfc00d4_canonical_signing_payload(meaning_json, evidence_json, authority_json);
        py::dict d;
        d["canonical_signing_payload"] = payload;
        d["signature_b64"] = crypto::sign_rfc00d4_payload_b64(kp.seed, kp.public_key, payload);
        d["public_key_pem"] = crypto::export_public_key_pem(kp.public_key);
        return d;
      },
      py::arg("private_key_pem"), py::arg("meaning_json"), py::arg("evidence_json"),
      py::arg("authority_json"));

  m.def("verify_dag", [](const std::vector<py::dict>& raw_nodes) {
    std::vector<crypto::AccountableBundleNode> nodes;
    nodes.reserve(raw_nodes.size());
    for (const auto& nd : raw_nodes) {
      crypto::AccountableBundleNode node;
      node.bundle_id = nd["bundle_id"].cast<std::string>();
      node.parent_bundle_ids = nd["parent_bundle_ids"].cast<std::vector<std::string>>();
      node.canonical_signing_payload = nd["canonical_signing_payload"].cast<std::string>();
      node.signature_b64 = nd["signature_b64"].cast<std::string>();
      auto pub = crypto::import_public_key_pem(nd["public_key_pem"].cast<std::string>());
      if (!pub.has_value()) {
        py::dict out;
        out["ok"] = false;
        out["code"] = "ERR_INVALID_PUBLIC_KEY";
        out["error"] = "invalid Ed25519 public key PEM in DAG node";
        out["verified_nodes"] = 0;
        return out;
      }
      node.public_key = *pub;
      nodes.push_back(std::move(node));
    }
    auto res = crypto::verify_dag(nodes);
    py::dict out;
    out["ok"] = res.ok;
    out["code"] = res.code;
    out["error"] = res.error;
    out["verified_nodes"] = res.verified_nodes;
    return out;
  });

  struct PyProviderGovernor {
    std::shared_ptr<axion::PolicyEngine> policy_engine;
    std::shared_ptr<canonfs::Driver> canonfs_driver;
    std::string policy_sha3_512;
    provider::ProviderGovernor governor;

    PyProviderGovernor(const std::string& policy_text, std::shared_ptr<canonfs::Driver> drv)
        : policy_engine([&]() {
            auto parsed = axion::parse_policy(policy_text);
            if (!parsed.has_value()) throw std::invalid_argument("invalid .apl policy");
            return std::make_shared<axion::PolicyEngine>(std::move(parsed.value()));
          }()),
          canonfs_driver(drv ? std::move(drv)
                             : std::shared_ptr<canonfs::Driver>(canonfs::make_in_memory_driver())),
          policy_sha3_512(
              provider::ProviderGovernor::compute_policy_sha3_512(policy_engine->policy())),
          governor(policy_engine, canonfs_driver) {}

    py::dict run_provider(provider::IAiProvider& prov, const std::string& operation,
                          const std::string& media_type, const std::string& input_str,
                          const std::string& principal_id, bool user_consent_verified,
                          std::optional<uint64_t> seed, uint64_t max_steps) {
      provider::ProviderRequest req;
      req.operation = operation;
      req.input.media_type = media_type;
      req.input.bytes.assign(reinterpret_cast<const uint8_t*>(input_str.data()),
                             reinterpret_cast<const uint8_t*>(input_str.data()) + input_str.size());
      req.seed = seed;
      req.budget.max_steps = max_steps;
      provider::ExecutionContext ctx;
      ctx.principal_id = principal_id;
      ctx.policy_sha3_512 = policy_sha3_512;
      ctx.user_consent_verified = user_consent_verified;
      provider::CancellationToken cancel;
      auto outcome = governor.execute(prov, req, ctx, cancel);
      py::dict d;
      d["ok"] = (outcome.status == provider::ExecutionStatus::Succeeded);
      d["trit"] = static_cast<int>(outcome.trit);
      d["provider_invoked"] = outcome.provider_invoked;
      d["evidence_json"] = outcome.evidence_json;
      d["evidence_sha3_512"] = outcome.evidence_sha3_512;
      d["output_bytes"] = py::bytes(reinterpret_cast<const char*>(outcome.output.bytes.data()),
                                    outcome.output.bytes.size());
      d["error_message"] = outcome.error_message;
      return d;
    }
  };

  py::class_<provider::T81VmProvider>(m, "T81VmProvider")
      .def(py::init<>())
      .def("capabilities_json", [](const provider::T81VmProvider& p) {
        return provider::ProviderGovernor::serialize_capabilities_json(p.descriptor());
      });

  py::class_<provider::ControlledProvider>(m, "ControlledProvider")
      .def(py::init<>())
      .def("capabilities_json", [](const provider::ControlledProvider& p) {
        return provider::ProviderGovernor::serialize_capabilities_json(p.descriptor());
      });

  py::class_<PyProviderGovernor>(m, "ProviderGovernor")
      .def(py::init<const std::string&, std::shared_ptr<canonfs::Driver>>(), py::arg("policy_text"),
           py::arg("canonfs_driver") = nullptr)
      .def_readonly("policy_sha3_512", &PyProviderGovernor::policy_sha3_512)
      .def(
          "execute_vm",
          [](PyProviderGovernor& self, provider::T81VmProvider& prov, py::bytes bytecode,
             const std::string& principal_id, uint64_t max_steps) {
            return self.run_provider(prov, "tisc.execute", "application/x-t81-tisc",
                                     std::string(bytecode), principal_id, true, std::nullopt,
                                     max_steps);
          },
          py::arg("provider"), py::arg("bytecode"), py::arg("principal_id") = "python-agent",
          py::arg("max_steps") = 100000)
      .def(
          "execute_controlled",
          [](PyProviderGovernor& self, provider::ControlledProvider& prov,
             const std::string& input_text, const std::string& principal_id,
             bool user_consent_verified, std::optional<uint64_t> seed) {
            return self.run_provider(prov, "controlled.sample", "text/plain", input_text,
                                     principal_id, user_consent_verified, seed, 1000);
          },
          py::arg("provider"), py::arg("input_text"), py::arg("principal_id") = "python-agent",
          py::arg("user_consent_verified") = false, py::arg("seed") = std::nullopt);
}
