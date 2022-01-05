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
#include "riscv/mmio_plugin.h"

/*
    This is a sc_module which simulate a simple rocc core. 
    Spike should load this module with custom-ext like dummy rocc.
*/
class str_transformer_rocc_t : public sc_module, public tlm_bw_transport_if<>,
    public tlm_fw_transport_if<> , public rocc_t{
public:
    const char* name() { return "str_transformer_rocc"; }
    reg_t custom0(rocc_insn_t insn, reg_t xs1, reg_t xs2);
    SC_HAS_PROCESS(str_transformer_rocc_t);
    str_transformer_rocc_t(
        sc_core::sc_modele_name _name
        );
    virtual ~sysc_wrapper_t() {};
    tlm_utils::peq_with_get<tlm::tlm_generic_payload> peq;
    tlm_utils::tlm_quantumkeeper quantum_keeper;
    TransAllocator<Transaction<cosim_resp> trans_allocator;
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
    bool is_busy();
    void transport_recv(tlm_generic_payload& trans, sc_time& t);
    void send_resp(rocc_insn_union_t* rocc_insn_union, uint32_t rd, reg_t data);

private:
    static constexpr int num_buffers = 2;
    static constexpr int num_funcs = 2;
    static constexpr int buffer_size = 256;
    static const sc_core::sc_time contr_cycle;

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

const sc_core::sc_time str_transformer_rocc_t::contr_cycle(10, sc_core::SC_NS);
// register this sc_module like custom-ext in Spike
REGISTER_EXTENSION(str_trans_former_rocc, []() { return new str_transformer_rocc_t; })

/* not used
struct rocc_mmio_plugin{
    rocc_mmio_plugin(const std::string& args){
        printf("ALLOC -- ARGS=%s\n", args.c_str());
    }
    ~rocc_mmio_plugin(){
        printf("DEALLOC --SELF=%p\n", this);
    }
    bool load(reg_t addr, size_t len. uint8_t* bytes){
        printf("TRY LOAD -- SELF=%p ADDR=0x%lx LEN=%lu BYTES=%p\n", this, addr, len, (void*)bytes);
    //    memcpy(bytes, this->contents(addr), len);
        addr += n;
        bytes += n;
        len -= n;
        return true;
    }

    bool store(reg_t addr, size_t len, const uint8_t* bytes){
        printf("TRY STORE -- SELF=%p ADDR=0x%lx LEN=%lu BYTES=%p\n", this, addr, len, (const void*)bytes);
    //    memcpy(this->contents(addr), bytes, len);
        addr += n;
        bytes += n;
        len -= n;
        return true;
    }
};

static mmio_plugin_registration_t<rocc_mmio_plugin> rocc_mmio_plugin_registration("rocc_mmio_plugin");
*/