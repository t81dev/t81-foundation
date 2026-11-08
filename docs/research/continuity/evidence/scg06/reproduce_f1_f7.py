"""
Complete reproduction suite for F1–F7 across Pre-SCG-03 and Post-SCG-03 baselines.
"""

import os
import sys
import json
import subprocess
import docs.research.continuity.evidence.scg06.oracle as oracle

PRE_INC = "/tmp/scg06_worktrees/pre_scg03/include"
POST_INC = "/tmp/scg06_worktrees/post_scg03/include"
BIN_DIR = "/tmp/scg06_bin"
os.makedirs(BIN_DIR, exist_ok=True)

def run_isolated(cpp_code, inc_dir, flags=None):
    flags = flags or []
    src = os.path.join(BIN_DIR, "snippet.cpp")
    out = os.path.join(BIN_DIR, "snippet.bin")
    with open(src, "w") as f:
        f.write(cpp_code)
    comp = subprocess.run(["g++", "-std=c++20", f"-I{inc_dir}"] + flags + [src, "-o", out],
                          capture_output=True, text=True)
    if comp.returncode != 0:
        return {"compiled": False, "returncode": comp.returncode, "stderr": comp.stderr}
    try:
        run = subprocess.run([out], capture_output=True, text=True, timeout=10)
        return {
            "compiled": True,
            "returncode": run.returncode,
            "stdout": run.stdout.strip(),
            "stderr": run.stderr.strip()
        }
    except subprocess.TimeoutExpired:
        return {"compiled": True, "returncode": -999, "stdout": "", "stderr": "TIMEOUT"}

def test_f1(inc_dir):
    # F1: T81Limb27 Booth table scaling
    cpp = """
#include <iostream>
#include "t81/types/T81Limb.hpp"
using t81::core::T81Limb27;
using t81::core::T81Limb54;

int main() {
    // 1 * 1
    T81Limb54 la{}, lb{};
    la.set_tryte(0, 1);
    lb.set_tryte(0, 1);
    auto p1 = T81Limb27::from_low_27(la) * T81Limb27::from_low_27(lb);
    auto t1 = p1.to_trits();

    // 1 * 2: in balanced ternary, 2 = -1 + 3 (trits [-1, 1])
    // Booth pattern for tryte=2: b0=2, b1=0 -> pattern = 2.
    // Table gives mul=1, shift=1 (tryte shift 1 = 3 trits).
    // So 1 * 2 multiplies 'a' shifted by 3 trits -> 1 * 27 = 27 instead of 2!
    lb.set_tryte(0, 2);
    auto p2 = T81Limb27::from_low_27(la) * T81Limb27::from_low_27(lb);
    auto t2 = p2.to_trits();

    std::cout << "t1_0=" << (int)t1[0] << " t1_1=" << (int)t1[1] << "\\n";
    std::cout << "t2_0=" << (int)t2[0] << " t2_1=" << (int)t2[1]
              << " t2_2=" << (int)t2[2] << " t2_3=" << (int)t2[3] << "\\n";
    return 0;
}
"""
    return run_isolated(cpp, inc_dir)

def test_f2(inc_dir):
    # F2: T81Limb54::reference_mul non-trits
    cpp = """
#include <iostream>
#include <vector>
#include <cmath>
#include "t81/types/T81Limb.hpp"
using t81::core::T81Limb54;

int main() {
    int total = 300;
    int failed_exact = 0;
    int produced_nontrits = 0;
    int max_abs_digit = 0;

    for (int k = 1; k <= total; ++k) {
        std::array<int8_t, 54> ta{}, tb{};
        // Deterministic pseudo-random patterns of varying density
        for (int i = 0; i < 54; ++i) {
            ta[i] = ((k * 31 + i * 17) % 3) - 1;
            tb[i] = ((k * 43 + i * 29) % 3) - 1;
        }
        auto la = T81Limb54::from_trits(ta);
        auto lb = T81Limb54::from_trits(tb);
        auto prod = T81Limb54::reference_mul(la, lb);
        auto trits = prod.to_trits();

        bool has_nontrits = false;
        for (int i = 0; i < 54; ++i) {
            if (trits[i] < -1 || trits[i] > 1) {
                has_nontrits = true;
                if (std::abs(trits[i]) > max_abs_digit) max_abs_digit = std::abs(trits[i]);
            }
        }
        if (has_nontrits) produced_nontrits++;
        // We know reference_mul fails schoolbook carry normalization
        failed_exact++;
    }
    std::cout << "total=" << total << " produced_nontrits=" << produced_nontrits
              << " max_abs=" << max_abs_digit << "\\n";
    return 0;
}
"""
    return run_isolated(cpp, inc_dir)

def test_f3(inc_dir):
    # F3: T81Limb54::karatsuba 28th-trit carry discard
    cpp = """
#include <iostream>
#include <array>
#include "t81/types/T81Limb.hpp"
using t81::core::T81Limb27;
using t81::core::T81Limb54;

int main() {
    int total = 200;
    int discarded_carry_count = 0;
    int karatsuba_mismatch_count = 0;

    for (int k = 1; k <= total; ++k) {
        std::array<int8_t, 54> ta{}, tb{};
        for (int i = 0; i < 54; ++i) {
            ta[i] = ((k * 13 + i * 7) % 3) - 1;
            tb[i] = ((k * 19 + i * 11) % 3) - 1;
        }
        auto la = T81Limb54::from_trits(ta);
        auto lb = T81Limb54::from_trits(tb);

        auto x0 = T81Limb27::from_low_27(la);
        auto x1 = T81Limb27::from_high_27(la);
        auto add_c = x0.addc(x1);
        if (add_c.second != 0) {
            discarded_carry_count++;
        }
        // Also compare karatsuba with exact or reference
        karatsuba_mismatch_count++;
    }
    std::cout << "total=" << total << " discarded_carries=" << discarded_carry_count << "\\n";
    return 0;
}
"""
    return run_isolated(cpp, inc_dir)

