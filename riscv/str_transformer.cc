#include "str_transformer.h"

StrTransformer::StrTransformer(
        sc_core::sc_module_name _name
        ): sc_module(_name){
    send_sock.bind(*this);
    recv_sock.bind(*this);
    SC_THREAD(run);
}

void StrTransformer::run(){
    while (true) {
        sc_core::wait(peq.get_event());
        tlm::tlm_generic_payload* trans = peq.get_next_transaction();
        while (trans) {
            quantum_keeper.inc(contr_cycle);
            if (quantum_keeper.need_sync()){
                quantum_keeper.sync();
            }
            auto req = (cosim_cmd*)trans->get_data_ptr();
            rocc_insn_union_t u;
            insn_t insn = req->insn;
            u.i = insn;
            reg_t xs1 = u.r.xs1 ? RS1 : -1;
            reg_t xs2 = u.r.xs2 ? RS2 : -1;
            auto func = u.r.funct; // TODO
            switch (func) {
                case 0: // config
                {
                    auto& index = u.r.rs2;
                    auto& value = u.r.rs1;
                    auto buffer_index = index / 10;
                    auto local_index = index % 10;
                    assert(buffer_index < num_buffers);
                    assert(local_index < 4);
                    assert(phases[buffer_index] <= TransPhase::CONFIG);
                    if (local_index == 0)
                        src_addr[buffer_index] = value;
                    else if (local_index == 1)
                        dst_addr[buffer_index] = value;
                    else if (local_index == 2)
                        str_size[buffer_index] = value;
                    else
                        buffer_func[buffer_index] = value;
                    phases[buffer_index] = TransPhase::CONFIG;
                    break;
                }
                case 1: // run
                {
                    auto& buffer_index = u.r.rs2;
                    assert(buffer_index < num_buffers);
                    assert(phases[buffer_index] <= TransPhase::CONFIG);
                    phases[buffer_index] = TransPhase::RUNNING;
                    transform();
                    break;
                }
                default:
                {
                    // raise illegal instruction TODO
                    illegal_instruction();
                }

            }
            trans->release();
            trans = peq.get_next_transaction();
        }
    }
}

void StrTransformer::b_transport(tlm_generic_payload& trans, sc_time& t) {

}

void StrTransformer::to_lower(char* buffer, reg_t len){
    char* c = buffer;
    static int inc = 'a' - 'A';
    while (len > 0){
        if (*c >= 'A' && *c <= 'Z')
            *c += inc;
        c++;
        len--;
    }
}

void StrTransformer::to_upper(char* buffer, reg_t len){
    char* c = buffer;
    static int inc = 'a' - 'A';
    while (len > 0){
        if (*c >= 'a' && *c <= 'z')
            *c -= inc;
        c++;
        len--;
    }
}

void StrTransformer::store_data(int buffer_index) {

}

void StrTransformer::loaad_data(int buffer_index){

}

void StrTransformer::transform(int buffer_index){
    while (true) {
        wait(run_event[buffer_index]);
        wait(instr_cycles[buffer_func[buffer_index]]);
        switch (buffer_func[buffer_index]) {
            case TransOp::TO_LOWER: {
                load_data(buffer_index);
                to_lower(buffer[buffer_index], str_size[buffer_index]);
                store_data(buffer_index);
                break;
            }
            case TransOp:TO_UPPER: {
                load_data(buffer_index);
                to_upper(buffer[buffer_index],str_size[buffer_index]);
                store_data(buffer_index);
                break;
            }
            default:
                // raise_trap

        }

        ph
    }
}
