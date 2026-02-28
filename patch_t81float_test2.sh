sed -i 's/try { check/try { check/g' tests/cpp/test_T81Float.cpp
sed -i 's/catch(const std::domain_errorcheck(/catch(const std::domain_error\& e) { } \/\/check(/g' tests/cpp/test_T81Float.cpp
