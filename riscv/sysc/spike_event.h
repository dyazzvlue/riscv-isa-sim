#ifndef _SPIKE_EVENT_H
#define _SPIKE_EVENT_H
#include <assert.h>
#include <stdlib.h>
#include <iostream>

namespace sc_cosim{
using namespace std;

typedef enum {
    sync_time,
    rocc_inst
} spike_event_type;

class spike_event_t {
public:
    spike_event_t(
            spike_event_type _type, uint64_t _start_time, uint64_t _steps);
    virtual ~spike_event_t() {};

    spike_event_type getType();
    uint64_t get_start_time();
    uint64_t get_steps();
    void show();

private:
    spike_event_type type; // event type
    uint64_t start_time; // start time()
    uint64_t steps; // sync up steps
    // rocc_inst_t inst;
};

inline spike_event_t* createSyncTimeEvent(uint64_t start_time, uint64_t steps){
    return new spike_event_t(spike_event_type::sync_time, start_time, steps);
}

inline spike_event_t* createRoccInstEvent(){
    // TODO
    return NULL;
}

}
#endif
