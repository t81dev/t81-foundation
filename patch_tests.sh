sed -i 's/f.exp();/auto r = f.exp(); (void)r;/g' tests/determinism_cross_platform.cpp
sed -i 's/e1.get()/e1.value()/g' tests/determinism_cross_platform.cpp
sed -i 's/e2.get()/e2.value()/g' tests/determinism_cross_platform.cpp
sed -i 's/e3.get()/e3.value()/g' tests/determinism_cross_platform.cpp
sed -i 's/e4.get()/e4.value()/g' tests/determinism_cross_platform.cpp
