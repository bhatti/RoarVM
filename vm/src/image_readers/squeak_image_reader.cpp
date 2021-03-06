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


#include "headers.h"

# include <fcntl.h>
# include <unistd.h>


void Squeak_Image_Reader::read(char* fileName, Memory_System* ms, Squeak_Interpreter* i) {
  Squeak_Image_Reader* sir = new Squeak_Image_Reader(fileName, ms, i);
  sir->read_image();
  dittoing_stdout_printer->printf("finished reading %s\n", fileName);
}

void Squeak_Image_Reader::fake_read(char* fileName, Memory_System* ms, Squeak_Interpreter* i) {
  Squeak_Image_Reader* sir = new Squeak_Image_Reader(fileName, ms, i);
  sir->read_header();

  //mimic reader
  Memory_Semantics::shared_malloc(sir->dataSize);
  malloc(sir->dataSize);

  i->restore_all_from_checkpoint(sir->dataSize, sir->lastHash, sir->savedWindowSize, sir->fullScreenFlag);
  imageNamePut_on_all_cores(sir->file_name, strlen(sir->file_name));
}

Squeak_Image_Reader::Squeak_Image_Reader(char* fn, Memory_System* ms, Squeak_Interpreter* i) {
  dittoing_stdout_printer->printf("Reading %s\n", fn);
  memory_system = ms;
  interpreter = i;

  file_name = fn;
  image_file = fopen(file_name, "r");
  swap_bytes = false;
  if (image_file == NULL) {
    char buf[BUFSIZ];
    snprintf(buf, sizeof(buf), "Could not open image file %s", file_name);
    perror(buf);
    abort();
  }
}

void Squeak_Image_Reader::read_image() {
/*

readImageFromFile: f HeapSize: desiredHeapSize StartingAt: imageOffset
	"Read an image from the given file stream, allocating the given amount of memory to its object heap. Fail if the image has an unknown format or requires more than the given amount of memory."
	"Details: This method detects when the image was stored on a machine with the opposite byte ordering from this machine and swaps the bytes automatically. Furthermore, it allows the header information to start 512 bytes into the file, since some file transfer programs for the Macintosh apparently prepend a Mac-specific header of this size. Note that this same 512 bytes of prefix area could also be used to store an exec command on Unix systems, allowing one to launch Smalltalk by invoking the image name as a command."
	"This code is based on C code by Ian Piumarta and Smalltalk code by Tim Rowledge. Many thanks to both of you!!"
*/

  Object::verify_constants();

  read_header();

  fprintf(stdout, "allocating memory for snapshot\n");

  // "allocate a contiguous block of memory for the Squeak heap"
  memory = (char*)Memory_Semantics::shared_malloc(dataSize);
  assert_always(memory != NULL);

  /*
	memStart := self startOfMemory.
	memoryLimit := (memStart + heapSize) - 24.  "decrease memoryLimit a tad for safety"
	endOfMemory := memStart + dataSize.
  */

  // "position file after the header"
  fprintf(stdout, "reading objects in snapshot\n");
  if ( fseek(image_file, headerStart + headerSize, SEEK_SET))
    perror("seek"), fatal();

  // "read in the image in bulk, then swap the bytes if necessary"
  xfread(memory, 1, dataSize, image_file);

  // "First, byte-swap every word in the image. This fixes objects headers."
  if (swap_bytes) reverseBytes((int32*)&memory[0], (int32*)&memory[dataSize]);

  // "Second, return the bytes of bytes-type objects to their orginal order."
  if (swap_bytes) byteSwapByteObjects();

  Safepoint_Ability sa(false); // for distributing objects and putting image name
  distribute_objects();
  imageNamePut_on_all_cores(file_name, strlen(file_name));
}


void Squeak_Image_Reader::imageNamePut_on_all_cores(char* bytes, unsigned int len) {
  // Use a shared buffer to reduce the size of the message to optimize the footprint of message buffer allocation -- dmu & sm
  char* shared_buffer = (char*)Memory_Semantics::shared_malloc(len);
  bcopy(bytes, shared_buffer, len);
  imageNamePutMessage_class m(shared_buffer, len);
  if (On_Tilera) m.send_to_all_cores();
  else           m.handle_me();
  free(shared_buffer);
}


