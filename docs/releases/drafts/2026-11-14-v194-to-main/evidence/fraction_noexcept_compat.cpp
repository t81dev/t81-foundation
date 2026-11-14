#include "t81/types/T81Fraction.hpp"
using Fraction = t81::v1::T81Fraction<13>;
using OldPlusContract = Fraction (Fraction::*)(const Fraction&) const noexcept;
OldPlusContract consumer = &Fraction::operator+;
int main() { return consumer == nullptr; }
