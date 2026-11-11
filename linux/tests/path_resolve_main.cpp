#include "linux_port/path_util.h"

#include <filesystem>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "Usage: fl_path_resolve <base-dir> <path> [--allow-missing-tail]\n";
    std::cerr << "Example: fl_path_resolve . ..\\DATA\\solar\\solararch.ini\n";
    return 2;
  }

  const std::filesystem::path base = argv[1];
  const std::string input = argv[2];
  fl_port::ResolveOptions opts;
  for (int i = 3; i < argc; ++i) {
    if (std::string(argv[i]) == "--allow-missing-tail") opts.allow_missing_tail = true;
  }

  std::cout << "normalized: " << fl_port::normalize_path_separators(input) << "\n";
  auto resolved = fl_port::resolve_path_case_insensitive(base, input, opts);
  if (!resolved) {
    std::cout << "resolved: <not found>\n";
    return 1;
  }
  std::cout << "resolved: " << resolved->string() << "\n";
  return 0;
}
