/******************************************************************************
 *  Copyright (c) 2008 - 2010 IBM Corporation and others.
 *  All rights reserved. This program and the accompanying materials
 *  are made available under the terms of the Eclipse Public License v1.0
 *  which accompanies this distribution, and is available at
 *  http://www.eclipse.org/legal/epl-v10.html
 * 
 *  Contributors:
 *    David Ungar, IBM Research - Initial Implementation
 *    Sam Adams, IBM Research - Initial Implementation
 *    Stefan Marr, Vrije Universiteit Brussel - Port to x86 Multi-Core Systems
 ******************************************************************************/


extern "C" { void rvm_exit(); }

class Abstract_OS_Interface {
public:
  
  static inline void abort() { fatal(); }
  static inline void die(const char* err_msg) { fatal(); }
  static inline void exit()  { fatal(); }
  
  static inline void initialize() {}
  
  static void ensure_Time_Machine_backs_up_run_directory() { fatal(); }
  
  static inline void profiler_enable()  { fatal(); }
  static inline void profiler_disable() { fatal(); }
  static inline void profiler_clear()   { fatal(); }
  static inline void sim_end_tracing()  { fatal(); }
  
  typedef int  get_cycle_count_quickly_t;
  static inline int64 get_cycle_count() { fatal(); return 0; }
  
  typedef int Mutex;
  static inline void mutex_init(Mutex*, void*) { fatal(); }
  static inline void mutex_destruct(Mutex*)    { fatal(); }
  static inline int  mutex_lock(Mutex*)        { fatal(); return 0; }
  static inline int  mutex_trylock(Mutex*)     { fatal(); return 0; }
  static inline int  mutex_unlock(Mutex*)      { fatal(); return 0; }
  
  static inline uint32_t leading_zeros(uint32_t x)    { fatal(); return 0; }
  static inline uint32_t population_count(uint32_t x) { fatal(); return 0; }
  
  struct OS_Heap {};
  
  static inline void* rvm_malloc_shared(uint32_t sz) { fatal(); return NULL; }
  static inline void* rvm_calloc_shared(uint32_t num_members, uint32_t mem_size)  { fatal(); return NULL; }
  static inline void* rvm_memalign(OS_Heap, int al, int sz) { fatal(); return NULL; }
  static inline void* rvm_memalign(int al, int sz) { fatal(); return NULL; }
  static inline void* malloc_in_mem(int /* alignment */, int /* size */) { fatal(); return NULL; }
  static inline void  invalidate_mem(void*, size_t) {}
  static inline void  mem_flush(void* ptr, size_t size) {}
  static inline void  mem_fence() {}
  static inline int   mem_create_heap_if_on_Tilera(OS_Heap* heap, bool replicate) { fatal(); return 0; }
  
  /* To enable the RVM to use more than one core, one of the following functions
     has to be implemented.
     Either threads or processes are chosen by configuring shared-by-default
     or private-by-default memory semantics in rvm_config.h. */
  static void start_threads  (void (*)(/* helper_core_main */), char* /* argv */[]) { fatal(); }
  static void start_processes(void (*)(/* helper_core_main */), char* /* argv */[]) { fatal(); }
  
  static inline int get_process_rank() { fatal(); return -1; } 
  static inline int get_thread_rank()  { fatal(); return -1; }
  
  static inline int abort_if_error(const char*, int) { fatal(); return -1; }  
  
};
