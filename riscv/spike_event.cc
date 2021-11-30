#include "spike_event.h"

namespace sc_cosim{
    spike_event_t::spike_event_t (spike_event_type _type, uint64_t _start_time,
            uint64_t _steps){
        this->type = _type;
        this->start_time = _start_time;
        this->steps = _steps;
        this->state = false;
    }

    spike_event_type  spike_event_t::get_type(){
        return this->type;
    }

    uint64_t spike_event_t::get_start_time(){
        return this->start_time;
    }

    uint64_t spike_event_t::get_steps(){
        std::cout << "Try get steps " << this->steps <<  std::endl;
        return this->steps;
    }

    bool spike_event_t::isFinished(){
        return this->state;
    }

    void spike_event_t::finish(){
        this->state = true;
    }

    void spike_event_t::show(){
        if (this->type == spike_event_type::sync_time){
            std::cout << "[sync_time] start time : " << this->start_time 
                << " steps: " << this->steps << std::endl;
        }else if (this->type == spike_event_type::rocc_inst){
            std::cout << "[rocc_inst]" << std::endl;
        }
    }
}
