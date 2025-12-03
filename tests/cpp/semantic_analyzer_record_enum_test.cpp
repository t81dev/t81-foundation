#include "test_utils.hpp"

int main() {
    const std::string simple_record = R"(
        record Point {
            x: i32;
            y: i32;
        }

        fn main() -> i32 {
            let p: Point = Point { x: 1; y: 2; };
            let sum: i32 = p.x + p.y;
            return sum;
        }
    )";
    expect_semantic_success(simple_record, "simple_record");

    const std::string missing_field = R"(
        record Point {
            x: i32;
            y: i32;
        }

        fn main() -> i32 {
            let p: Point = Point { x: 1 };
            return 0;
        }
    )";
    expect_semantic_failure(missing_field, "missing_field", "missing field 'y'");

    const std::string unknown_field = R"(
        record Point {
            x: i32;
            y: i32;
        }

        fn main() -> i32 {
            let p: Point = Point { x: 1; y: 2; z: 3 };
            return 0;
        }
    )";
    expect_semantic_failure(unknown_field, "unknown_field", "has no field 'z'");

    const std::string duplicate_field = R"(
        record Point {
            x: i32;
            y: i32;
        }

        fn main() -> i32 {
            let p: Point = Point { x: 1; x: 2; y: 3 };
            return 0;
        }
    )";
    expect_semantic_failure(duplicate_field, "duplicate_field", "is provided more than once");

    const std::string type_mismatch = R"(
        record Point {
            x: i32;
            y: i32;
        }

        fn main() -> i32 {
            let p: Point = Point { x: 1.5; y: 2 };
            return 0;
        }
    )";
    expect_semantic_failure(type_mismatch, "type_mismatch", "Cannot assign 'T81Float' to field 'x' of type 'i32'");

    const std::string enum_definition = R"(
        enum Flag {
            On;
            Off;
        }

        fn main() -> i32 {
            return 0;
        }
    )";
    expect_semantic_success(enum_definition, "enum_definition");

    const std::string enum_duplicate_variant = R"(
        enum Mode {
            Start;
            Start;
        }

        fn main() -> i32 {
            return 0;
        }
    )";
    expect_semantic_failure(enum_duplicate_variant, "enum_duplicate_variant", "already exists in enum");

    std::cout << "Semantic analyzer record/enum tests passed!" << std::endl;
    return 0;
}
