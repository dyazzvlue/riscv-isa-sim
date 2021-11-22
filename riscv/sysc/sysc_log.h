#ifndef _SYSC_LOG_H
#define _SYSC_LOG_H
#include<systemc.h>
#include<string>

using namespace std;
using namespace sc_core;

template <typename T>
void log(char ch, std::string name, T t){
    string i(5,ch);
    std::cout << i << "[" << name <<  "] " << t << std::endl;
}

template <typename T>
void sysc_log(char ch, std::string name, T t){
    std::cout << sc_time_stamp() << ": ";
    log(ch,name,t);
}

#endif
