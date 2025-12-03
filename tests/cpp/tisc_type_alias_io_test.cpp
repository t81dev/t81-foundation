#include "t81/tisc/binary_io.hpp"
#include "t81/tisc/type_alias.hpp"

#include <cassert>
#include <filesystem>
#include <iostream>

void test_type_alias_io_roundtrip() {
    t81::tisc::Program program;
    t81::tisc::TypeAliasMetadata box_alias;
    box_alias.name = "Box";
    box_alias.params = {"T", "Rank"};
    box_alias.alias = "Tensor[T, Rank]";
    program.type_aliases.push_back(box_alias);

    t81::tisc::TypeAliasMetadata vertex_alias;
    vertex_alias.name = "Vertex";
    vertex_alias.params = {"Graph"};
    vertex_alias.alias = "Tensor[Graph, 2]";
    program.type_aliases.push_back(vertex_alias);

    auto temp_file = std::filesystem::temp_directory_path() / "tisc_alias_io_test.bin";
    t81::tisc::save_program(program, temp_file.string());

    auto loaded = t81::tisc::load_program(temp_file.string());
    assert(loaded.type_aliases.size() == program.type_aliases.size());
    for (size_t i = 0; i < program.type_aliases.size(); ++i) {
        const auto& expected = program.type_aliases[i];
        const auto& actual = loaded.type_aliases[i];
        assert(actual.name == expected.name);
        assert(actual.params == expected.params);
        assert(actual.alias == expected.alias);
    }

    std::filesystem::remove(temp_file);
    std::cout << "TypeAliasIoTest test_type_alias_io_roundtrip passed!" << std::endl;
}

int main() {
    test_type_alias_io_roundtrip();
    return 0;
}
