#include "sysc_wrapper.h"
#include <assert.h>
#include <sched.h>
#include <stdlib.h>
namespace sc_cosim{

sysc_wrapper_t::sysc_wrapper_t(
        sc_core::sc_module_name _name
        ):
    sc_module( _name ){
        // initializations
        send_sock.bind(*this);
        recv_sock.bind(*this);
        SC_THREAD(run);
        SC_METHOD(event_notified);
        sensitive << event;
        dont_initialize();
        sc_process_handle_ = sc_get_current_process_handle();
    }

void sysc_wrapper_t::run(){
    // Main thread of systemc wrapper
    while(true){
        wait(event);
        if (event.get_spike_list().empty()){
            //sysc_log('=',this->name(), "spike list is empty");
            sysc_log(this->log_file,this->name(), "spike list is empty");
            isSyncCompleted = true;
            continue;
        }
        std::list<spike_event_t*>::iterator it;
        std::list<spike_event_t*> event_list = event.get_spike_list();
        //while(it!=event.get_spike_list().end() && (*it) != NULL){
        for (it=event_list.begin();
                it!=event_list.end();
                ++it){
            if  ((*it)->get_type() == spike_event_type::rocc_inst){
                // is rocc rqst/rsp event
                send_rocc_rqst(*it);
            }else if ((*it)->get_type() == spike_event_type::sync_time){
                // is sync time event
                spike_event_t* tmp = (*it);
                //sysc_log('=',this->name(),"sync up time");
                sysc_log(this->log_file,this->name(),"sync up time");
                sysc_log(this->log_file,this->name(),tmp->info());
                // wait time to target step 
                wait(tmp->get_steps(), SC_NS);
                //tmp->show();
                tmp->finish();
                sysc_log(this->log_file,this->name(),"sync time complete");
            }
        }
        sysc_log(this->log_file ,this->name(), " wrapper run complete ");
        isSyncCompleted = true;
    }
}

void sysc_wrapper_t::send_rocc_rqst(spike_event_t* event){
    // send cosim cmd by tlm socket
    sysc_log(this->log_file, this->name() , " send rocc rqst ");
    sysc_log(this->log_file, this->name() , event->info());

    // send tlm payload to sock 
    auto trans = trans_allocator.allocate();
    tlm::tlm_command cmd = tlm::TLM_IGNORE_COMMAND;
    trans->set_command(cmd);
    trans->set_address(COSIM_START_ADDR); // TODO: is necessary ?
    auto req = trans->get_data_ptr();
    req->insn = event->get_cosim_insn();
    sc_time delay = SC_ZERO_TIME;
    trans->set_data_ptr((unsigned char*)req);
    trans->set_data_length(sizeof(cosim_cmd));
    trans->set_response_status(tlm::TLM_OK_RESPONSE);
    this->send_sock->b_transport(*trans, delay);  // send to target sc_module

}

void sysc_wrapper_t::b_transport(tlm_generic_payload& trans, sc_time& t) {
    // recv result from binded sc_module
    auto recv_trans = (cosim_resp*)trans.get_data_ptr();
    sysc_log(this->log_file, this->name(), " recv cosim response");
    // update the related spike event status
    insn_t insn = recv_trans->insn;
    // find spike event by insn
    spike_event_t* event = this->find_spike_event_by_insn(insn);
    if (event == NULL){
        std::cerr << "invalid response insn " << std::endl;
    }else{
        event->finish();
        this->release_wait_event(insn);
        std::cout << sc_time_stamp() << " [sysc_wrapper] close the event " <<
            insn.bits() << std::endl;
    }

}

spike_event_t* sysc_wrapper_t::find_spike_event_by_insn(insn_t insn){
    std::list<spike_event_t*>::iterator it;
    for (it=this->waiting_event_list.begin();
            it!=this->waiting_event_list.end();
            ++it){
        if ((*it)->get_cosim_insn().bits() == insn.bits()){
            return (*it);
        }
    }
    return NULL;
}

// TODO insn may be same, change to evnet ID is better?
void sysc_wrapper_t::release_wait_event(insn_t insn) {
    std::list<spike_event_t*>::iterator it;
    for (it=this->waiting_event_list.begin();
            it!=this->waiting_event_list.end();
            ++it){
        if ((*it)->get_cosim_insn().bits() == insn.bits()){
             this->waiting_event_list.erase(it);
             break;
        }
    }
}

void sysc_wrapper_t::event_notified(){
    // event notified
    sysc_log(this->log_file ,this->name(),"event notified");
}

void sysc_wrapper_t::notify(uint64_t step){
    isSyncCompleted=false;
    event.sync_time(step);
}

void sysc_wrapper_t::notify(std::list<spike_event_t*> events){
    isSyncCompleted=false;
    std::list<spike_event_t*> tmp=events;
    this->waiting_event_list.splice(this->waiting_event_list.end(),events);
    event.recv_spike_event(tmp);
}

bool sysc_wrapper_t::is_sync_complete(){
    return this->isSyncCompleted;
}

void sysc_wrapper_t::set_log_file(FILE* file){
    this->log_file = file;
}

// TODO Not used, check if is possible
void sysc_wrapper_t::config_cosim_model(cosim_model_t* model){
    // combine the sockets TODO: support vector/array sockets
    this->send_sock.bind(model->recv_sock);
    model->send_sock.bind(this->recv_sock);
}

void* sysc_controller_t::sysc_control(void *arg){
    sysc_controller_t *thiz = static_cast<sysc_controller_t *> (arg);
    while (true){
        log(thiz->log_file, "sysc_controller", "waiting");
        pthread_mutex_lock(&thiz->mtx);
        while(thiz->is_notified == false){
            pthread_cond_wait(&thiz->cond, &thiz->mtx);
        }
        log(thiz->log_file, "sysc_controller", "notify sysc_wrapper");
        // run rocc event until meet sync time event
        std::list<spike_event_t*> spike_event_list;
        spike_event_t* first_spike_event = thiz->get_first_spike_event();
        while (first_spike_event->get_type() != sc_cosim::sync_time) {
            // keep adding
            spike_event_list.push_back(first_spike_event);
            first_spike_event = thiz->get_first_spike_event();
        }
        spike_event_list.push_back(first_spike_event);
        thiz->sysc_wrapper.notify(spike_event_list);
        // notify sysc_wrapper
        while(thiz->sysc_wrapper.is_sync_complete()==false){
            // waiting sc_complete
        }
        thiz->is_notified=false;
        thiz->is_sc_completed=true;
        pthread_mutex_unlock(&thiz->mtx); 

    }
}

bool sysc_controller_t::notify_systemc(){
    if (this->spike_events.empty()){
        // no pending event, no need to notify systemc
//        std::cout << "[controller]spike event is empty()" << std::endl;
        log(this->log_file, "controller", "spike event is empty");
        return true;
    }
    pthread_mutex_lock(&this->mtx);
    this->is_notified = true;
    this->is_sc_completed = false;
    pthread_mutex_unlock(&this->mtx);
    pthread_cond_signal(&this->cond);
    // wait sc finish
    while (this->is_sc_completed==false){
        usleep(1*1000);
    }
    return true;
}

void sysc_controller_t::run(){
    pthread_create(&thread, NULL, sysc_controller_t::sysc_control, this);
    pthread_detach(thread);
}

void sysc_controller_t::add_spike_events(spike_event_t* i){
    log(this->log_file, "add spike event", i->info());
    this->spike_events.push_back(i);
}

spike_event_t* sysc_controller_t::get_first_spike_event(){
    spike_event_t* event = this->spike_events.front();
    log(this->log_file, "current spike event", event->info());
    this->spike_events.pop_front();
    return event;
}

void sysc_controller_t::set_log_file(FILE* file){
    this->log_file = file;
    this->sysc_wrapper.set_log_file(file);
}

void sysc_controller_t::config_cosim_model(cosim_model_t* model){
    this->sysc_wrapper.config_cosim_model(model);
}
}