void Squeak_Image_Reader::read_header() {
  fprintf(stdout, "reading snapshot header\n");
  check_image_version();
  // headerStart := (self sqImageFilePosition: f) - bytesPerWord.  "record header start position"
  headerStart = ftell(image_file) - bytesPerWord;
  headerSize         = get_long();
  dataSize           = get_long();
  oldBaseAddr        = (char*)get_long();
  specialObjectsOop = Oop::from_bits(get_long());

  lastHash = get_long();
  if (lastHash == 0) lastHash = 999;

  savedWindowSize    = get_long();
  fullScreenFlag     = get_long();


  extraVMMemory      = get_long();
}

void Squeak_Image_Reader::check_image_version() {
    int32 first_version = get_long();
    interpreter->image_version = first_version;
    if ( readable_format(interpreter->image_version) ) return;
    swap_bytes = true;
    if (fseek(image_file, -sizeof(int32), SEEK_CUR) != 0) {
      perror("seek failed"); fatal();
    }
    interpreter->image_version = get_long();
    if ( readable_format(interpreter->image_version) ) return;

    fatal("cannot read file");

}

int32 Squeak_Image_Reader::get_long() {
  int32 x;
  xfread(&x, sizeof(x), 1, image_file);
  if (swap_bytes) swap_bytes_long(&x);
  return x;
}

bool Squeak_Image_Reader::readable_format(int32 version) {
  /* Check against a magic constant that changes when the image format changes. Since the image reading code uses this to detect byte ordering, one must avoid version numbers that are invariant under byte reversal. */
  return version == Pre_Closure_32_Bit_Image_Version ||  version == Post_Closure_32_Bit_Image_Version;
}




void Squeak_Image_Reader::byteSwapByteObjects() {
  fprintf(stdout, "swapping bytes back in byte objects\n");
  for (Chunk* c = (Chunk*)memory;
       (char*)c <  &memory[dataSize]; ) {
    Object* obj = c->object_from_chunk_without_preheader();
    obj->byteSwapIfByteObject();
    c = obj->nextChunk();
  }
  fprintf(stdout, "done swapping bytes back in byte objects\n");
}


class Convert_Closure: public Oop_Closure {
  Squeak_Image_Reader* reader;
public:
  Convert_Closure(Squeak_Image_Reader* r)  : Oop_Closure() { reader = r; }
  void value(Oop* p, Object*) {
    if (p->is_mem()) {
      *p = reader->oop_for_oop(*p);
    }
  }
  virtual const char* class_name(char*) { return "Convert_Closure"; }
};



void Squeak_Image_Reader::distribute_objects() {
  fprintf(stdout, "distributing objects\n");
  u_int64 start = OS_Interface::get_cycle_count();
  char* base = memory;
  u_int32 total_bytes = dataSize;

  object_oops = (Oop*)malloc(total_bytes);
  bzero(object_oops, total_bytes);
  Convert_Closure cc(this);

  memory_system->initialize_from_snapshot(dataSize, savedWindowSize, fullScreenFlag, lastHash);
  
  for (Chunk *c = (Chunk*)base, *nextChunk = NULL;
       (char*)c <  &base[total_bytes];
       c = nextChunk) {
    Object* obj = c->object_from_chunk_without_preheader();
    if (check_many_assertions &&  (char*)obj - memory == (char*)specialObjectsOop.bits() - oldBaseAddr)
      lprintf("about to do specialObjectsOop");
    nextChunk = obj->nextChunk();
    if (!obj->isFreeObject()) {
      obj->do_all_oops_of_object_for_reading_snapshot(this);
      memory_system->ask_cpu_core_to_add_object_from_snapshot_allocating_chunk(oop_for_addr(obj), obj);
    }
  }
  cc.value(&specialObjectsOop, NULL);

  memory_system->finished_adding_objects_from_snapshot();
  free(object_oops);
  fprintf(stdout, "done distributing objects, %lld cycles\n",
    OS_Interface::get_cycle_count() - start);

  interpreter->initialize(specialObjectsOop, false);
}



Oop Squeak_Image_Reader::oop_for_oop(Oop x) {
  return oop_for_relative_addr(x.bits() - (int)oldBaseAddr);
}

Oop Squeak_Image_Reader::oop_for_addr(Object* obj) {
  return oop_for_relative_addr(int(obj) - int(memory));
}


Oop Squeak_Image_Reader::oop_for_relative_addr(int relative_addr) {
  assert(relative_addr < dataSize);
  Oop* addr_in_table = &object_oops[relative_addr / sizeof(Oop)];
  Object* obj = (Object*) &memory[relative_addr];
  if (addr_in_table->bits() == 0) {
    Multicore_Object_Table* ot = memory_system->object_table;
    *addr_in_table = ot->allocate_OTE_for_object_in_snapshot(obj);
  }
  return *addr_in_table;
}

