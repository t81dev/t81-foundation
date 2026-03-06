#include <iostream>
#include <cstdlib>
#include <cstring>
#include <vector>

void test_env() {
    std::string val;
    bool has_val = false;
#ifdef _WIN32
    char* buf = nullptr;
    size_t sz = 0;
    if (_dupenv_s(&buf, &sz, "PATH") == 0 && buf != nullptr) {
        val = buf;
        has_val = true;
        free(buf);
    }
#else
    if (const char* v = std::getenv("PATH")) {
        val = v;
        has_val = true;
    }
#endif
    if (has_val) {
        std::cout << "PATH length: " << val.length() << std::endl;
    }
}

int main() {
    test_env();
    return 0;
}
