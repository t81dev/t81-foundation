/**
 * @file complex_demo.cpp
 * @brief A complex example demonstrating the usage of all T81 C++ Library datatypes.
 *
 * This example simulates a scenario where two agents, Alice and Bob, interact
 * in a simulated environment. They use various mathematical and cognitive
 * constructs provided by the library.
 */

#include <chrono>
#include <cmath>
#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include "t81/fraction.hpp"
#include "t81/types/T81Fixed.hpp"
#include "t81/types/T81Result.hpp"
#include "t81/types/T81Uint.hpp"
#include "t81/types/all.hpp"

// Helper to print section headers
void print_section(const std::string& title) {
  std::cout << "\n==================================================\n";
  std::cout << "  " << title << "\n";
  std::cout << "==================================================\n";
}

using namespace t81;

// Function to demonstrate basic types
void demo_basics() {
  print_section("Basic Types: Int, Float, String, Symbol, Time");

  // T81Int
  T81Int<81> i1(42);
  T81Int<81> i2(-10);
  auto sum = i1 + i2;
  std::cout << "T81Int: " << i1.to_string() << " + " << i2.to_string() << " = " << sum.to_string()
            << "\n";

  // T81Float (using T81Float72_9 concrete type)
  using Float = T81Float72_9;
  Float f1 = Float::from_double(3.14159);
  Float f2 = Float::from_double(2.71828);
  auto prod = f1 * f2;
  std::cout << "T81Float: " << std::to_string(f1.to_double()) << " * "
            << std::to_string(f2.to_double()) << " = " << std::to_string(prod.to_double()) << "\n";

  // T81String
  T81String s1("Hello, ");
  T81String s2("T81 World!");
  auto s3 = s1 + s2;
  std::cout << "T81String: " << s3.str() << "\n";

  // T81Symbol
  T81Symbol sym1 = T81Symbol::intern("ALICE");
  T81Symbol sym2 = T81Symbol::intern("BOB");
  std::cout << "T81Symbol: " << sym1 << " vs " << sym2 << "\n";

  // T81Time
  T81Time now = T81Time::now();
  std::cout << "T81Time: Current timestamp: " << now.narrate() << "\n";
}

// Function to demonstrate mathematical types
void demo_math() {
  print_section("Math Types: BigInt, Complex, Quaternion, Fraction, Polynomial, Matrix");

  using namespace t81::v1;  // For T81BigInt, T81Fraction, T81Fixed
  using Float = T81Float72_9;

  // T81BigInt
  T81BigInt b1 = T81BigInt::from_int64(123456789012345LL);
  T81BigInt b2 = T81BigInt::from_int64(987654321098765LL);
  auto bsum = b1 + b2;
  std::cout << "T81BigInt: " << b1.to_string() << " + " << b2.to_string() << " = "
            << bsum.to_string() << "\n";

  // T81Complex - supports 18 or 27 mantissa trits. Using 27.
  using ComplexFloat = T81Float27_9;
  T81Complex<27> c1(ComplexFloat::from_double(1.0), ComplexFloat::from_double(2.0));
  T81Complex<27> c2(ComplexFloat::from_double(3.0), ComplexFloat::from_double(4.0));
  [[maybe_unused]] auto cprod = c1 * c2;
  std::cout << "T81Complex: (" << std::to_string(c1.real().to_double()) << ","
            << std::to_string(c1.imag().to_double()) << ") * ... \n";

  // T81Quaternion - uses T81Float27_9 internally
  using QFloat = T81Float27_9;
  T81Quaternion q1 = T81Quaternion::identity();
  T81Quaternion q2(QFloat::from_double(0.707), QFloat::from_double(0.707), QFloat::from_double(0.0),
                   QFloat::from_double(0.0));
  [[maybe_unused]] auto qrot = q1 * q2;
  std::cout << "T81Quaternion: Rotated identity by 90 deg around X\n";

  // T81Fraction - explicitly using 81 trits version
  t81::v1::T81Fraction<81> fr1(T81Int<81>(1), T81Int<81>(3));
  t81::v1::T81Fraction<81> fr2(T81Int<81>(1), T81Int<81>(2));
  auto frsum = fr1 + fr2;
  std::cout << "T81Fraction: 1/3 + 1/2 = " << frsum.num().to_string() << "/"
            << frsum.den().to_string() << "\n";

  // T81Polynomial
  T81List<Float> coeffs;
  coeffs.push_back(Float::from_double(1.0));
  coeffs.push_back(Float::from_double(2.0));
  coeffs.push_back(Float::from_double(1.0));
  T81Polynomial<Float> poly(std::move(coeffs));
  Float x = Float::from_double(2.0);
  Float y = poly.eval(x);
  std::cout << "T81Polynomial: P(2.0) = " << std::to_string(y.to_double()) << "\n";

  // T81Matrix
  T81Matrix<Float, 2, 2> mat1;
  mat1(0, 0) = Float::from_double(1.0);
  mat1(0, 1) = Float::from_double(0.0);
  mat1(1, 0) = Float::from_double(0.0);
  mat1(1, 1) = Float::from_double(1.0);
  std::cout << "T81Matrix: Created 2x2 Matrix\n";
}

