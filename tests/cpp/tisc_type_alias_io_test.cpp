#include "t81/tisc/binary_io.hpp"
#include "t81/tisc/type_alias.hpp"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <optional>

void test_type_alias_io_roundtrip() {
    using t81::tisc::FieldInfo;
    using t81::tisc::StructuralKind;
    using t81::tisc::VariantInfo;

    t81::tisc::Program program;
    t81::tisc::TypeAliasMetadata box_alias;
    box_alias.name = "Point";
    box_alias.kind = StructuralKind::Record;
    box_alias.fields = {FieldInfo{"x", "i32"}, FieldInfo{"y", "i32"}};
    box_alias.schema_version = 2;
    box_alias.module_path = "PointModule";
    program.type_aliases.push_back(box_alias);

    t81::tisc::TypeAliasMetadata flag_alias;
    flag_alias.name = "Flag";
    flag_alias.kind = StructuralKind::Enum;
    flag_alias.variants = {VariantInfo{"On", std::nullopt}, VariantInfo{"Off", std::nullopt}};
    flag_alias.schema_version = 3;
    flag_alias.module_path = "FlagModule";
    program.type_aliases.push_back(flag_alias);

    t81::tisc::TypeAliasMetadata vertex_alias;
    vertex_alias.name = "Vertex";
    vertex_alias.params = {"Graph"};
    vertex_alias.alias = "Tensor[Graph, 2]";
    vertex_alias.schema_version = 5;
    vertex_alias.module_path = "tensor.alias";
    program.type_aliases.push_back(vertex_alias);

    auto temp_file = std::filesystem::temp_directory_path() / "tisc_alias_io_test.bin";
    t81::tisc::save_program(program, temp_file.string());

    auto loaded = t81::tisc::load_program(temp_file.string());
    assert(loaded.type_aliases.size() == program.type_aliases.size());

    [[maybe_unused]] auto matches_fields = [](const std::vector<FieldInfo>& lhs, const std::vector<FieldInfo>& rhs) {
        if (lhs.size() != rhs.size()) return false;
        for (size_t i = 0; i < lhs.size(); ++i) {
            if (lhs[i].name != rhs[i].name || lhs[i].type != rhs[i].type) {
                return false;
            }
        }
        return true;
    };

    [[maybe_unused]] auto matches_variants = [](const std::vector<VariantInfo>& lhs, const std::vector<VariantInfo>& rhs) {
        if (lhs.size() != rhs.size()) return false;
        for (size_t i = 0; i < lhs.size(); ++i) {
            if (lhs[i].name != rhs[i].name) return false;
            if (lhs[i].payload != rhs[i].payload) return false;
        }
        return true;
    };

    for (size_t i = 0; i < program.type_aliases.size(); ++i) {
        const auto& expected = program.type_aliases[i];
        [[maybe_unused]] const auto& actual = loaded.type_aliases[i];
        assert(actual.name == expected.name);
        assert(actual.params == expected.params);
        assert(actual.alias == expected.alias);
        assert(actual.kind == expected.kind);
        assert(actual.schema_version == expected.schema_version);
        assert(actual.module_path == expected.module_path);
        if (expected.kind == StructuralKind::Record) {
            assert(matches_fields(actual.fields, expected.fields));
        }
        if (expected.kind == StructuralKind::Enum) {
            assert(matches_variants(actual.variants, expected.variants));
        }
    }

    std::filesystem::remove(temp_file);
    std::cout << "TypeAliasIoTest test_type_alias_io_roundtrip passed!" << std::endl;
}

int main() {
    test_type_alias_io_roundtrip();
    return 0;
}
