/*
 * @file str_transformer.h
 *
 * A string transforer as RoCC accelerator.
 * /
 */

#pragma once
#include "cosim_model.h"
#include <systemc.h>
#include <tlm.h>
#include <tlm_utils/peq_with_get.h>
#include <tlm_utils/tlm_quantumkeeper.h>

#include <array>
#include <memory>
#include "allocator.h"
#include "async_event.h"
#include "spike_event.h"

#include "mmu.h"
#include "rocc.h"
/*
    This is a sc_module which simulate a simple rocc core. 
    Spike should load this module with custom-ext like dummy rocc.
*/
class str_transformer_rocc_t : public cosim_model_t{
public:
    SC_HAS_PROCESS(str_transformer_rocc_t);
    str_transformer_rocc_t(
        sc_core::sc_module_name _name = "str_transformer_model"
        );
    virtual ~str_transformer_rocc_t() {};
    tlm_utils::peq_with_get<tlm::tlm_generic_payload> peq;
    tlm_utils::tlm_quantumkeeper quantum_keeper;
    TransAllocator<Transaction<cosim_resp>> trans_allocator;

    void run();
    void transform(int buffer_index);
    bool is_busy();
    void transport_recv_cb(tlm::tlm_generic_payload& trans, sc_core::sc_time& t) override;
    void send_resp(rocc_insn_union_t* rocc_insn_union, uint32_t rd, reg_t data);
    const char* cosim_model_name() override {return "str_transformer_model";}
    void transport_send(tlm::tlm_generic_payload& trans, sc_core::sc_time& t) override {};

private:
    static constexpr int num_buffers = 2;
    static constexpr int num_funcs = 2;
    static constexpr int buffer_size = 256;
    static const sc_core::sc_time contr_cycle;

    void to_lower(char* buffer, reg_t len);
    void to_upper(char* buffer, reg_t len);
    void store_data(int buffer_index);
    void load_data(int buffer_index);

    enum TransOp {TO_LOWWER = 0, TO_UPPER};
    enum TransPhase {IDLE = 0, CONFIG, RUNNING } phases[num_buffers]{TransPhase::IDLE};
    reg_t src_addr[num_buffers]{0};
    reg_t dst_addr[num_buffers]{0};
    reg_t str_size[num_buffers]{0};
    reg_t buffer_func[num_buffers]{0};

    char buffer[num_buffers][buffer_size];
    std::array<sc_core::sc_time, num_funcs> instr_cycles;
    sc_core::sc_event run_event[num_buffers];

};

const sc_core::sc_time str_transformer_rocc_t::contr_cycle(10, sc_core::SC_NS);

// register this sc_module like custom-ext in Spike
REGISTER_COSIM_MODEL(str_transformer_model, []() { return new str_transformer_rocc_t("str_model"); })
