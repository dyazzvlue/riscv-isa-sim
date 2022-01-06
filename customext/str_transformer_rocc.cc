#include "rocc.h"
#include "mmu.h"
#include <cstring>

class str_transformer_rocc_t : public rocc_t
{
 public:
  const char* name() { return "str_transformer_rocc"; }

  reg_t custom0(rocc_insn_t insn, reg_t xs1, reg_t xs2)
  {
    // do nothing 
    return 0;
  }

  dummy_rocc_t()
  {
  }

 private:
};

REGISTER_EXTENSION(str_transformer_rocc, []() { return new str_transformer_rocc_t; })