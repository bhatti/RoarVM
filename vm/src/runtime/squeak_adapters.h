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


# define main sqr_main

# define clone sqr_clone

# if On_Tilera && !defined(__APPLE__) || defined(RVM_CODE_NOT_SQUEAK_CODE)
typedef int sqInt;
typedef long long sqLong;
# endif

# ifdef __cplusplus
extern "C" { char* pointerForIndex_xxx_dmu(sqInt); }
# endif

# ifdef __cplusplus
extern "C" {
# endif

void rvm_exit();
void print_vm_info();

# ifdef __cplusplus
}
# endif

