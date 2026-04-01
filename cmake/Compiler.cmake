include_guard(GLOBAL)

if(T81_TRITWISE_PROFILE)
  add_compile_definitions(T81_TRITWISE_PROFILE)
endif()

if(T81_STRICT_DETERMINISTIC_FLOAT)
  add_compile_definitions(T81_DETERMINISTIC)
endif()

if(T81_USE_CXX23)
  set(T81_CXX_STD_FEATURE cxx_std_23)
else()
  set(T81_CXX_STD_FEATURE cxx_std_20)
endif()

# Global compiler flags (ANSI warnings for GCC/Clang; use /W4 for MSVC)
if(MSVC)
  add_compile_options(/W4 /O2 /DNDEBUG /WX)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -O3 -DNDEBUG)
endif()

if(MSVC)
  add_compile_options(/wd4244 /wd4307 /wd4018 /wd4101 /wd4189 /wd4996 /wd4456)
endif()

# warn-strict mode: suppress -Wextra subwarnings that are not Windows CI issues.
# These must come AFTER -Wall/-Wextra above so they take effect.
if(T81_WARN_STRICT AND NOT MSVC)
  add_compile_options(-Wno-unused-parameter -Wno-missing-field-initializers)
endif()
