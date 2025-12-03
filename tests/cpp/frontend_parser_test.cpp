#include "test_utils.hpp"
#include "t81/frontend/parser.hpp"

int main() {
    std::string source = R"(
        fn fib(n: i32) -> i32 {
            if (n < 2) {
                return n;
            }
            return fib(n - 1) + fib(n - 2);
        }
    )";
    Lexer lexer(source);
    Parser parser(lexer);
    auto stmts = parser.parse();

    assert(stmts.size() == 1);

    AstPrinter printer;
    std::string result = printer.print(*stmts[0]);
    std::string expected = "(fn fib (n: i32 ) -> i32 (block (if (< n 2) (block (return n))) (return (+ (call fib (- n 1)) (call fib (- n 2))))))";

    if (result != expected) {
        std::cerr << "Parser test failed!" << std::endl;
        std::cerr << "  Expected len: " << expected.length() << std::endl;
        std::cerr << "  Actual   len: " << result.length() << std::endl;
        std::cerr << "  Expected: \"" << expected << "\"" << std::endl;
        std::cerr << "  Actual:   \"" << result << "\"" << std::endl;
        return 1;
    }

    std::cout << "Parser test passed!" << std::endl;

    std::string loop_source = R"(
        @bounded(10)
        loop {
            let x: i32 = 0;
        }
    )";
    Lexer loop_lexer(loop_source);
    Parser loop_parser(loop_lexer);
    auto loop_stmts = loop_parser.parse();

    assert(loop_stmts.size() == 1);

    std::string loop_result = printer.print(*loop_stmts[0]);
    std::string loop_expected = "(loop @bounded(10) (block (let x: i32 = 0)))";

    if (loop_result != loop_expected) {
        std::cerr << "Loop parser test failed!" << std::endl;
        std::cerr << "  Expected: \"" << loop_expected << "\"" << std::endl;
        std::cerr << "  Actual:   \"" << loop_result << "\"" << std::endl;
        return 1;
    }

    std::cout << "Loop parser test passed!" << std::endl;

    return 0;
}
