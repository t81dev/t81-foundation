#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include "t81/canonfs/canon_driver.hpp"
#include "t81/canonfs/canon_types.hpp"
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

PYBIND11_MODULE(_t81, m) {
  m.doc() = "T81 Foundation Python Bindings";

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
      .def_static("max_value", []() { return T81Int<81>::kMaxValue; })
      .def_static("min_value", []() { return T81Int<81>::kMinValue; });

  // Bind T81BigInt
  py::class_<v1::T81BigInt>(m, "BigInt")
      .def(py::init<int64_t>())
      .def("__add__", [](const v1::T81BigInt& a, const v1::T81BigInt& b) { return a + b; })
      .def("__sub__", [](const v1::T81BigInt& a, const v1::T81BigInt& b) { return a - b; })
      .def("__mul__", [](const v1::T81BigInt& a, const v1::T81BigInt& b) { return a * b; })
      .def("__repr__", &v1::T81BigInt::str)
      .def("to_int", &v1::T81BigInt::to_int64);

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
          [](Tensor1D3& t, size_t i) -> T81Int<81>& {
            if (i >= 3) throw py::index_error();
            return t(i);
          },
          py::return_value_policy::reference_internal)
      .def("__setitem__",
           [](Tensor1D3& t, size_t i, const T81Int<81>& v) {
             if (i >= 3) throw py::index_error();
             t(i) = v;
           })
      .def("size", [](const Tensor1D3&) { return Tensor1D3::size(); })
      .def("rank", [](const Tensor1D3&) { return Tensor1D3::rank(); })
      .def("__add__", [](const Tensor1D3& a, const Tensor1D3& b) { return a + b; });

  // Bind T81Tensor<T81Int<81>, 2, 3, 3>
  using Tensor2D33 = T81Tensor<T81Int<81>, 2, 3, 3>;
  py::class_<Tensor2D33>(m, "Tensor2D33")
      .def(py::init<>())
      .def(py::init<T81Int<81>>())
      .def(
          "__getitem__",
          [](Tensor2D33& t, py::tuple idx) -> T81Int<81>& {
            if (idx.size() != 2) throw py::value_error("Index must be a 2-tuple");
            size_t i = idx[0].cast<size_t>();
            size_t j = idx[1].cast<size_t>();
            if (i >= 3 || j >= 3) throw py::index_error();
            return t(i, j);
          },
          py::return_value_policy::reference_internal)
      .def("__setitem__",
           [](Tensor2D33& t, py::tuple idx, const T81Int<81>& v) {
             if (idx.size() != 2) throw py::value_error("Index must be a 2-tuple");
             size_t i = idx[0].cast<size_t>();
             size_t j = idx[1].cast<size_t>();
             if (i >= 3 || j >= 3) throw py::index_error();
             t(i, j) = v;
           })
      .def("size", [](const Tensor2D33&) { return Tensor2D33::size(); })
      .def("rank", [](const Tensor2D33&) { return Tensor2D33::rank(); })
      .def("__add__", [](const Tensor2D33& a, const Tensor2D33& b) { return a + b; });

  // Bind T729DynamicTensor (float)
  py::class_<T729DynamicTensor>(m, "T729DynamicTensor")
      .def(py::init<std::vector<int>>())
      .def(py::init<std::vector<int>, std::vector<float>>())
      .def_property_readonly("shape", &T729DynamicTensor::shape)
      .def_property(
          "data", py::overload_cast<>(&T729DynamicTensor::data),
          [](T729DynamicTensor& self, std::vector<float> d) { self.data() = std::move(d); })
      .def("rank", &T729DynamicTensor::rank)
      .def("size", &T729DynamicTensor::size);

  // Bind T729IntTensor
  py::class_<T729IntTensor>(m, "T729IntTensor")
      .def(py::init<std::vector<int>>())
      .def(py::init<std::vector<int>, std::vector<T81Int<81>>>())
      .def_property_readonly("shape", &T729IntTensor::shape)
      .def_property(
          "data", py::overload_cast<>(&T729IntTensor::data),
          [](T729IntTensor& self, std::vector<T81Int<81>> d) { self.data() = std::move(d); })
      .def("rank", &T729IntTensor::rank)
      .def("size", &T729IntTensor::size);

  // Bind TISC Program
  py::class_<tisc::Program>(m, "Program").def(py::init<>());

  // Bind HanoiVM
  py::class_<vm::IVirtualMachine>(m, "HanoiVM")
      .def("load_program", &vm::IVirtualMachine::load_program)
      .def("step",
           [](vm::IVirtualMachine& self) {
             auto res = self.step();
             if (!res) throw std::runtime_error("VM Trap");
           })
      .def(
          "run_to_halt",
          [](vm::IVirtualMachine& self, size_t max_steps) {
            auto res = self.run_to_halt(max_steps);
            if (!res) throw std::runtime_error("VM Trap during execution");
          },
          py::arg("max_steps") = 100000)
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

  m.def("compile", [](const std::string& source) {
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
  });

  m.def("compile_and_run", [](const std::string& source) {
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
    auto program = emitter.emit(ir);

    auto vm = vm::make_interpreter_vm();
    vm->load_program(program);
    auto res = vm->run_to_halt();
    if (!res) throw std::runtime_error("VM Trap during execution");

    auto& state = vm->state();
    if (state.contexts.empty()) throw std::runtime_error("VM state invalid");
    // R0 is unused; the TISC calling convention places main()'s return
    // value in R2 of the root context after the call stack unwinds.
    return state.contexts[state.current_context].registers[2];
  });

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

  py::class_<canonfs::CanonRef>(m, "CanonRef")
      .def(py::init<>())
      .def_property_readonly("hash_bytes",
                             [](const canonfs::CanonRef& ref) {
                               // CanonHash81 is 32 bytes (SHA3-256)
                               return py::bytes(
                                   reinterpret_cast<const char*>(ref.hash.h.bytes.data()),
                                   ref.hash.h.bytes.size());
                             })
      .def("__repr__", [](const canonfs::CanonRef&) {
        return "<CanonRef>";  // We could format hash bytes if we had a formatter easily
                              // available
      });

  py::class_<canonfs::Driver>(m, "CanonDriver")  // Abstract base
      .def("write_object",
           [](canonfs::Driver& self, canonfs::ObjectType type, py::bytes data) {
             std::string s = data;  // Copy from python bytes
             std::span<const std::byte> sp(reinterpret_cast<const std::byte*>(s.data()), s.size());
             auto res = self.write_object(type, sp);
             if (!res)
               throw std::runtime_error("CanonFS Write Error: " + std::to_string((int)res.error()));
             return *res;
           })
      .def("read_object_bytes", [](canonfs::Driver& self, const canonfs::CanonRef& ref) {
        auto res = self.read_object_bytes(ref);
        if (!res)
          throw std::runtime_error("CanonFS Read Error: " + std::to_string((int)res.error()));
        return py::bytes(reinterpret_cast<const char*>(res->data()), res->size());
      });

  m.def("make_in_memory_driver", &canonfs::make_in_memory_driver);
  m.def("make_persistent_driver", [](std::string path) {
    return canonfs::make_persistent_driver(std::filesystem::path(path));
  });
}
