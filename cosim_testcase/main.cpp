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

  to_lower_case(upper, sizeof(upper));
  assert(string(upper) == "world");

  to_lower_case(mixed, sizeof(mixed));
  assert(string(mixed) == "risc-v");

  to_upper_case(mixed, sizeof(mixed));
  assert(string(mixed) == "RISC-V");

	printf("success!\n");
}