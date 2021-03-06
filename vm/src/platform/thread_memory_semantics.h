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


# if !On_Tilera

class Thread_Memory_Semantics : public Abstract_Memory_Semantics {
private:
  static pthread_key_t my_core_key;
  
  static void _dtor_my_core_key(void* value) {
    pthread_setspecific(my_core_key, NULL);
  }

// It is not necessary to replicate the memory system in shared-memory
// environments, this is the standard case for the RVM.
// However, to have a version with equal functionallity, this flag can be
// used to enforce replication.
# if  Replicate_PThread_Memory_System
private:
  static void _dtor_memory_system_key(void* local_obj);
public:
  static pthread_key_t memory_system_key;
  static void initialize_memory_system();
# endif
  
public:
  static void initialize_memory_system();
  static void initialize_local_memory_system();
  
# if !Force_Direct_Squeak_Interpreter_Access
  // For test it can be enforced to use the same strategy as for processes
  // to allocate the interpreter, but that is usually not done.
  // Furthermore, if Force_Direct_Squeak_Interpreter_Access would be set,
  // the RVM could only use a single core.
private:
  static void _dtor_interpreter(void* local_head);
public:
  static pthread_key_t interpreter_key;
# endif
  
  static void initialize_interpreter();
  static void initialize_local_interpreter();
  
  static inline bool cores_are_initialized() { return my_core_key != 0; }
  
  static const size_t max_num_threads_on_threads_or_1_on_processes = Max_Number_Of_Cores;
  
  static Logical_Core* my_core();
  static int my_rank();
  static u_int64 my_rank_mask();
  static inline size_t rank_on_threads_or_zero_on_processes() { return my_rank(); } 

  static void initialize_logical_cores();
  static void initialize_local_logical_core();
  static void initialize_local_logical_core(int rank);

  
  static void go_parallel(void (*helper_core_main)(), char* argv[]) { 
    OS_Interface::start_threads(helper_core_main, argv);
  }
  
  static inline int get_group_rank() { return OS_Interface::get_thread_rank(); }
  
  static inline void* shared_malloc(u_int32 sz) {
    return malloc(sz);
  }
  static inline void* shared_calloc(u_int32 num_members, u_int32 mem_size)  {
    return calloc(num_members, mem_size);
  }
  
  static char* map_heap_memory(int total_size, int bytes_to_map,
                               int page_size_used_in_heap_arg, void* where, int offset,
                               int main_pid, int flags);
    
};

class Memory_System;
# if  !Replicate_PThread_Memory_System
  extern Memory_System _memory_system;

  inline __attribute__((always_inline)) Memory_System* The_Memory_System() {
    return &_memory_system;
  };
# else
  inline __attribute__((always_inline)) Memory_System* The_Memory_System() {
    return (Memory_System*)pthread_getspecific(Memory_Semantics::memory_system_key);
  };
# endif



# if Force_Direct_Squeak_Interpreter_Access
  extern Squeak_Interpreter _interpreter;

  //#define The_Squeak_Interpreter() (&_interpreter)
  // At least the Tilera compiler does not like the inlines, costs about 2-5% performance
  inline  __attribute__((always_inline)) Squeak_Interpreter* The_Squeak_Interpreter() { return &_interpreter;  }
# else
  inline __attribute__((always_inline)) Squeak_Interpreter* The_Squeak_Interpreter() {
    assert(Thread_Memory_Semantics::interpreter_key !=0 /* ensure it is initialized */);
    return (Squeak_Interpreter*)pthread_getspecific(Thread_Memory_Semantics::interpreter_key);
  }
# endif

# endif // !On_Tilera