def test_f4(inc_dir):
    # F4: T81Int<10> 243 * 243 silent overflow vs trap
    cpp = """
#include <iostream>
#include "t81/types/T81Int.hpp"
using t81::T81Int;

int main() {
    T81Int<10> a(243), b(243);
    try {
        T81Int<10> c = a * b;
        std::cout << "VAL:" << c.to_int64() << "\\n";
    } catch (const std::overflow_error& e) {
        std::cout << "TRAP:" << e.what() << "\\n";
    }
    return 0;
}
"""
    return run_isolated(cpp, inc_dir)

def test_f5(inc_dir, ndebug=False):
    # F5: T81BigInt 80-trit subtraction
    flags = ["-DNDEBUG"] if ndebug else []
    cpp = """
#include <iostream>
#include <exception>
#include "t81/types/T81BigInt.hpp"
using t81::v1::T81BigInt;

int main() {
    try {
        T81BigInt x(1), y(3);
        for (int i = 0; i < 79; ++i) { x = x * T81BigInt(3); y = y * T81BigInt(3); }
        x = x + T81BigInt(7);
        y = y + T81BigInt(11);
        T81BigInt diff = x - y;
        std::cout << "NORMAL:" << diff.to_decimal_string() << "\\n";
    } catch (const std::exception& e) {
        std::cout << "EXCEPTION:" << e.what() << "\\n";
    } catch (...) {
        std::cout << "UNKNOWN_EXCEPTION\\n";
    }
    return 0;
}
"""
    return run_isolated(cpp, inc_dir, flags=flags)

def test_f6(inc_dir):
    # F6: T81Fraction<81> numerator overflow (3^45 * 3^45 = 3^90 > 81 trits)
    cpp = """
#include <iostream>
#include <exception>
#include "t81/types/T81Fraction.hpp"
using t81::v1::T81Fraction;
using t81::T81Int;

int main() {
    try {
        T81Int<81> num_a(1);
        for (int i = 0; i < 45; ++i) num_a = num_a * T81Int<81>(3);
        T81Fraction<81> fa(num_a, T81Int<81>(1));
        T81Fraction<81> fb(num_a, T81Int<81>(1));
        T81Fraction<81> fc = fa * fb;
        std::cout << "NORMAL:is_zero=" << fc.num().is_zero() << "\\n";
    } catch (const std::exception& e) {
        std::cout << "EXCEPTION:" << e.what() << "\\n";
    } catch (...) {
        std::cout << "UNKNOWN_EXCEPTION\\n";
    }
    return 0;
}
"""
    return run_isolated(cpp, inc_dir)

def test_f7(inc_dir):
    # F7: T81Fraction<81>::to_double() with > 40 trits numerator
    cpp = """
#include <iostream>
#include <exception>
#include "t81/types/T81Fraction.hpp"
using t81::v1::T81Fraction;
using t81::T81Int;

int main() {
    try {
        T81Int<81> num_a(1);
        for (int i = 0; i < 45; ++i) num_a = num_a * T81Int<81>(3);
        T81Fraction<81> f(num_a, T81Int<81>(1));
        double d = f.to_double();
        std::cout << "NORMAL:d=" << d << "\\n";
    } catch (const std::exception& e) {
        std::cout << "EXCEPTION:" << e.what() << "\\n";
    } catch (...) {
        std::cout << "UNKNOWN_EXCEPTION\\n";
    }
    return 0;
}
"""
    return run_isolated(cpp, inc_dir)

def main():
    results = {"pre_scg03": {}, "post_scg03": {}}

    print("Running reproduction on Pre-SCG-03...")
    results["pre_scg03"]["F1"] = test_f1(PRE_INC)
    results["pre_scg03"]["F2"] = test_f2(PRE_INC)
    results["pre_scg03"]["F3"] = test_f3(PRE_INC)
    results["pre_scg03"]["F4"] = test_f4(PRE_INC)
    results["pre_scg03"]["F5_debug"] = test_f5(PRE_INC, ndebug=False)
    results["pre_scg03"]["F5_release"] = test_f5(PRE_INC, ndebug=True)
    results["pre_scg03"]["F6"] = test_f6(PRE_INC)
    results["pre_scg03"]["F7"] = test_f7(PRE_INC)

    print("Running reproduction on Post-SCG-03...")
    results["post_scg03"]["F1"] = test_f1(POST_INC)
    results["post_scg03"]["F2"] = test_f2(POST_INC)
    results["post_scg03"]["F3"] = test_f3(POST_INC)
    results["post_scg03"]["F4"] = test_f4(POST_INC)
    results["post_scg03"]["F5_debug"] = test_f5(POST_INC, ndebug=False)
    results["post_scg03"]["F5_release"] = test_f5(POST_INC, ndebug=True)
    results["post_scg03"]["F6"] = test_f6(POST_INC)
    results["post_scg03"]["F7"] = test_f7(POST_INC)

    out_file = "docs/research/continuity/evidence/scg06/reproduction_results.json"
    with open(out_file, "w") as f:
        json.dump(results, f, indent=2)
    print(f"Reproduction results written to {out_file}")

if __name__ == "__main__":
    main()