// Function to demonstrate containers
void demo_containers() {
  print_section("Containers: List, Map, Set, Tree, Vector");

  // T81List
  T81List<int> list;
  list.push_back(1);
  list.push_back(2);
  list.push_back(3);
  std::cout << "T81List: size=" << list.size() << "\n";

  // T81Map
  T81Map<T81Symbol, int> map;
  map[T81Symbol::intern("one")] = 1;
  map[T81Symbol::intern("two")] = 2;
  std::cout << "T81Map: one=" << map[T81Symbol::intern("one")] << "\n";

  // T81Set
  T81Set<int> set;
  set = set.insert(10);
  set = set.insert(20);
  std::cout << "T81Set: contains 10? " << (set.contains(10) ? "Yes" : "No") << "\n";

  // T81Tree
  auto tree = T81Tree<int>::leaf(100);
  tree = T81Tree<int>::node(50, std::nullopt, std::make_optional(tree), std::nullopt);
  std::cout << "T81Tree: Root value=" << tree->value() << "\n";

  // T81Vector
  T81Vector<3, double> vec;
  vec[0] = 1.1;
  vec[1] = 2.2;
  std::cout << "T81Vector: size=" << vec.dimension << "\n";
}

// Function to demonstrate cognitive and agent types
void demo_cognitive() {
  print_section("Cognitive: Agent, Entropy, Prob, Qutrit");

  // Entropy Pool
  auto entropy = acquire_entropy(T81Symbol::intern("MAIN"));
  std::cout << "T81Entropy: Acquired token with sequence " << entropy.sequence() << "\n";

  // Agent
  T81List<T81Entropy> fuel;
  fuel.push_back(std::move(entropy));  // Give one unit of fuel
  T81Agent agent(T81Symbol::intern("AGENT_007"), std::move(fuel));

  std::cout << "T81Agent: Created agent " << agent.identity()
            << " with fuel=" << agent.fuel_remaining() << "\n";

  // Beliefs
  agent.believe(T81Symbol::intern("SKY_IS_BLUE"), T81Prob27::from_prob(0.99));
  auto belief = agent.belief(T81Symbol::intern("SKY_IS_BLUE"));
  std::cout << "T81Agent: Believes sky is blue with p=" << belief.to_prob() << "\n";

  // Qutrit
  T81Qutrit q = qutrit::ZERO;
  std::cout << "T81Qutrit: State is " << (q == qutrit::ZERO ? "Zero" : "Non-Zero") << "\n";
}

