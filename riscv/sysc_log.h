#ifndef _SYSC_LOG_H
#define _SYSC_LOG_H
#include<systemc.h>
#include<string>
#include<sstream>

using namespace std;
using namespace sc_core;

template <typename T>
void log(char ch, std::string name, T t){
    string i(5,ch);
    std::cout << i << "[" << name <<  "] " << t << std::endl;
}

template <typename T>
void log(FILE* file, std::string name, T t){
    std::stringstream s;
    s << name << " : " << t << std::endl;
    fprintf(file, s.str().c_str());
}

template <typename T>
void sysc_log(char ch, std::string name, T t){
    std::cout << sc_time_stamp() << ": ";
    log(ch,name,t);
}

template <typename T>
void sysc_log(FILE* file, std::string name, T t){
    std::stringstream s;
    s << sc_time_stamp() << " | " << name << " : " << t << std::endl;
    fprintf(file, s.str().c_str());
}

#endif
