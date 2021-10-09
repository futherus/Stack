/** \file 
 *  \brief Header for stack dump
 */
#ifdef DUMP

#ifndef DUMP_H
#define DUMP_H
 
#include <stdio.h>

/** \brief Sets printing elements function for dump
 *  \param stream [in] FILE* pointer
 *  \param elem   [in] Pointer to elem to be printed
 */
void set_print(void (*print_func)(FILE* stream, const Elem_t* elem));

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
const char POP_EMPTY_STACK[]   = "Trying to pop from empty stack";
const char NULLPOINTER[]       = "Nullptr was passed\n";

struct Stack;

enum DumpLevel
{
    ONLYERR = 0,
    BRIEF = 1,
    DETAILED = 2
};


void dump(const Stack* const stk, ErrType err, DumpLevel level, const char msg[],
          const char func[], const char file[], int line);

void stack_dump_(const Stack* const stk, const char msg[],
                 const char func[], const char file[], int line);

#endif // DUMP_H

#endif // DUMP