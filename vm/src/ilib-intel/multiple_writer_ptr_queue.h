/******************************************************************************
 *  Copyright (c) 2008 - 2010 IBM Corporation and others.
 *  All rights reserved. This program and the accompanying materials
 *  are made available under the terms of the Eclipse Public License v1.0
 *  which accompanies this distribution, and is available at
 *  http://www.eclipse.org/legal/epl-v10.html
 * 
 *  Contributors:
 *    Stefan Marr, Vrije Universiteit Brussel - Initial Implementation
 *
 ******************************************************************************/


#include <queue>
#include <pthread.h>

class MultipleWriterPtrQueue {
public:
  MultipleWriterPtrQueue();

	void enqueue(const void* element);
  const void* dequeue();
  bool empty();

private:
  std::queue<const void*>* queue;
  pthread_mutex_t lock;
};

