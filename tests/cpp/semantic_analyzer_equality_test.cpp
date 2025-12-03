#include "test_utils.hpp"

int main() {
    const std::string valid_equality = R"(
        fn main() -> bool {
            return 1 == 2;
        }
    )";
    expect_semantic_success(valid_equality, "valid_equality");

    const std::string invalid_equality = R"(
        fn main() -> bool {
            return 1 == true;
        }
    )";
    expect_semantic_failure(invalid_equality, "invalid_equality", "Cannot compare 'i32' with 'bool'");

    std::cout << "Semantic analyzer equality tests passed!" << std::endl;
    return 0;
}
