#ifndef _COSIM_FENCE_H
#define _COSIM_FENCE_H

#include "spike_event.h"
#include "decode.h"
#include <stdlib.h>

using namespace sc_cosim;

// this cosim_fence_t is not real FENCE inst
// it's used for waiting systemc thread executed complete
//
// First step , if fence executed, throw the cosim_fence_t and 
// send a spike event to sysc_wrapper
class cosim_fence_t{
public:
    //cosim_fence_t(reg_t which)
    //    : which(which){}
    cosim_fence_t(){};
    const char* name();
    // return if the event is executed
    bool isFinished();
    // set the related fence
    void set_spike_event(spike_event_t* event);
    spike_event_t* get_spike_event();
private:
    char _name[16];
//    reg_t which;
    spike_event_t* current_event; // related spike event


};
#endif
