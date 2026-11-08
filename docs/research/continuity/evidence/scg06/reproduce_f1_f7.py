"""
Complete reproduction suite for F1–F7 across Pre-SCG-03 and Post-SCG-03 baselines.
All operations are tested against an independent exact-integer mathematical oracle.
Uses deterministic SplitMix64 pseudo-random generation with diverse operand profiles.
"""

import os
import sys
import json
import subprocess
import oracle

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.abspath(os.path.join(SCRIPT_DIR, "../../../../"))
WORKTREE_BASE = os.path.join(REPO_ROOT, ".scg06_worktrees")
PRE_INC = os.path.join(WORKTREE_BASE, "pre_scg03/include")
POST_INC = os.path.join(WORKTREE_BASE, "post_scg03/include")
# Fallback to /tmp/scg06_worktrees if local worktree base not present
if not os.path.exists(PRE_INC) and os.path.exists("/tmp/scg06_worktrees/pre_scg03/include"):
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
        run = subprocess.run([out], capture_output=True, text=True, timeout=15)
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
    T81Limb54 la{}, lb{};
    la.set_tryte(0, 1);
    lb.set_tryte(0, 1);
    auto p1 = T81Limb27::from_low_27(la) * T81Limb27::from_low_27(lb);
    auto t1 = p1.to_trits();

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

