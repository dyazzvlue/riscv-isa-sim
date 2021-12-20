// See LICENSE for license details.

#include "rocc.h"
#include "trap.h"
#include <cstdlib>
#include <iostream>
#include "insn_macros.h"
#include "cosim_fence.h"
#include "spike_event.h"

#define customX(n) \
  static reg_t c##n(processor_t* p, insn_t insn, reg_t pc) \
  { \
    std::cout << "[rocc.cc] customX " << std::endl;\
    rocc_t* rocc = static_cast<rocc_t*>(p->get_extension()); \
    rocc_insn_union_t u; \
    u.i = insn; \
    reg_t xs1 = u.r.xs1 ? RS1 : -1; \
    reg_t xs2 = u.r.xs2 ? RS2 : -1; \
    reg_t xd = rocc->custom##n(u.r, xs1, xs2); \
    if (u.r.xd) \
      WRITE_RD(xd); \
    return pc+4; \
  } \
  \
  reg_t rocc_t::custom##n(rocc_insn_t insn, reg_t xs1, reg_t xs2) \
  { \
    std::cout << "[rocc.cc] illegal" << std::endl;\
    illegal_instruction(); \
    return 0; \
  }

customX(0)
customX(1)
customX(2)
customX(3)

#define cosim_customX(n) \
  static reg_t cosim_c##n(processor_t* p, insn_t insn, reg_t pc) \
  { \
    std::cout << "[rocc.cc] cosim_customX " << std::endl;\
    /* check if the cosim_fence is created.
     * If created, waiting for execution by cosim model
     * Else create a new cosim fence*/  \
    \
    cosim_fence_t* cosim_fence = static_cast<cosim_fence_t*> \
                                    (p->find_insn_in_cosim_fence_table(insn)); \
    if (cosim_fence == NULL) { \
       /* create a new cosim fence */ \
       std::cout << "[rocc.cc] create new rocc insn event " << std::endl; \
       spike_event_t* event = sc_cosim::createRoccInsnEvent(); \
       cosim_fence = new cosim_fence_t(insn); \
       cosim_fence->set_spike_event(event); \
       p->add_cosim_fence(cosim_fence); \
       return pc; \
    }else {\
       spike_event_t* event = cosim_fence->get_spike_event(); \
           if (event->isFinished()){ \
            rocc_t* rocc = static_cast<rocc_t*>(p->get_extension()); \
            rocc_insn_union_t u; \
            u.i = insn; \
            reg_t xs1 = u.r.xs1 ? RS1 : -1; \
            reg_t xs2 = u.r.xs2 ? RS2 : -1; \
            reg_t xd = rocc->custom##n(u.r, xs1, xs2); \
            if (u.r.xd) \
              WRITE_RD(xd); \
            return pc+4; \
            /* is finished , update related register status*/  \
           }else{ \
             /* keep waiting  */  \
             return pc; \
           } \
    } \
  } \
  \

cosim_customX(0)
cosim_customX(1)
cosim_customX(2)
cosim_customX(3)
std::vector<insn_desc_t> rocc_t::get_instructions()
{
  std::vector<insn_desc_t> insns;
  if (!is_cosim_enabled()){
    std::cout << "rocc_t get_instructions() [cosim_disabled]" << std::endl;
    insns.push_back((insn_desc_t){0x0b, 0x7f, &::illegal_instruction, c0});
    insns.push_back((insn_desc_t){0x2b, 0x7f, &::illegal_instruction, c1});
    insns.push_back((insn_desc_t){0x5b, 0x7f, &::illegal_instruction, c2});
    insns.push_back((insn_desc_t){0x7b, 0x7f, &::illegal_instruction, c3});
  }else{
    std::cout << "rocc_t get_instructions() [cosim_enabled]" << std::endl;
    insns.push_back((insn_desc_t){0x0b, 0x7f, &::illegal_instruction, cosim_c0});
    insns.push_back((insn_desc_t){0x2b, 0x7f, &::illegal_instruction, cosim_c1});
    insns.push_back((insn_desc_t){0x5b, 0x7f, &::illegal_instruction, cosim_c2});
    insns.push_back((insn_desc_t){0x7b, 0x7f, &::illegal_instruction, cosim_c3});

  }
  //insns.push_back((insn_desc_t){0x0b, 0x7f, &::illegal_instruction, test_cosim});
  //insns.push_back((insn_desc_t){0x2b, 0x7f, &::illegal_instruction, test_cosim});
  //insns.push_back((insn_desc_t){0x5b, 0x7f, &::illegal_instruction, test_cosim});
  //insns.push_back((insn_desc_t){0x7b, 0x7f, &::illegal_instruction, test_cosim});
  return insns;
}

std::vector<disasm_insn_t*> rocc_t::get_disasms()
{
  std::vector<disasm_insn_t*> insns;
  //insns.push_back(new disasm_insn_t("custom0",0x0b,0x7f,{&xrd, &xrs1, &xrs2}));
  return insns;
}
