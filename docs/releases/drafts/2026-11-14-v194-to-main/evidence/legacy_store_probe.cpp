#include "t81/canonfs/canon_driver.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
int main(int argc, char** argv) {
 if (argc != 2) return 2;
 const std::filesystem::path root(argv[1]);
 std::filesystem::create_directories(root / "objects");
 { std::ofstream out(root / "objects/legacy.blk"); out << "preserve-me"; }
 try { auto driver = t81::canonfs::make_persistent_driver(root); return 3; }
 catch (const std::runtime_error& error) {
  std::ifstream in(root / "objects/legacy.blk"); std::string bytes; in >> bytes;
  std::cout << error.what() << "; legacy bytes=" << bytes << "\n";
  return bytes == "preserve-me" && std::string(error.what()).find("explicit migration") != std::string::npos ? 0 : 4;
 }
}