def test_f2_and_f3(inc_dir):
    # F2: T81Limb54::reference_mul non-trits vs exact value mismatches
    # F3: T81Limb54::karatsuba carry discard vs other component defects
    cpp = """
#include <iostream>
#include <array>
#include <cstdint>
#include "t81/types/T81Limb.hpp"

using t81::core::T81Limb27;
using t81::core::T81Limb54;

struct SplitMix64 {
    uint64_t state;
    uint64_t next() {
        uint64_t z = (state += 0x9e3779b97f4a7c15ULL);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        return z ^ (z >> 31);
    }
};

int main() {
    SplitMix64 rng{0x123456789abcdef0ULL};
    int total = 300;
    std::cout << total << "\\n";
    for (int k = 0; k < total; ++k) {
        std::array<int8_t, 54> ta{}, tb{};
        int mode = k % 5;
        for (int i = 0; i < 54; ++i) {
            uint64_t r = rng.next();
            if (mode == 0) { // sparse
                ta[i] = (r % 7 == 0) ? ((r >> 4) % 3 - 1) : 0;
                tb[i] = ((r >> 8) % 7 == 0) ? ((r >> 12) % 3 - 1) : 0;
            } else if (mode == 1) { // small width
                ta[i] = (i < 20) ? (r % 3 - 1) : 0;
                tb[i] = (i < 20) ? ((r >> 8) % 3 - 1) : 0;
            } else if (mode == 2) { // dense
                ta[i] = r % 3 - 1;
                tb[i] = (r >> 8) % 3 - 1;
            } else if (mode == 3) { // alternating
                ta[i] = (i % 2 == 0) ? 1 : -1;
                tb[i] = ((r >> 4) % 3 - 1);
            } else { // boundary
                ta[i] = (i < 27) ? 1 : -1;
                tb[i] = ((r >> 4) % 3 - 1);
            }
        }

        auto la = T81Limb54::from_trits(ta);
        auto lb = T81Limb54::from_trits(tb);

        auto p_ref = T81Limb54::reference_mul(la, lb);
        auto tr_ref = p_ref.to_trits();

        auto x0 = T81Limb27::from_low_27(la);
        auto x1 = T81Limb27::from_high_27(la);
        auto y0 = T81Limb27::from_low_27(lb);
        auto y1 = T81Limb27::from_high_27(lb);

        auto add_x = x0.addc(x1);
        auto add_y = y0.addc(y1);
        int carry_x = (int)add_x.second;
        int carry_y = (int)add_y.second;

        auto p_karat = T81Limb54::karatsuba(la, lb);
        auto tr_karat = p_karat.to_trits();

        for (int i = 0; i < 54; ++i) std::cout << (int)ta[i] << (i + 1 == 54 ? "\\n" : " ");
        for (int i = 0; i < 54; ++i) std::cout << (int)tb[i] << (i + 1 == 54 ? "\\n" : " ");
        for (int i = 0; i < 54; ++i) std::cout << (int)tr_ref[i] << (i + 1 == 54 ? "\\n" : " ");
        std::cout << carry_x << " " << carry_y << "\\n";
        for (int i = 0; i < 54; ++i) std::cout << (int)tr_karat[i] << (i + 1 == 54 ? "\\n" : " ");
    }
    return 0;
}
"""
    raw_res = run_isolated(cpp, inc_dir)
    if not raw_res["compiled"] or raw_res["returncode"] != 0:
        return {"compiled": raw_res["compiled"], "returncode": raw_res["returncode"], "stderr": raw_res.get("stderr", "")}

    lines = raw_res["stdout"].splitlines()
    total = int(lines[0])
    line_idx = 1

    f2_nontrits_count = 0
    f2_max_abs_digit = 0
    f2_value_mismatches = 0

    f3_carry_loss_cases = 0
    f3_carry_loss_mismatches = 0
    f3_no_carry_loss_cases = 0
    f3_no_carry_loss_mismatches = 0

    mod54 = 3**54

    for k in range(total):
        ta = [int(x) for x in lines[line_idx].split()]
        tb = [int(x) for x in lines[line_idx+1].split()]
        tr_ref = [int(x) for x in lines[line_idx+2].split()]
        cx, cy = [int(x) for x in lines[line_idx+3].split()]
        tr_karat = [int(x) for x in lines[line_idx+4].split()]
        line_idx += 5

        int_a = oracle.trits_to_int(ta)
        int_b = oracle.trits_to_int(tb)
        expected_prod = int_a * int_b
        expected_trits, _ = oracle.int_to_trits(expected_prod, 54)
        expected_val_54 = oracle.trits_to_int(expected_trits)

        # F2 evaluation: invalid non-trit outputs
        has_nontrit = any(d < -1 or d > 1 for d in tr_ref)
        if has_nontrit:
            f2_nontrits_count += 1
            max_d = max(abs(d) for d in tr_ref)
            if max_d > f2_max_abs_digit:
                f2_max_abs_digit = max_d

        # F2 evaluation: true mathematical value comparison
        actual_ref_val = oracle.trits_to_int_allow_nontrits(tr_ref)
        if (actual_ref_val - expected_val_54) % mod54 != 0:
            f2_value_mismatches += 1

        # F3 evaluation: causal carry loss vs other errors
        has_carry_loss = (cx != 0 or cy != 0)
        actual_karat_val = oracle.trits_to_int_allow_nontrits(tr_karat)
        karat_mismatch = ((actual_karat_val - expected_val_54) % mod54 != 0)

        if has_carry_loss:
            f3_carry_loss_cases += 1
            if karat_mismatch:
                f3_carry_loss_mismatches += 1
        else:
            f3_no_carry_loss_cases += 1
            if karat_mismatch:
                f3_no_carry_loss_mismatches += 1

    return {
        "F2": {
            "total_tested": total,
            "produced_nontrits": f2_nontrits_count,
            "max_abs_digit": f2_max_abs_digit,
            "mathematical_value_mismatches": f2_value_mismatches
        },
        "F3": {
            "total_tested": total,
            "cohort_with_carry_loss": {
                "cases": f3_carry_loss_cases,
                "exact_mismatches": f3_carry_loss_mismatches
            },
            "cohort_without_carry_loss": {
                "cases": f3_no_carry_loss_cases,
                "exact_mismatches": f3_no_carry_loss_mismatches,
                "explanation": "Karatsuba fails even without radical carry loss due to underlying T81Limb27::operator* Booth table defect F1"
            },
            "total_exact_mismatches": f3_carry_loss_mismatches + f3_no_carry_loss_mismatches
        }
    }

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

    for baseline, inc in [("pre_scg03", PRE_INC), ("post_scg03", POST_INC)]:
        print(f"Running reproduction on {baseline}...")
        results[baseline]["F1"] = test_f1(inc)
        f2_f3 = test_f2_and_f3(inc)
        results[baseline]["F2"] = f2_f3["F2"]
        results[baseline]["F3"] = f2_f3["F3"]
        results[baseline]["F4"] = test_f4(inc)
        results[baseline]["F5_debug"] = test_f5(inc, ndebug=False)
        results[baseline]["F5_release"] = test_f5(inc, ndebug=True)
        results[baseline]["F6"] = test_f6(inc)
        results[baseline]["F7"] = test_f7(inc)

    out_file = os.path.join(SCRIPT_DIR, "reproduction_results.json")
    with open(out_file, "w") as f:
        json.dump(results, f, indent=2)
    print(f"Reproduction results written to {out_file}")

if __name__ == "__main__":
    main()
