#include "t81/tisc/binary_io.hpp"
#include "t81/bigint.hpp"
#include "t81/fraction.hpp"
#include "t81/tensor.hpp"
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace t81 {
namespace tisc {

// --- Generic Helpers for Simple Types ---
template<typename T>
void write_vector(std::ostream& os, const std::vector<T>& vec) {
    uint64_t size = vec.size();
    os.write(reinterpret_cast<const char*>(&size), sizeof(size));
    if (size > 0) {
        os.write(reinterpret_cast<const char*>(vec.data()), size * sizeof(T));
    }
}

template<typename T>
void read_vector(std::istream& is, std::vector<T>& vec) {
    uint64_t size;
    is.read(reinterpret_cast<char*>(&size), sizeof(size));
    vec.resize(size);
    if (size > 0) {
        is.read(reinterpret_cast<char*>(vec.data()), size * sizeof(T));
    }
}

// --- Specializations for Complex Types ---

// std::string
void write_string(std::ostream& os, const std::string& str) {
    uint64_t size = str.size();
    os.write(reinterpret_cast<const char*>(&size), sizeof(size));
    os.write(str.data(), size);
}

void read_string(std::istream& is, std::string& str) {
    uint64_t size;
    is.read(reinterpret_cast<char*>(&size), sizeof(size));
    str.resize(size);
    is.read(&str[0], size);
}

template<typename T>
void write_serializable_vector(std::ostream& os, const std::vector<T>& vec) {
    uint64_t size = vec.size();
    os.write(reinterpret_cast<const char*>(&size), sizeof(size));
    for (const auto& item : vec) {
        item.serialize(os);
    }
}

template<typename T>
void read_serializable_vector(std::istream& is, std::vector<T>& vec) {
    uint64_t size;
    is.read(reinterpret_cast<char*>(&size), sizeof(size));
    vec.resize(size);
    for (uint64_t i = 0; i < size; ++i) {
        vec[i].deserialize(is);
    }
}


void write_vector_string(std::ostream& os, const std::vector<std::string>& vec) {
    uint64_t size = vec.size();
    os.write(reinterpret_cast<const char*>(&size), sizeof(size));
    for (const auto& str : vec) {
        write_string(os, str);
    }
}

void read_vector_string(std::istream& is, std::vector<std::string>& vec) {
    uint64_t size;
    is.read(reinterpret_cast<char*>(&size), sizeof(size));
    vec.resize(size);
    for (uint64_t i = 0; i < size; ++i) {
        read_string(is, vec[i]);
    }
}

// vector<vector<int>> for shapes
void write_vector_vector_int(std::ostream& os, const std::vector<std::vector<int>>& vec) {
    uint64_t size = vec.size();
    os.write(reinterpret_cast<const char*>(&size), sizeof(size));
    for (const auto& inner_vec : vec) {
        write_vector(os, inner_vec);
    }
}

void read_vector_vector_int(std::istream& is, std::vector<std::vector<int>>& vec) {
    uint64_t size;
    is.read(reinterpret_cast<char*>(&size), sizeof(size));
    vec.resize(size);
    for (uint64_t i = 0; i < size; ++i) {
        read_vector(is, vec[i]);
    }
}


// --- Main Functions ---

void save_program(const Program& program, const std::string& path) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not open file for writing: " + path);
    }

    write_vector(file, program.insns);
    write_vector(file, program.float_pool);
    write_serializable_vector(file, program.fraction_pool);
    write_vector_string(file, program.symbol_pool);
    write_serializable_vector(file, program.tensor_pool);
    write_vector_vector_int(file, program.shape_pool);
    write_string(file, program.axion_policy_text);
}

Program load_program(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Could not open file for reading: " + path);
    }

    Program program;
    read_vector(file, program.insns);
    read_vector(file, program.float_pool);
    read_serializable_vector(file, program.fraction_pool);
    read_vector_string(file, program.symbol_pool);
    read_serializable_vector(file, program.tensor_pool);
    read_vector_vector_int(file, program.shape_pool);
    read_string(file, program.axion_policy_text);

    return program;
}

} // namespace tisc
} // namespace t81