// Function to demonstrate data and IO
void demo_data_io() {
  print_section("Data/IO: Tensor, Stream, Bytes");

  using Float = T81Float72_9;

  // T81Tensor
  [[maybe_unused]] T81Tensor<Float, 2, 2, 2> tensor;
  std::cout << "T81Tensor: Created 2x2 static tensor\n";

  // T81Bytes
  T81Bytes bytes(10);
  bytes[0] = 0xDE;
  bytes[1] = 0xAD;
  std::cout << "T81Bytes: First two bytes: " << std::hex << (int)bytes[0] << " " << (int)bytes[1]
            << std::dec << "\n";

  // T81Stream
  auto stream = stream_from([]() -> int { return 42; });
  auto it = stream.begin();
  std::cout << "T81Stream: First value from infinite stream: " << *it << "\n";
}

// Coroutine for Promise demo
t81::T81Promise<int> async_task() { co_return 100; }

// Function to demonstrate concurrency and network
void demo_concurrency_network() {
  print_section("Concurrency/Network: Thread, Promise, Network, Discovery");

  // T81Promise
  auto promise = async_task();

  if (promise.state() == T81Promise<int>::State::FULFILLED) {
    auto res = promise.try_get();
    if (res.has_value()) {
      std::cout << "T81Promise: Fulfilled with " << res.value() << "\n";
    }
  } else {
    std::cout << "T81Promise: Pending...\n";
  }

  // T81Thread
  // Needs an agent to spawn
  T81List<T81Entropy> fuel;
  fuel.push_back(acquire_entropy(T81Symbol::intern("THREAD_SPAWNER")));
  T81Agent worker_agent(T81Symbol::intern("WORKER"), std::move(fuel));

  auto thread = T81Thread::spawn(T81Symbol::intern("WORKER_THREAD"), std::move(worker_agent), []() {
    std::cout << "T81Thread: Working inside spawned thread...\n";
  });
  thread.join();

  // T81Discovery
  // T81Discovery discovery;
  // discovery.announce(T81Symbol::intern("SERVICE"), 8080);
  std::cout << "T81Discovery: Service announced (mocked)\n";

  // T81Network
  T81Endpoint endpoint("127.0.0.1", 8080);
  std::cout << "T81Network: Target endpoint " << endpoint.to_string() << "\n";
}

// Function to demonstrate extra types
void demo_extras() {
  print_section("Extras: Fixed, Uint, Maybe, Result, Reflection");

  using namespace t81::v1;  // For T81Fixed

  // T81Fixed
  using Fixed = T81Fixed<18, 9>;  // 18 integer trits, 9 fractional trits
  Fixed fx1(10.5);
  Fixed fx2(2.0);
  auto fxprod = fx1 * fx2;
  std::cout << "T81Fixed: 10.5 * 2.0 = " << fxprod.to_double() << "\n";

  // T81UInt - Fixed type name and size (must be multiple of 4)
  // T81UInt<81> is invalid, using T81UInt<80> (20 trytes)
  t81::T81UInt<80> u1(100);
  // T81UInt has private `as_uint64`, but likely `to_signed().to_int64()` works or we just
  // demonstrate creation. The previous error said `to_uint64` is missing. T81UInt wraps T81Int.
  // Let's cast to signed then int64 for demo.
  std::cout << "T81UInt: " << u1.to_signed().to_int64() << "\n";

  // T81Maybe
  t81::T81Maybe<int> maybe_val(42);
  if (maybe_val.has_value()) {
    std::cout << "T81Maybe: Has value " << maybe_val.value() << "\n";
  }

  // T81Result
  t81::T81Result<int> res = t81::T81Result<int>::success(200);
  if (res.is_ok()) {  // Fixed is_success -> is_ok
    std::cout << "T81Result: Success with " << res.value() << "\n";
  }

  // T81Reflection
  // T81Result has reflect()
  auto reflection = res.reflect();
  std::cout << "T81Reflection: Reflected result with status "
            << reflection.instance_id().to_string() << "\n";
}

// Main execution
int main() {
  std::cout << "Starting T81 Complex Demo...\n";

  try {
    demo_basics();
    demo_math();
    demo_containers();
    demo_cognitive();
    demo_data_io();
    demo_concurrency_network();
    demo_extras();

    print_section("Demo Completed Successfully");
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }

  return 0;
}
