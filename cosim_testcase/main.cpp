#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <iostream>
#include "driver.hpp"

using namespace std;

int main() {
  char lower[] = "hello";
  char upper[] = "WORLD";
  char mixed[] = "risc-V";

  to_upper_case(lower, sizeof(lower));
  assert(string(lower) == "HELLO");
  std::cout << "[User function] hello >> HELLO complete " << std::endl;

  to_lower_case(upper, sizeof(upper));
  assert(string(upper) == "world");
  std::cout << "[User function] WORLD >> world complete " << std::endl;

  to_lower_case(mixed, sizeof(mixed));
  assert(string(mixed) == "risc-v");
  std::cout << "[User function] risc-V >> risc-v complete " << std::endl;

  to_upper_case(mixed, sizeof(mixed));
  assert(string(mixed) == "RISC-V");
  std::cout << "[User function] ricv-v >> RISC-V complete " << std::endl;

	printf("[User function] success!\n");
}
