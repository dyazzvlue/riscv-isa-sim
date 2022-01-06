#include "str_transformer.h"

str_transformer_rocc_t::str_transformer_rocc_t(
        sc_core::sc_module_name _name
        ): sc_module(_name), peq("str_transformer") {
    for (int i = 0; i < num_buffers; i++) {
		memset(buffer[i], 0, sizeof(buffer[i]));
	}
    send_sock.bind(*this);
    recv_sock.bind(*this);
    recv_sock.register_b_transport(this, &str_transformer_rocc_t::transprot_recv);
    SC_THREAD(run);
}

void str_transformer_rocc_t::run(){
    for (int i = 0; i < num_buffers; i++){
            sc_core::sc_spawn(sc_bind(&str_transformer_rocc_t::transform, this, i));
        }
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
                    run_event[buffer_index].notify();
                    break;
                }
                default:
                {
                    // raise illegal instruction TODO
                    illegal_instruction();
                }

            }
            trans->release();
            // send resposne to sysc_wrapper
            send_resp(&u, 0 , 0); // no need to set rd or data in this case
            trans = peq.get_next_transaction();
        }
    }
}

void str_transformer_rocc_t::b_transport(tlm_generic_payload& trans, sc_time& t) {

}

void str_transformer_rocc_t::send_resp(rocc_insn_union_t* rocc_insn_union, uint32_t rd, reg_t data){
    std::cout << "str_transformer_rocc_t send resp to sysc_wrapper" << std::endl;
    auto ts = sc_time_stamp();
    auto trans = trans_allocator.allocate();
    tlm::tlm_command cmd = tlm::TLM_IGNORE_COMMAND;
    trans->set_command(cmd);
    trans->set_address(COSIM_END_ADDR);
    auto resp = trans->get_data_ptr();
    resp->insn = rocc_insn_union->i;
    resp->rd = rd;
    resp->data = data;
    sc_time delay = SC_ZERO_TIME;
    trans->set_data_ptr((unsigned char*)resp);
    trans->set_data_length(sizeof(cosim_resp));
    trans->set_response_status(tlm::TLM_OK_RESPONSE);
    send_sock->b_transport(*trans,delay);
}

void str_transformer_rocc_t::transport_recv(tlm_generic_payload& trans, sc_time& t){
    // recv rocc rqst from sysc_wrapper
    peq.notify(trans);
}

bool str_transformer_rocc_t::is_busy(){
    for (auto& p : phases) {
		if (p == TransPhase::RUNNING) {
			return true;
        }
    }
	return false;
}

void str_transformer_rocc_t::to_lower(char* buffer, reg_t len){
    char* c = buffer;
    static int inc = 'a' - 'A';
    while (len > 0){
        if (*c >= 'A' && *c <= 'Z')
            *c += inc;
        c++;
        len--;
    }
}

void str_transformer_rocc_t::to_upper(char* buffer, reg_t len){
    char* c = buffer;
    static int inc = 'a' - 'A';
    while (len > 0){
        if (*c >= 'a' && *c <= 'z')
            *c -= inc;
        c++;
        len--;
    }
}

void str_transformer_rocc_t::store_data(int buffer_index) {
    auto len = str_size[buffer_index];
	auto addr = dst_addr[buffer_index];
	auto offset = 0;
	reg_t data;
    mmu_t* _mmu = p->get_mmu();
	while (len > sizeof(reg_t)) {
		memcpy(&data, buffer[buffer_index] + offset, sizeof(reg_t));
		//mem_if->store(addr, data);
        //_mmu->mmio_store(addr, data)
        _mmu->store_uint64(addr, data);
		offset += sizeof(reg_t);
		len -= sizeof(reg_t);
		addr += sizeof(reg_t);
	}
	if (len > 0) {
		memcpy(&data, buffer[buffer_index] + offset, len);
		mem_if->store(addr, data);
        _mmu->store_uint64(addr + offset, data);
	}
}

void str_transformer_rocc_t::load_data(int buffer_index){
    auto len = str_size[buffer_index];
	auto addr = src_addr[buffer_index];
	auto offset = 0;
	reg_t data;
    mmu_t* _mmu = p->get_mmu();
	while (len > sizeof(reg_t)) {
		//data = mem_if->load(addr);
        data = _mmu->load_uint64(addr);
        // read from memory
		memcpy(buffer[buffer_index] + offset, &data, sizeof(reg_t));
		offset += sizeof(reg_t);
		len -= sizeof(reg_t);
		addr += sizeof(reg_t);
	}
	if (len > 0) {
		//data = mem_if->load(addr);
        data = _mmu->load_uint64(addr);
        // load from mem
		memcpy(buffer[buffer_index] + offset, &data, len);
	}
}

void str_transformer_rocc_t::transform(int buffer_index){
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
        phases[buffer_index] = TransPhase::IDLE;
    		if (!is_busy()) {
		//	core.io_fence_done();
            // do nothing
		}

    }
}
/*
 reg_t str_transformer_rocc_t::custom0(rocc_insn_t insn, reg_t xs1, reg_t xs2){
     // do nothing , actual execution is in run method
     return 0;
 }
 */
