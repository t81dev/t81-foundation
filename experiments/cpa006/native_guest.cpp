#include <cstdio>
#include "cpa006_evaluator.hpp"
static void emit(const char* s) { std::fputs(s,stdout); }
int main() { cpa006_run(emit); }
