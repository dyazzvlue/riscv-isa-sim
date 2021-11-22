#ifndef _SYSC_WRAPPER_H
#define _SYSC_WRAPPER_H
#include<systemc.h>
#include<pthread.h>
#include<string>
#include"async_event.h"
#include"spike_event.h"
#include"sysc_log.h"
#include <deque>
#include <unistd.h>
namespace sc_cosim{

using namespace std;
using namespace sc_core;

class sysc_wrapper_t : public sc_module {
public:
    SC_HAS_PROCESS(sysc_wrapper_t);
    sysc_wrapper_t(
            sc_core::sc_module_name _name
            );
    virtual ~sysc_wrapper_t() {};
    AsyncEvent event;
    // update sc_time, which will be triggered by Spike
    void run();
    void event_notified();
    void notify(uint64_t step);
    bool is_sync_complete();
    pthread_mutex_t mtx;
    pthread_cond_t cond;

private:
    // TODO running flags
    // std::vector<rocc_rqst_t> rocc_rqsts;
    sc_process_handle sc_process_handle_;
    bool notStarted=true;
    bool isSyncCompleted=false;
};

class sysc_controller_t{
    public:
        sysc_controller_t(const char *name):
            mtx(PTHREAD_MUTEX_INITIALIZER),
            cond(PTHREAD_COND_INITIALIZER),
            name(name),
            sysc_wrapper((std::string(name)+"_wrapper").c_str())
        {};
        ~sysc_controller_t(){};
        // thread function
        static void *sysc_control(void *arg);
        // notify systemc
        bool notify_systemc();
        // create thread and waiting notify
        void run();
        // add events from spike sync request, TBD
        void add_spike_events(spike_event_t* spike_event);
        // return first event in spike sync request, TBD
        spike_event_t* get_first_spike_event();

    private:
        pthread_t thread;
        pthread_mutex_t mtx;
        pthread_cond_t cond;
        std::string name;
        bool is_notified = false;
        bool is_sc_completed = false;
        sysc_wrapper_t sysc_wrapper;
        // deque of pending events from spike, TBD
        std::deque<spike_event_t*> spike_events;
};

}
#endif
