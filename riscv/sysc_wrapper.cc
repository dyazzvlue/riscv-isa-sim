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
        sysc_log('=',this->name(),"sync up time");
        // wait time to target step 
        wait(event.get_steps());
        sysc_log('=',this->name(),"sync time complete");
        isSyncCompleted = true;
    //    sc_pause();
    }
}

void sysc_wrapper_t::event_notified(){
    // event notified
    sysc_log('=',this->name(),"event notified");
}

void sysc_wrapper_t::notify(uint64_t step){
    isSyncCompleted=false;
    event.sync_time(step);
}

bool sysc_wrapper_t::is_sync_complete(){
    return this->isSyncCompleted;
}

void* sysc_controller_t::sysc_control(void *arg){
    sysc_controller_t *thiz = static_cast<sysc_controller_t *> (arg);
    while (true){
        std::cout << "Waiting " << std::endl;
        pthread_mutex_lock(&thiz->mtx);
        while(thiz->is_notified == false){
            pthread_cond_wait(&thiz->cond, &thiz->mtx);
        }
        //sc_start();
        std::cout << "Notify wrapper " << std::endl;
        thiz->sysc_wrapper.notify(thiz->get_first_spike_event()->get_steps());
        while(thiz->sysc_wrapper.is_sync_complete()==false){
            // waiting sc_complete
        }
        std::cout << "wrapper complete" << std::endl;
        thiz->is_notified=false;
        thiz->is_sc_completed=true;
        pthread_mutex_unlock(&thiz->mtx); 

    }
}

bool sysc_controller_t::notify_systemc(){
    if (this->spike_events.empty()){
        // no pending event, no need to notify systemc
        std::cout << "spike event is empty()" << std::endl;
        return true;
    }
    std::cout << "Try notify " << std::endl;
    pthread_mutex_lock(&this->mtx);
    this->is_notified = true;
    this->is_sc_completed = false;
    pthread_mutex_unlock(&this->mtx);
    std::cout << "cond signal " << std::endl;
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
