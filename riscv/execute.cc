// See LICENSE for license details.

#include "processor.h"
#include "mmu.h"
#include "disasm.h"
#include <cassert>
#include "cosim_fence.h"

#ifdef RISCV_ENABLE_COMMITLOG
static void commit_log_reset(processor_t* p)
{
  p->get_state()->log_reg_write.clear();
  p->get_state()->log_mem_read.clear();
  p->get_state()->log_mem_write.clear();
}

static void commit_log_stash_privilege(processor_t* p)
{
  state_t* state = p->get_state();
  state->last_inst_priv = state->prv;
  state->last_inst_xlen = p->get_xlen();
  state->last_inst_flen = p->get_flen();
}

static void commit_log_print_value(FILE *log_file, int width, const void *data)
{
  assert(log_file);

  switch (width) {
    case 8:
      fprintf(log_file, "0x%01" PRIx8, *(const uint8_t *)data);
      break;
    case 16:
      fprintf(log_file, "0x%04" PRIx16, *(const uint16_t *)data);
      break;
    case 32:
      fprintf(log_file, "0x%08" PRIx32, *(const uint32_t *)data);
      break;
    case 64:
      fprintf(log_file, "0x%016" PRIx64, *(const uint64_t *)data);
      break;
    default:
      // max lengh of vector
      if (((width - 1) & width) == 0) {
        const uint64_t *arr = (const uint64_t *)data;

        fprintf(log_file, "0x");
        for (int idx = width / 64 - 1; idx >= 0; --idx) {
          fprintf(log_file, "%016" PRIx64, arr[idx]);
        }
      } else {
        abort();
      }
      break;
  }
}

static void commit_log_print_value(FILE *log_file, int width, uint64_t val)
{
  commit_log_print_value(log_file, width, &val);
}

const char* processor_t::get_symbol(uint64_t addr)
{
  return sim->get_symbol(addr);
}

static void commit_log_print_insn(processor_t *p, reg_t pc, insn_t insn)
{
  FILE *log_file = p->get_log_file();

  auto& reg = p->get_state()->log_reg_write;
  auto& load = p->get_state()->log_mem_read;
  auto& store = p->get_state()->log_mem_write;
  int priv = p->get_state()->last_inst_priv;
  int xlen = p->get_state()->last_inst_xlen;
  int flen = p->get_state()->last_inst_flen;

  // print core id on all lines so it is easy to grep
  fprintf(log_file, "core%4" PRId32 ": ", p->get_id());

  fprintf(log_file, "%1d ", priv);
  commit_log_print_value(log_file, xlen, pc);
  fprintf(log_file, " (");
  commit_log_print_value(log_file, insn.length() * 8, insn.bits());
  fprintf(log_file, ")");
  bool show_vec = false;

  for (auto item : reg) {
    if (item.first == 0)
      continue;

    char prefix;
    int size;
    int rd = item.first >> 4;
    bool is_vec = false;
    bool is_vreg = false;
    switch (item.first & 0xf) {
    case 0:
      size = xlen;
      prefix = 'x';
      break;
    case 1:
      size = flen;
      prefix = 'f';
      break;
    case 2:
      size = p->VU.VLEN;
      prefix = 'v';
      is_vreg = true;
      break;
    case 3:
      is_vec = true;
      break;
    case 4:
      size = xlen;
      prefix = 'c';
      break;
    default:
      assert("can't been here" && 0);
      break;
    }

    if (!show_vec && (is_vreg || is_vec)) {
        fprintf(log_file, " e%ld %s%ld l%ld",
                p->VU.vsew,
                p->VU.vflmul < 1 ? "mf" : "m",
                p->VU.vflmul < 1 ? (reg_t)(1 / p->VU.vflmul) : (reg_t)p->VU.vflmul,
                p->VU.vl->read());
        show_vec = true;
    }

    if (!is_vec) {
      if (prefix == 'c')
        fprintf(log_file, " c%d_%s ", rd, csr_name(rd));
      else
        fprintf(log_file, " %c%2d ", prefix, rd);
      if (is_vreg)
        commit_log_print_value(log_file, size, &p->VU.elt<uint8_t>(rd, 0));
      else
        commit_log_print_value(log_file, size, item.second.v);
    }
  }

  for (auto item : load) {
    fprintf(log_file, " mem ");
    commit_log_print_value(log_file, xlen, std::get<0>(item));
  }

  for (auto item : store) {
    fprintf(log_file, " mem ");
    commit_log_print_value(log_file, xlen, std::get<0>(item));
    fprintf(log_file, " ");
    commit_log_print_value(log_file, std::get<2>(item) << 3, std::get<1>(item));
  }
  fprintf(log_file, "\n");
}
#else
static void commit_log_reset(processor_t* p) {}
static void commit_log_stash_privilege(processor_t* p) {}
static void commit_log_print_insn(processor_t* p, reg_t pc, insn_t insn) {}
#endif

