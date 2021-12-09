#ifndef _SYSC_WRAPPER_H
#define _SYSC_WRAPPER_H
#include<systemc.h>
#include<pthread.h>
#include<string>
#include"spike_event.h"
#include"async_event.h"
#include"sysc_log.h"
#include <deque>
#include <list>
#include <unistd.h>

namespace sc_cosim{

class spike_event_t;
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
    void notify(std::list<spike_event_t*> events);
    void send_rocc_rqst(spike_event_t* event); // send rocc rqst to systemc model
    bool is_sync_complete();
    pthread_mutex_t mtx;
    pthread_cond_t cond;
    void set_log_file(FILE* file);

private:
    // TODO running flags
    // std::vector<rocc_rqst_t> rocc_rqsts;
    sc_process_handle sc_process_handle_;
    bool notStarted=true;
    bool isSyncCompleted=false;
    std::list<spike_event_t*> waiting_event_list;
    FILE* log_file;
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
        void set_log_file(FILE* file);

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
        FILE* log_file;
};

}
#endif
