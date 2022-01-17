#include "cosim_model.h"

cosim_model_t::~cosim_model_t(){

}

void cosim_model_t::set_processor(processor_t* _p) {
    this->p = _p;
}