inline void processor_t::update_histogram(reg_t pc)
{
#ifdef RISCV_ENABLE_HISTOGRAM
  pc_histogram[pc]++;
#endif
}

// This is expected to be inlined by the compiler so each use of execute_insn
// includes a duplicated body of the function to get separate fetch.func
// function calls.
//
static inline reg_t execute_insn(processor_t* p, reg_t pc, insn_fetch_t fetch,
        bool run_cosim_insn = false)
{
  commit_log_reset(p);
  commit_log_stash_privilege(p);
  reg_t npc;

  try {
      if (p->get_cosim_enabled()){
        if (run_cosim_insn == true){
              std::string insn_str= p->get_insn_string(fetch.insn);
              fprintf(p->get_cosim_log_file(), "proc%4" PRId32 ": ",
                  p->get_id());
              fprintf(p->get_cosim_log_file(), insn_str.c_str());
              fprintf(p->get_cosim_log_file(), "\n");
        }
        // if is cosim instrcution, throw
        // if (p->is_fence(fetch.insn)&& run_cosim_insn == false){
        if (p->is_cosim_insn(fetch.insn)&& run_cosim_insn == false){
            fprintf(p->get_cosim_log_file(), "proc%4" PRId32 ": ",
                p->get_id());
            fprintf(p->get_cosim_log_file(), " meet cosim insn, throw\n");
            throw cosim_fence_t(fetch.insn);
        }

      }
    npc = fetch.func(p, fetch.insn, pc);
    if (npc != PC_SERIALIZE_BEFORE) {

#ifdef RISCV_ENABLE_COMMITLOG
      if (p->get_log_commits_enabled()) {
        commit_log_print_insn(p, pc, fetch.insn);
      }
#endif

     }
#ifdef RISCV_ENABLE_COMMITLOG
  } catch (wait_for_interrupt_t &t) {
      if (p->get_log_commits_enabled()) {
        commit_log_print_insn(p, pc, fetch.insn);
      }
      throw;
  } catch(mem_trap_t& t) {
      //handle segfault in midlle of vector load/store
      if (p->get_log_commits_enabled()) {
        for (auto item : p->get_state()->log_reg_write) {
          if ((item.first & 3) == 3) {
            commit_log_print_insn(p, pc, fetch.insn);
            break;
          }
        }
      }
      throw;
#endif
  } catch(...) {
    throw;
  }
  p->update_histogram(pc);

  return npc;
}


bool processor_t::is_fence(insn_t insn){
    std::string insn_str = this->get_insn_string(insn);
    if (insn_str.find("fence")!=std::string::npos){
        std::cout << "fence inst " << insn_str << std::endl;
        return true;
    }else{
        return false;
    }
}

bool processor_t::is_cosim_insn(insn_t insn){
    std::string insn_str = this->get_insn_string(insn);
    if (insn_str.find(this->cosim_insn)!=std::string::npos){
        fprintf(this->cosim_log_file, "catch cosim insn: ");
        fprintf(this->cosim_log_file, insn_str.c_str());
        fprintf(this->cosim_log_file, "\n");
        return true;
    }else{
        return false;
    }
}

std::string processor_t::get_insn_string(insn_t insn){
    return this->disassembler->disassemble(insn);
}

bool processor_t::slow_path()
{
  return debug || state.single_step != state.STEP_NONE || state.debug_mode;
}

