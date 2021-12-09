#ifndef _ASYNC_EVENT_H
#define _ASYNC_EVENT_H
#include<systemc.h>
#include<pthread.h>
#include<string>
#include "sysc_log.h"
#include "spike_event.h"
#include<list>

using namespace std;
using namespace sc_core;

namespace sc_cosim{
class spike_event_t;

class AsyncEventIf : public sc_interface{
    virtual void notify(sc_time delay = SC_ZERO_TIME) =0;
    // TBD
    virtual void sync_time(uint64_t steps)=0;
    virtual const sc_event &default_event(void) const =0;
protected:
    virtual void update(void) = 0;
};

class AsyncEvent: public sc_prim_channel, public AsyncEventIf{
public:
    AsyncEvent(const char *name=""): event(name){
        async_attach_suspending();
    }
    // basic notify method
    void notify(sc_time delay = SC_ZERO_TIME) {
        this->delay = delay;
        async_request_update();
    }

    void sync_time(uint64_t steps){
        // TODO
        // suppose one step = 1 NS
        sc_time tmp = sc_time(steps, SC_NS);
        this->steps = tmp;
        notify();
    }

    void recv_spike_event(std::list<spike_event_t*> events){
        //log('-',this->name(), "recv spike event");
        this->spike_event_list.assign(events.begin(), events.end());
        notify();
    }

    std::list<spike_event_t*> get_spike_list(){
        return this->spike_event_list;
    }

    const sc_event &default_event(void) const {
        return event;
    }

    operator const sc_event&() const {return event;}

    sc_time get_delay(){
        return this->delay;
    }

    sc_time get_steps(){
        return this->steps;
    }
protected:
    virtual void update(void){
        event.notify(delay);
    }
    sc_event event;
    sc_time delay;
    sc_time steps;
    std::list<spike_event_t*> spike_event_list;

};
}
#endif
