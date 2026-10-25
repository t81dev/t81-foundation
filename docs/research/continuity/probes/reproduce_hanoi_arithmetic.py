#!/usr/bin/env python3
"""Collect isolated counterexamples from pinned CWEB bodies; not a Hanoi build."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

PIN = "59205d76db4aca3da7c0bdc521924b8f9bc52cc3"
SOURCE = "src/ternary_arithmetic_optimization.cweb"


def function_body(text, name):
    match = re.search(r"static void " + name + r"\([^)]*\)\s*\{", text)
    if not match:
        raise ValueError("function not found: " + name)
    end, depth = match.end(), 1
    while depth:
        if text[end] == "{":
            depth += 1
        elif text[end] == "}":
            depth -= 1
        end += 1
    return text[match.start():end], text[:match.start()].count("\n") + 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--hanoi-root", required=True, type=Path)
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--cc", default="clang")
    args = parser.parse_args()
    text = subprocess.check_output(
        ["git", "-C", str(args.hanoi_root), "show", PIN + ":" + SOURCE]).decode("utf-8")
    typedef = re.search(r"typedef struct\s*\{[^}]*\}\s*TernaryNum;", text).group()
    bodies, sections = [], []
    for name in ["ternary_addition", "ternary_multiplication", "ternary_negation"]:
        body, line = function_body(text, name)
        bodies.append(body)
        sections.append({"name": name, "line": line,
                         "sha256": hashlib.sha256(body.encode()).hexdigest()})
    driver = r'''
#include <stdio.h>
static int value(TernaryNum x) { return x.a + 3*x.b + 9*x.c; }
static int closed(TernaryNum x) {
  return x.a >= -1 && x.a <= 1 && x.b >= -1 && x.b <= 1 && x.c >= -1 && x.c <= 1;
}
int main(void) {
  TernaryNum xs[27]; int n = 0;
  for (int a=-1;a<=1;a++) for (int b=-1;b<=1;b++) for (int c=-1;c<=1;c++)
    xs[n++] = (TernaryNum){a,b,c};
  int add_cases=0, add_fail=0, mul_cases=0, mul_fail=0, neg_fail=0;
  for (int i=0;i<n;i++) {
    TernaryNum z=xs[i]; ternary_negation(&z);
    if (!closed(z) || value(z) != -value(xs[i])) neg_fail++;
    for (int j=0;j<n;j++) {
      int sum=value(xs[i])+value(xs[j]), product=value(xs[i])*value(xs[j]);
      if (sum >= -13 && sum <= 13) {
        add_cases++; ternary_addition(xs[i],xs[j],&z);
        if (!closed(z) || value(z) != sum) add_fail++;
      }
      if (product >= -13 && product <= 13) {
        mul_cases++; ternary_multiplication(xs[i],xs[j],&z);
        if (!closed(z) || value(z) != product) mul_fail++;
      }
    }
  }
  TernaryNum z;
  ternary_addition((TernaryNum){-1,0,0},(TernaryNum){0,0,0},&z);
  printf("{\"addition_cases\":%d,\"addition_failures\":%d,",add_cases,add_fail);
  printf("\"negative_one_plus_zero\":[%d,%d,%d],",z.a,z.b,z.c);
  ternary_multiplication((TernaryNum){0,1,0},(TernaryNum){1,0,0},&z);
  printf("\"multiplication_cases\":%d,\"multiplication_failures\":%d,",mul_cases,mul_fail);
  printf("\"three_times_one\":[%d,%d,%d],\"negation_cases\":27,\"negation_failures\":%d}\n",z.a,z.b,z.c,neg_fail);
  return 0;
}
'''
    out = args.output_dir
    out.mkdir(parents=True, exist_ok=True)
    source, binary = out / "isolated_hanoi_arithmetic.c", out / "isolated_hanoi_arithmetic"
    source.write_text(typedef + "\n" + "\n".join(bodies) + driver)
    command = [args.cc, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror",
               str(source), "-o", str(binary)]
    subprocess.run(command, check=True)
    result = json.loads(subprocess.check_output([str(binary)], text=True))
    assert result["negative_one_plus_zero"] == [2, 0, 0]
    assert result["three_times_one"] == [0, 0, 0]
    assert result["negation_failures"] == 0
    result.update(source_commit=PIN, source_path=SOURCE, extracted_sections=sections,
                  source_sha256=hashlib.sha256(text.encode()).hexdigest(),
                  compiler=subprocess.check_output([args.cc, "--version"], text=True).splitlines()[0],
                  compile_command=command,
                  scope="verbatim isolated functions, balanced inputs, only exact results in [-13,13]; no CWEB module/VM build",
                  oracle="ordinary integer addition/multiplication of a+3b+9c; closure in {-1,0,1} required",
                  candidate_passes=False,
                  exit_zero_means="data collection and expected counterexample verification completed")
    (out / "hanoi-arithmetic-results.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
