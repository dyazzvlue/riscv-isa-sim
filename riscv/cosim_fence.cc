#include "cosim_fence.h"
#include "processor.h"
#include <cstdio>

const char* cosim_fence_t::name(){
//    const char* fmt = uint8_t(which) == which ? "fence #%u" : "interrupt #%u";
//    sprintf(_name, fmt, uint8_t(which));
//    return _name;
      return "cosim_fence";
}

bool cosim_fence_t::isFinished(){
    return this->current_event->isFinished();
}

void cosim_fence_t::set_spike_event(spike_event_t* event){
    this->current_event = event;
}

spike_event_t* cosim_fence_t::get_spike_event(){
    return current_event;
}
