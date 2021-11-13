/** \file 
 *  \brief Header for stack dump
 */
#ifndef DUMP_H
#define DUMP_H

#include "config.h"

#ifndef __USE_MINGW_ANSI_STDIO
#define __USE_MINGW_ANSI_STDIO 1
#endif
#include <stdio.h>

/** \brief Sets function for printing elements in dump
 *  
 *  \param dumpstream Stream for dump (if 0 passed sets stream with STACK_DUMPFILE)
 *  \param print_func Function printing Elem_t to FILE (if 0 passed elements won't be printed)
 */
void stack_dump_init(FILE* dumpstream, void (*print_func)(FILE*, const Elem_t*));

#ifdef DUMP
const char BAD_ALLOCATION[]    = "Allocation has failed\n";
const char BAD_BUFFER[]        = "Buffer is corrupted\n";
const char BAD_STACK_HASH[]    = "Bad stack hash (stack is corrupted)\n";
const char BAD_BUFFER_HASH[]   = "Bad buffer hash (buffer is corrupted)\n";
const char BAD_STACK_CANARY[]  = "Bad stack canary (stack is corrupted)\n";
const char BAD_BUFFER_CANARY[] = "Bad buffer canary (buffer is corrupted)\n";
const char REINITIALIZING[]    = "Trying to initialize already initialized stack\n";
const char REDESTRUCTING[]     = "Trying to destroy already destroyed stack\n";
const char DESTRUCTED[]        = "Stack is destructed\n";
const char SIZE_OVER_CAP[]     = "Size is greater than capacity\n";
const char CAP_OVER_SIZE[]     = "Capacity is greater than needed for current size\n";
const char POP_EMPTY_STACK[]   = "Trying to pop from empty stack\n";
const char NULLPOINTER[]       = "Nullptr was passed\n";

struct Stack;

/// \brief Sets level of dump  
enum Stack_dump_lvl
{
    ONLYERR  = 0, ///< dumps only in case of error
    BRIEF    = 1, ///< dumps one-line dump
    DETAILED = 2, ///< dumps all information about stack condition
};

void dump_(const Stack* const stk, Stack_err err, Stack_dump_lvl level, const char msg[],
           const char func[], const char file[], int line);

Stack_err stack_dump_(const Stack* const stk, const char msg[],
                      const char func[], const char file[], int line);

#endif // DUMP

#endif // DUMP_H