// fetch/decode/execute loop
void processor_t::step(size_t n)
{
  if (!state.debug_mode) {
    if (halt_request == HR_REGULAR) {
      enter_debug_mode(DCSR_CAUSE_DEBUGINT);
    } else if (halt_request == HR_GROUP) {
      enter_debug_mode(DCSR_CAUSE_GROUP);
    } // !!!The halt bit in DCSR is deprecated.
    else if (state.dcsr->halt) {
      enter_debug_mode(DCSR_CAUSE_HALT);
    }
  }
  this->inst_count = 0;
  while (n > 0) {
    size_t instret = 0;
    size_t fence_steps = -1; // steps in take_cosim_fence();
    reg_t pc = state.pc;
    mmu_t* _mmu = mmu;

    #define advance_pc() \
     if (unlikely(invalid_pc(pc))) { \
       switch (pc) { \
         case PC_SERIALIZE_BEFORE: state.serialized = true; break; \
         case PC_SERIALIZE_AFTER: ++instret; break; \
         case PC_SERIALIZE_WFI: n = ++instret; break; \
         default: abort(); \
       } \
       pc = state.pc; \
       break; \
     } else { \
       /*std::cout << "[advance_pc() invalid_pc]" << std::endl;*/ \
       state.pc = pc; \
       instret++; \
     }

    try
    {
      take_pending_interrupt();
      if (cosim_enabled){
        // take cosim fence
        pc = take_cosim_fence(mmu, pc, &fence_steps);
        if (fence_steps == (size_t)-1){
          // the pending fence not complete, keep wait
          break;
        }else{
          if (fence_steps != 0){
            fprintf(get_cosim_log_file(), "proc%4" PRId32 ": ",
                get_id());
            fprintf(cosim_log_file, " take_cosim_fence spend %d", fence_steps);
            fprintf(cosim_log_file, " steps\n");
          }
          instret+=fence_steps;
        }
      }

      if (unlikely(slow_path()))
      {
        // Main simulation loop, slow path.
        while (instret < n)
        {
          if (unlikely(!state.serialized && state.single_step == state.STEP_STEPPED)) {
            state.single_step = state.STEP_NONE;
            if (!state.debug_mode) {
              enter_debug_mode(DCSR_CAUSE_STEP);
              // enter_debug_mode changed state.pc, so we can't just continue.
              break;
            }
          }

          if (unlikely(state.single_step == state.STEP_STEPPING)) {
            state.single_step = state.STEP_STEPPED;
          }

          insn_fetch_t fetch = mmu->load_insn(pc);
          if (debug && !state.serialized)
            disasm(fetch.insn);
          pc = execute_insn(this, pc, fetch);
          advance_pc();
        }
      }
      else while (instret < n)
      {
        // Main simulation loop, fast path.
        for (auto ic_entry = _mmu->access_icache(pc); ; ) {
          auto fetch = ic_entry->data;
          pc = execute_insn(this, pc, fetch);
          ic_entry = ic_entry->next;
          if (unlikely(ic_entry->tag != pc))
            break;
          if (unlikely(instret + 1 == n))
            break;
          instret++;
          state.pc = pc;
        }
//        std::cout << "Run advance_pc()" << std::endl;
        advance_pc();
      }
    }
    catch(trap_t& t)
    {
      //std::cout << "[DEBUG] taking trap " << this->get_insn_string(t.get_tinst())
      //   <<  std::endl;
      take_trap(t, pc);
      n = instret;

      if (unlikely(state.single_step == state.STEP_STEPPED)) {
        state.single_step = state.STEP_NONE;
        enter_debug_mode(DCSR_CAUSE_STEP);
      }
    }
    catch (trigger_matched_t& t)
    {
      if (mmu->matched_trigger) {
        // This exception came from the MMU. That means the instruction hasn't
        // fully executed yet. We start it again, but this time it won't throw
        // an exception because matched_trigger is already set. (All memory
        // instructions are idempotent so restarting is safe.)

        insn_fetch_t fetch = mmu->load_insn(pc);
        pc = execute_insn(this, pc, fetch);
        advance_pc();

        delete mmu->matched_trigger;
        mmu->matched_trigger = NULL;
      }
      switch (state.mcontrol[t.index].action) {
        case ACTION_DEBUG_MODE:
          enter_debug_mode(DCSR_CAUSE_HWBP);
          break;
        case ACTION_DEBUG_EXCEPTION: {
          trap_breakpoint trap(state.v, t.address);
          take_trap(trap, pc);
          break;
        }
        default:
          abort();
      }
    }
    catch (wait_for_interrupt_t &t)
    {
      // Return to the outer simulation loop, which gives other devices/harts a
      // chance to generate interrupts.
      //
      // In the debug ROM this prevents us from wasting time looping, but also
      // allows us to switch to other threads only once per idle loop in case
      // there is activity.
      n = ++instret;
    }
    catch (cosim_fence_t &t){
        // TODO
        // waiting for cosim-fence , do nothing
        fprintf(this->cosim_log_file, "proc%4" PRId32 ": ",
            this->get_id());
        fprintf(this->cosim_log_file, " catch cosim_fence\n");
        // create a spike event
        spike_event_t* event = createRoccInstEvent(instret, instret, t.get_cosim_insn());
        t.set_spike_event(event);
        this->cosim_fence_table.push_back(&t);
        n = ++instret;
        //std::cout << "instret: " << instret << std::endl;
    }
    state.minstret->bump(instret);
    this->inst_count += instret;
    n -= instret;
  }
  /*
  fprintf(this->cosim_log_file, "proc%4" PRId32 ": ",
    this->get_id());
  fprintf(this->cosim_log_file, " executed%8l " PRId32 " instructions\n",
    this->inst_count);
  */
  this->executed_inst += this->inst_count;
}

