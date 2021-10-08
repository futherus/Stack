#ifndef STACK_H
#define STACK_H

#include <stdint.h>
#include "config.h"

const size_t STACK_CAP_MULTPLR = 2;
const size_t STACK_MIN_CAP = 4 * STACK_CAP_MULTPLR;
const unsigned char BYTE_POISON = 0xBD;

enum ErrType
{
    NOERR           = 0,       /// no message
    BAD_ALLOC       = 1 << 0,  /// error with allocating
    BAD_BUF         = 1 << 1,  /// invalid buffer
    BAD_STK_HSH     = 1 << 2,  /// bad stack hash (stack is corrupted)
    BAD_BUF_HSH     = 1 << 3,  /// bad buffer hash (buffer is corrupted)
    BAD_STK_CAN     = 1 << 4,  /// bad stack canary (stack is corrupted)
    BAD_BUF_CAN     = 1 << 5,  /// bad buffer canary (buffer is corrupted)
    REINIT          = 1 << 6,  /// reinitialization
    REDESTR         = 1 << 7,  /// redestruction
    DSTRCTED        = 1 << 8,  /// initialization of destructed stack
    SZ_OVR_CAP      = 1 << 9,  /// size is over capacity
    CAP_OVR_SZ      = 1 << 10, /// capacity is too large for current size
    POP_EMPT_STCK   = 1 << 11, /// pop from empty stack
    NULLPTR         = 1 << 12, /// nullptr was passed
    UNEXPCTD_ERR    = 1 << 13, /// ERROR
};

#ifdef DEBUG //////////////////////////////////////////////////////////////////

#include "dump.h"

typedef uint64_t guard_t;
const size_t SIZE_POISON = 0x1BADBADBADBADBAD;
const Elem_t* const BUF_POISON = (Elem_t*) 0x000000000BAD;

#ifdef CANARY
const guard_t DEFAULT_CANARY = 0xBAC1CAB1DED1BED1;
#endif // CANARY

struct Stack
{
#ifdef CANARY
    guard_t beg_can = 0;
#endif // CANARY
#ifdef HASH
    guard_t stk_hash = 0;
#endif // HASH

    Elem_t* buffer = nullptr;

    size_t preset_cap = 0;
    size_t size = 0;
    size_t capacity = 0;

#ifdef DUMP
    const char* init_func = nullptr;
    const char* init_file = 0;
    int init_line = 0;
#endif // DUMP
#ifdef HASH
#ifdef BUFFER_HASH
    guard_t buf_hash = 0;
#endif // BUFFER_HASH
#endif // HASH
#ifdef CANARY
    guard_t end_can = 0;
#endif // CANARY
};

ErrType stack_verify_(const Stack* const stk);

ErrType stack_init_(Stack* stk, size_t size, const char func[], const char file[], int line);

ErrType stack_push_(Stack* stk, Elem_t elem, const char func[], const char file[], int line);

ErrType stack_pop_(Stack* stk, Elem_t* elem, const char func[], const char file[], int line);

ErrType stack_dstr_(Stack* stk, const char func[], const char file[], int line);

#ifdef DUMP
#define stack_dump(stk, msg) stack_dump_((stk), (msg), __PRETTY_FUNCTION__, __FILE__, __LINE__);  
#else // DUMP
#define stack_dump(stk, msg)
#endif // DUMP

#define stack_init(stk, size)                                                \
        stack_init_((stk), (size), __PRETTY_FUNCTION__, __FILE__, __LINE__); \

#define stack_push(stk, elem)                                                \
        stack_push_((stk), (elem), __PRETTY_FUNCTION__, __FILE__, __LINE__); \

#define stack_pop(stk, elem)                                                 \
        stack_pop_((stk), (elem), __PRETTY_FUNCTION__, __FILE__, __LINE__);  \

#define stack_dstr(stk)                                                      \
        stack_dstr_((stk), __PRETTY_FUNCTION__, __FILE__, __LINE__);         \

#else //////////////////////////////////////////////////////////////////////////

struct Stack
{
    Elem_t* buffer = nullptr;

    size_t preset_cap = 0;
    size_t size = 0;
    size_t capacity = 0;
};

ErrType stack_init_(Stack* stk, size_t size);

ErrType stack_push_(Stack* stk, Elem_t elem);

ErrType stack_pop_(Stack* stk, Elem_t* elem);

ErrType stack_dstr_(Stack* stk);

#define stack_dump(stk, msg)

#define stack_init(stk, size)      \
        stack_init_((stk), (size))

#define stack_push(stk, elem)      \
        stack_push_((stk), (elem))

#define stack_pop(stk, elem)       \
        stack_pop_((stk), (elem))

#define stack_dstr(stk)            \
        stack_dstr_((stk))

#endif // DEBUG ////////////////////////////////////////////////////////////////
#endif // STACK_H