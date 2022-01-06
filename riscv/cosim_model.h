#pragma once
#ifndef _COSIM_COPROCESSOR_H
#define _COSIM_COPROCESSOR_H

#include <systemc.h>
#include <tlm.h>
#include "processer.h"
#include <vector>
#include <functional>
#include <iostream>

class cosim_model_t : public sc_module,  public tlm_bw_transport_if<>,
    public tlm_fw_transport_if<> {
public:
    virtual ~cosim_model_t();
    tlm_initiator_socket<> send_sock{"send_sock"};
    tlm_target_socket<> recv_sock{"recv_sock"};
    virtual const char* cosim_model_name() = 0;
    
    void set_processor(processor_t* _p) { p = _p; }
    tlm_initiator_socket<> get_send_port() {return this->send_sock;};
    tlm_target_socket<> get_recv_port() {return this->recv_sock;};
protected:
    processor_t* p;
};

std::function<cosim_model_t*()> find_cosim_model(const char* name);
void register_cosim_model(const char* name, std::function<extension_t*()> f);

#define REGISTER_COSIM_MODEL(name, constructor) \
    class register_##name { \
        public: register_##name() { register_cosim_model(#name, constructer);} \
    }; static register_cosim_##name dummy_cosim_##name;

#endif