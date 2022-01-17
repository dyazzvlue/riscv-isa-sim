#include "rocc.h"
#include "mmu.h"
#include <cstring>

class str_transformer_rocc_insn_t : public rocc_t
{
 public:
  const char* name() { return "str_transformer_rocc"; }

  reg_t custom0(rocc_insn_t insn, reg_t xs1, reg_t xs2)
  {
    // do nothing 
      std::cout << "[str_transformer_rocc_t ] do nothing " <<std::endl;
    return 0;
  }

  str_transformer_rocc_insn_t()
  {
  }

 private:
};

REGISTER_EXTENSION(str_transformer_rocc_insn, []() { return new str_transformer_rocc_insn_t; })
