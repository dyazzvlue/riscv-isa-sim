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
        SC_THREAD(run);
        SC_METHOD(event_notified);
        sensitive << event;
        dont_initialize();
        sc_process_handle_ = sc_get_current_process_handle();
    }

void sysc_wrapper_t::run(){
    // Main thread of systemc wrapper
    while(true){
//        sysc_log('=',this->name(),"waiting event");
        wait(event);
        if (event.get_spike_list().empty()){
            sysc_log('=',this->name(), "spike list is empty");
            isSyncCompleted = true;
            continue;
        }
        std::list<spike_event_t*>::iterator it;
        std::list<spike_event_t*> event_list = event.get_spike_list();
       // = event.get_spike_list().begin();
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
//                tmp->show();
                sysc_log('=',this->name(),"sync up time");
                // wait time to target step 
                wait(tmp->get_steps(), SC_NS);
                std::cout << "Close event ";
                tmp->show();
                tmp->finish();
                sysc_log('=',this->name(),"sync time complete");
            }
          //  it++;
        }
        sysc_log('=',this->name(), " wrapper run complete ");
        isSyncCompleted = true;
    }
}

void sysc_wrapper_t::send_rocc_rqst(spike_event_t* event){
    // TODO
    sysc_log('=',this->name() , " send rocc rqst ");
    event->show();
    // update the event status to finish
    event->finish();
    wait(SC_ZERO_TIME);
    
}

void sysc_wrapper_t::event_notified(){
    // event notified
    sysc_log('=',this->name(),"event notified");
}

void sysc_wrapper_t::notify(uint64_t step){
    isSyncCompleted=false;
    event.sync_time(step);
}

void sysc_wrapper_t::notify(std::list<spike_event_t*> events){
    isSyncCompleted=false;
    std::list<spike_event_t*> tmp=events;
    this->waiting_event_list.splice(this->waiting_event_list.end(),events);
//    std::cout << " tmps :" << tmp.size() << std::endl;
    event.recv_spike_event(tmp);
}

bool sysc_wrapper_t::is_sync_complete(){
    return this->isSyncCompleted;
}

void* sysc_controller_t::sysc_control(void *arg){
    sysc_controller_t *thiz = static_cast<sysc_controller_t *> (arg);
    while (true){
        std::cout << "sysc_controller waiting " << std::endl;
        pthread_mutex_lock(&thiz->mtx);
        while(thiz->is_notified == false){
            pthread_cond_wait(&thiz->cond, &thiz->mtx);
        }
        std::cout << "sysc_controller notify wrapper " << std::endl;
        // run rocc event until meet sync time event
        std::list<spike_event_t*> spike_event_list;
        spike_event_t* first_spike_event = thiz->get_first_spike_event();
        while (first_spike_event->get_type() != sc_cosim::sync_time) {
            // keep adding
            spike_event_list.push_back(first_spike_event);
            first_spike_event = thiz->get_first_spike_event();
        }
        spike_event_list.push_back(first_spike_event);
        // std::cout << "Add " << spike_event_list.size() << " events to list" << std::endl;
        thiz->sysc_wrapper.notify(spike_event_list);
        // notify sysc_wrapper
//        thiz->sysc_wrapper.notify(thiz->get_first_spike_event()->get_steps());
        while(thiz->sysc_wrapper.is_sync_complete()==false){
            // waiting sc_complete
        }
        // std::cout << "wrapper complete" << std::endl;
        thiz->is_notified=false;
        thiz->is_sc_completed=true;
        pthread_mutex_unlock(&thiz->mtx); 

    }
}

bool sysc_controller_t::notify_systemc(){
    if (this->spike_events.empty()){
        // no pending event, no need to notify systemc
        std::cout << "[controller]spike event is empty()" << std::endl;
        return true;
    }
//    std::cout << "Try notify " << std::endl;
    pthread_mutex_lock(&this->mtx);
    this->is_notified = true;
    this->is_sc_completed = false;
    pthread_mutex_unlock(&this->mtx);
//    std::cout << "cond signal " << std::endl;
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
    std::cout << "add spike event ";
    i->show();
    this->spike_events.push_back(i);
}

spike_event_t* sysc_controller_t::get_first_spike_event(){
    spike_event_t* event = this->spike_events.front();
    std::cout << " current spike event ";
    event->show();
    this->spike_events.pop_front();
    return event;
}

}
