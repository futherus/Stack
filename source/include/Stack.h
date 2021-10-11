/** \file 
 *  \brief Header containing Stack structure and it's main functions
 */

#ifndef STACK_H
#define STACK_H

/// \brief Error enum for reporting errors
enum Stack_err
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
    POP_EMPT_STK    = 1 << 11, /// pop from empty stack (WARNING: is not shown in dump)
    NULLPTR         = 1 << 12, /// nullptr was passed
};

#include <stdint.h>
#include "config.h"
#include "dump.h"

/// \brief Capacity multiplier for reallocating
const size_t STACK_CAP_MULTPLR  = 2;
/// \brief Minimal capacity 
const size_t STACK_MIN_CAP      = 4 * STACK_CAP_MULTPLR;

const unsigned char BYTE_POISON = 0xBD;

#ifdef DEBUG
typedef uint64_t guard_t;
const size_t SIZE_POISON       = 0x1BADBADBADBADBAD;
const Elem_t* const BUF_POISON = (Elem_t*) 0x000000000BAD;
#endif

#ifdef CANARY
const guard_t DEFAULT_CANARY   = 0xBAC1CAB1DED1BED1;
#endif

struct Stack
{
#ifdef CANARY
                guard_t beg_can       = 0;
#endif
#ifdef STACK_HASH
                guard_t stk_hash      = 0;
#endif

                Elem_t* buffer        = nullptr;

                size_t preset_cap     = 0;
                size_t size           = 0;
                size_t capacity       = 0;

#ifdef DUMP
                const char* init_func = nullptr;
                const char* init_file = 0;
                int init_line         = 0;
#endif 
#ifdef BUFFER_HASH
                guard_t buf_hash      = 0;
#endif
#ifdef CANARY
                guard_t end_can       = 0;
#endif
};

//////////////////////////////////////////////////////////////////////////////
/** \brief Verifies and dumps stack to logfile
 * 
 *  \param stk [in] Pointer to stack
 *  \param msg [in] String message for dump
 */
#ifdef DUMP
#define stack_dump(stk, msg)                                                 \
        stack_dump_((stk), (msg), __PRETTY_FUNCTION__, __FILE__, __LINE__)   
#else 
#define stack_dump(stk, msg) 
#endif // DUMP

/** \brief Initializes stack
 * 
 *  \param stk  [in][out] Pointer to stack
 *  \param size [in]      Initial size for stack (if 0 stack buffer is not allocated)
 * 
 *  \return Stack_err::NOERR if succeed and error number otherwise
 *  \warning Memory for stack structure should be free
 */
#define stack_init(stk, size)                                                \
        stack_init_((stk), (size)                                            \
                         DUMP_ON(__PRETTY_FUNCTION__, __FILE__, __LINE__))   \

/** \brief Pushes element to stack
 * 
 *  \param stk  [in][out]  Pointer to stack
 *  \param elem [in]       Element to be pushed
 * 
 *  \return Stack_err::NOERR if succeed and error number otherwise
 */
#define stack_push(stk, elem)                                                \
        stack_push_((stk), (elem)                                            \
                         DUMP_ON(__PRETTY_FUNCTION__, __FILE__, __LINE__))   \

/** \brief Pops element from stack
 * 
 *  \param stk  [in][out]  Pointer to stack
 *  \param elem [out]      Pointer to variable to write popped element
 * 
 *  \return Stack_err::NOERR if succeed and error number otherwise
 *  \warning Pop from empty stack returns Stack_err::POP_EMPT_STK (even if DEBUG is not defined)
 *           but is not shown in dump
 *  \warning Nullptr as second argument results in Stack_err::NULLPTR and error message in dump
 */
#define stack_pop(stk, elem)                                                 \
        stack_pop_((stk), (elem)                                             \
                        DUMP_ON(__PRETTY_FUNCTION__, __FILE__, __LINE__))    \

/** \brief Destroys stack
 * 
 *  \param stk [in][out]   Pointer to stack
 * 
 *  \return Stack_err::NOERR if succeed and error number otherwise
 *  \warning Fills fields of stack structure with poison
 */
#define stack_dstr(stk)                                                      \
        stack_dstr_((stk)                                                    \
                 DUMP_ON(__PRETTY_FUNCTION__, __FILE__, __LINE__))           \

//////////////////////////////////////////////////////////////////////////////

#ifdef DUMP
    #define DUMP_ON(arg1, arg2, arg3) , arg1, arg2, arg3
#else
    #define DUMP_ON(arg1, arg2, arg3) 
#endif

Stack_err stack_verify_(const Stack* const stk);

Stack_err stack_init_(Stack* stk, size_t preset_cap
              DUMP_ON(const char func[], const char file[], int line));

Stack_err stack_push_(Stack* stk, Elem_t elem
              DUMP_ON(const char func[], const char file[], int line));

Stack_err stack_pop_ (Stack* stk, Elem_t* elem
              DUMP_ON(const char func[], const char file[], int line));

Stack_err stack_dstr_(Stack* stk
              DUMP_ON(const char func[], const char file[], int line));

#endif // STACK_H