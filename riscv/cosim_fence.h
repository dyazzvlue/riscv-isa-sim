#ifndef _COSIM_FENCE_H
#define _COSIM_FENCE_H

#include "spike_event.h"
#include "decode.h"
#include <stdlib.h>

using namespace sc_cosim;

// this cosim_fence_t is not real FENCE inst
// it's used for waiting systemc thread executed complete
//
// If meet the target instruction, throw the cosim_fence_t and
// try to send a spike event to sysc_controller at sync step
class cosim_fence_t{
public:
    //cosim_fence_t(reg_t which)
    //    : which(which){}
    cosim_fence_t() {};
    cosim_fence_t(insn_t insn)
        : insn(insn){};
    const char* name();
    // return if the event is executed
    bool isFinished();
    // set the related fence
    void set_spike_event(spike_event_t* event);
    spike_event_t* get_spike_event();
    insn_t get_cosim_insn() {return this->insn;}
private:
    char _name[16];
//    reg_t which;
    insn_t insn;
    spike_event_t* current_event; // related spike event


};
#endif
