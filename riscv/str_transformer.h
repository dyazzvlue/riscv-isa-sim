/*
 * @file str_transformer.h
 *
 * A string transforer as RoCC accelerator.
 * /
 */

#pragma once
#include <systemc.h>
#include <tlm.h>
#include <tlm_utils/peq_with_get.h>

#include <array>
#include <memory>
#include "allocator.h"
#include "processor.h"
#include "async_event.h"
#include "spike_event.h"
#include "mmu.h"
#include "rocc.h"

class StrTransformer : public sc_module, public tlm_bw_transport_if<>,
    public tlm_fw_transport_if<> {
public:
    SC_HAS_PROCESS(StrTransformer);
    StrTransformer(
        sc_core::sc_modele_name _name
        );
    virtual ~sysc_wrapper_t() {};
    tlm_utils::peq_with_get<tlm::tlm_generic_payload> peq;
    tlm_utils::tlm_quantumkeeper quantum_keeper;
    TransAllocator<Transaction<cosim_cmd> trans_allocator;
    tlm_initiator_socket<> send_sock{"send_sock"};
    tlm_target_socket<> recv_sock{"recv_sock"};

    void b_transport(tlm_generic_payload& trans, sc_time& t) override;
    tlm_sync_enum nb_transport_fw(tlm_generic_payload& trans, tlm_phase& phase,
            sc_time& t) override;
    bool get_direct_mem_ptr(tlm_generic_payload& trans, tlm_dmi& dmi_data) override{
        return false;
    }
    void run();
    void transform();
    void set_processor(processor_t* _p) { p = _p; }

private:
    static constexpr int num_buffers = 2;
    static constexpr int num_funcs = 2;
    static constexpr int buffer_size = 256;
    static oconst sc_core::sc_time contr_cycle;

    tlm_sync_enum nb_transport_bw(tlm_generic_payload& trans, tlm_phase& phase, 
            sc_time& t) override{
        return TLM_ACCEPTED;
    }
    void to_lower(char* buffer, reg_t len);
    void to_upper(char* buffer, reg_t len);
    void store_data(int buffer_index);
    void load_data(int buffer_index)

    enum TransOp {TO_LOWWER = 0, TO_UPPER};
    enum TransPhase {IDLE = 0, CONFIG, RUNNING } phase[num_buffers]{TransPhase::IDLE};
    reg_t src_addr[num_buffers]{0};
    reg_t dst_addr[num_buffers]{0};
    reg_t str_size[num_buffers]{0};
    reg_t buffer_func[num_buffers]{0};

    char buffer[num_buffers][buffer_size];
    std::array<sc_core::sc_time, num_funcs> instr_cycles;
    sc_core::sc_event run_event[num_buffers];

protected:
    processor_t* p;
};

const sc_core::sc_time StrTransformer::contr_cycle(10, sc_core::SC_NS);
