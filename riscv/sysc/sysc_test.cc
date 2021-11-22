#include"sysc_log.h"
#include<systemc.h>
#include"sysc_wrapper.h"
#include"async_event.h"
#include"spike_event.h"
#include<pthread.h>

using namespace sc_core;
using namespace sc_cosim;

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

bool runsysc = false;

void *runSystemC(void *arg) {
    sysc_controller_t* sys = (sysc_controller_t*)(arg);
    usleep(1*1000*1000);
    while(true){
        pthread_mutex_lock(&mtx);
        //while(runsysc == false){
        //    pthread_cond_wait(&cond, &mtx);
        //}
        for (int i=0;i <20;i++){
            if (i %2 ==0){
                std::cout << " [*]notify " << i << std::endl;
                //sys->notify(i);
                spike_event_t* event= createSyncTimeEvent(0,i);
                sys->add_spike_events(event);
                sys->notify_systemc();
            }else{
                //pass
            }
        }
        pthread_mutex_unlock(&mtx);
        // wake up sc_kernel

    }
}

int sc_main(int argc, char* argv[]){
    log('#',"Start","");
    sysc_controller_t sysc_controller("sysc");
    pthread_t tmp;
    pthread_create(&tmp, NULL, runSystemC, &sysc_controller);
    sysc_controller.run();
    pthread_detach(tmp);
    /*
    for (int i=0;i <20;i++){
        if (i %2 ==0){
            std::cout << "notify " << i << std::endl;
            sysc_controller.add_spike_events(i);
            if (sysc_controller.notify_systemc()){
                std::cout << "systmc update complete" << std::endl;
            }
        }else{
            //pass
        }
    }
    */
    //sc_pause();
    sc_start(100,SC_NS);

    return 0;
}

