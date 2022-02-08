#ifndef _SPIKE_EVENT_H
#define _SPIKE_EVENT_H
#include <assert.h>
#include <stdlib.h>
#include <iostream>
#include "decode.h"
#include <sstream>
#include <string>

// TODO
#define COSIM_START_ADDR 0x10000000
#define COSIM_END_ADDR 0x1000ffff

// cosim cmd TODO: move to a spilted file
struct cosim_cmd {
    insn_t insn;
    reg_t rs1;
    reg_t rs2;
};

// cosim resp TODO: move to a spilted file
struct cosim_resp {
    insn_t insn;
    uint32_t rd;
    reg_t data;
};
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

    spike_event_type get_type();
    uint64_t get_start_time();
    uint64_t get_steps();
    void show();
    std::string info();
    bool isFinished();
    void finish();
    insn_t get_cosim_insn() { return this->insn; }
    void set_cosim_insn(insn_t _insn) {this->insn = _insn;}

private:
    spike_event_type type; // event type
    uint64_t start_time; // start time()
    uint64_t steps; // sync up steps
    bool state;
    insn_t insn; // cosim instruction
};

inline spike_event_t* createSyncTimeEvent(uint64_t start_time, uint64_t steps){
    return new spike_event_t(spike_event_type::sync_time, start_time, steps);
}

inline spike_event_t* createRoccInstEvent(uint64_t start_time, uint64_t steps,
        insn_t insn){
    spike_event_t* event = new spike_event_t(spike_event_type::rocc_inst,
            start_time, steps);
    event->set_cosim_insn(insn);
    return event;
}

inline spike_event_t* createRoccInsnEvent(insn_t insn) {
    spike_event_t* event = new spike_event_t(spike_event_type::rocc_inst,
            0,0);
    event->set_cosim_insn(insn);
    std::cout << "create a dummy rocc insn event" << std::endl;
    return event;
}
}
#endif