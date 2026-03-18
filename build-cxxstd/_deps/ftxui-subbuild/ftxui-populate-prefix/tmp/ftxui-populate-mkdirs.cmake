# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/app/build-cxxstd/_deps/ftxui-src"
  "/app/build-cxxstd/_deps/ftxui-build"
  "/app/build-cxxstd/_deps/ftxui-subbuild/ftxui-populate-prefix"
  "/app/build-cxxstd/_deps/ftxui-subbuild/ftxui-populate-prefix/tmp"
  "/app/build-cxxstd/_deps/ftxui-subbuild/ftxui-populate-prefix/src/ftxui-populate-stamp"
  "/app/build-cxxstd/_deps/ftxui-subbuild/ftxui-populate-prefix/src"
  "/app/build-cxxstd/_deps/ftxui-subbuild/ftxui-populate-prefix/src/ftxui-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/app/build-cxxstd/_deps/ftxui-subbuild/ftxui-populate-prefix/src/ftxui-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/app/build-cxxstd/_deps/ftxui-subbuild/ftxui-populate-prefix/src/ftxui-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
