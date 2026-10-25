typedef struct {
    int a, b, c; // Values: -1, 0, 1 (mapped to TERN_LOW, TERN_MID, TERN_HIGH)
} TernaryNum;
static void ternary_addition(TernaryNum a, TernaryNum b, TernaryNum *result) {
    int carry = 0, sum;
    // Compute first trit with carry
    sum = a.a + b.a + carry;
    result->a = (sum % 3 + 3) % 3; // Normalize to 0, 1, 2 (TERN_LOW, TERN_MID, TERN_HIGH)
    carry = sum / 3;
    // Compute second trit
    sum = a.b + b.b + carry;
    result->b = (sum % 3 + 3) % 3;
    carry = sum / 3;
    // Compute third trit
    sum = a.c + b.c + carry;
    result->c = (sum % 3 + 3) % 3;
}
static void ternary_multiplication(TernaryNum a, TernaryNum b, TernaryNum *result) {
    result->a = (a.a * b.a) % 3;
    result->b = (a.b * b.b) % 3;
    result->c = (a.c * b.c) % 3;
}
static void ternary_negation(TernaryNum *a) {
    a->a = (a->a == 1) ? -1 : (a->a == -1) ? 1 : a->a;
    a->b = (a->b == 1) ? -1 : (a->b == -1) ? 1 : a->b;
    a->c = (a->c == 1) ? -1 : (a->c == -1) ? 1 : a->c;
}
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
