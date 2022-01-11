#ifndef _COSIM_COPROCESSOR_H
#define _COSIM_COPROCESSOR_H

#include <systemc.h>
#include <tlm.h>
#include <tlm_utils/simple_initiator_socket.h>
#include <tlm_utils/simple_target_socket.h>

#include "processor.h"
#include <vector>
#include <functional>
#include <iostream>

class cosim_model_t : public sc_module {
public:
    tlm_utils::simple_initiator_socket<cosim_model_t> send_sock;
    tlm_utils::simple_target_socket<cosim_model_t> recv_sock;
    virtual const char* cosim_model_name() = 0;
    cosim_model_t(sc_core::sc_module_name _name):
        sc_core::sc_module(_name){
          recv_sock.register_b_transport(this, &cosim_model_t::transport_recv);
    }
    virtual ~cosim_model_t();

    void set_processor(processor_t* _p);
    virtual void transport_send(tlm::tlm_generic_payload& trans, sc_core::sc_time& t) = 0;
    void transport_recv(tlm::tlm_generic_payload& trans, sc_core::sc_time& t) {
        transport_recv_cb(trans, t);
    }
    virtual void transport_recv_cb(tlm::tlm_generic_payload& trans, sc_core::sc_time& t) = 0;
protected:
    processor_t* p;
};

std::function<cosim_model_t*()> find_cosim_model(const char* name);
void register_cosim_model(const char* name, std::function<cosim_model_t*()> f);

#define REGISTER_COSIM_MODEL(name, constructor) \
    class register_##name { \
        public: register_##name() { register_cosim_model(#name, constructor);} \
    }; static register_##name dummy_cosim_##name;

#endif