reg_t  processor_t::take_cosim_fence(mmu_t* _mmu, reg_t pc, size_t *steps){
    // TODO 
    // check the events in cosim_event_table,
    // if the event is finished, run the pending fence instruction
    // else keep waiting
    if (this->cosim_fence_table.empty()){
        *steps = 0; // TODO: not a good way
        return pc;
    }else{
       fprintf(cosim_log_file, "proc%4" PRId32 ": ", this->get_id());
       fprintf(cosim_log_file, " has pending fence\n");
       cosim_fence_t* cosim_fence = this->cosim_fence_table.front();
       spike_event_t* first_event = cosim_fence->get_spike_event();
       if (first_event->isFinished()){
           size_t step = 0;
           fprintf(cosim_log_file, "proc%4" PRId32 ": ", this->get_id());
           fprintf(cosim_log_file, " pending fence finished\n");
           // pc run a instruction and release the cosim_fence
           this->cosim_fence_table.pop_front();
           //reg_t pc = state.pc;
           for (auto ic_entry = _mmu->access_icache(pc); ; ){
               insn_fetch_t fetch = ic_entry->data;
               //insn_fetch_t fetch = mmu->load_insn(pc);
               if (debug && !state.serialized){
                    disasm(fetch.insn);
               }
               pc = execute_insn(this, pc, fetch, true);
               step++;
               ic_entry = ic_entry->next;
               if (unlikely(ic_entry->tag != pc)){
                   break;
               }
               this->state.pc = pc;
           }
           if (unlikely(invalid_pc(pc))){
               switch (pc){
                     case PC_SERIALIZE_BEFORE: state.serialized = true; break;
                     case PC_SERIALIZE_AFTER: ++step; break;
                     case PC_SERIALIZE_WFI: std::cout << "[WFI] after take cosim fence" << std::endl; break;
                    default: abort();
               }
               pc = this->state.pc;
           }else{
               this->state.pc = pc;
               step++;
           }
           // update the steps, let processor know how many step spent
           *steps = step;
       }else{
           fprintf(cosim_log_file, "proc%4" PRId32 ": ", this->get_id());
           fprintf(cosim_log_file, " pending fence not finished\n");
           //TODO keep waiting, no need to add a new cosim fence
           // return a nullptr steps
       }
       fprintf(cosim_log_file, "proc%4" PRId32 ": ", this->get_id());
       fprintf(cosim_log_file, " take cosim fence complete\n");
    }
    return pc;
}

spike_event_t* processor_t::get_first_spike_event(){
    if (this->cosim_fence_table.empty()){
        return NULL;
    }
    cosim_fence_t* cosim_fence = this->cosim_fence_table.front();
    spike_event_t* event = cosim_fence->get_spike_event();
    return event;
}

size_t processor_t::get_executed_inst(){
    return this->executed_inst;
}

size_t processor_t::get_inst_count(){
    return this->inst_count;
}

void processor_t::enable_cosim(bool enable_cosim, FILE* cosim_log){
    this->cosim_enabled = true;
    this->cosim_log_file = cosim_log;
}

// TODO change to a vector to save the cosim instructions
void processor_t::set_cosim_insn(const char* cosim_insn){
    this->cosim_insn = cosim_insn;
}
