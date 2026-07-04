#include <iostream>

#include "ipc/version.hpp"

int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;

  std::cout << ipc::kPracticeVersion << " consumer " << '\n';
  return 0;
}
