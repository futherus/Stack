#include "include/config.h"
#include "include/Stack.h"
#include "include/stack_hash.h"

#ifndef __USE_MINGW_ANSI_STDIO
#define __USE_MINGW_ANSI_STDIO 1
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

static size_t stack_init_cap_(size_t capacity);
static int stack_resize_(Stack* stk, size_t new_capacity);

static void* recalloc(void* ptr, size_t* nobj, size_t new_nobj, size_t size);

#define BUF_ (stk->buffer)
#define SZ_ (stk->size)
#define CAP_ (stk->capacity)

#ifdef DUMP
    #ifdef DUMP_ALL
        #define DO_DUMP dump_(stk, (Stack_err) err, Stack_dump_lvl::BRIEF, __func__, func, file, line)
    #else
        #define DO_DUMP dump_(stk, (Stack_err) err, Stack_dump_lvl::ONLYERR, __func__, func, file, line)
    #endif // DUMP_ALL
#else
    #define DO_DUMP 
#endif // DUMP

#define ASSERT(condition, error)        \
    do                                  \
    {                                   \
        if(!(condition))                \
        {                               \
            err |= error;               \
            DO_DUMP;                    \
            return (Stack_err) err;     \
        }                               \
    } while(0)                          \

////////////////////////////////////////////////////////////////
#ifdef PROTECT 
#ifdef STACK_HASH
    #define STK_HASH_ (stk->stk_hash)
    static void stack_set_stkhash_(Stack* stk);
    static int stack_check_stkhash_(const Stack* stk);
#endif // STACK_HASH

#ifdef BUFFER_HASH
    #define BUF_HASH_ (stk->buf_hash)
    static void stack_set_bufhash_(Stack* stk);
    static int stack_check_bufhash_(const Stack* stk);
#endif // BUFFER_HASH

#ifdef CANARY
    #define BEG_STK_CAN_ (stk->beg_can)
    #define END_STK_CAN_ (stk->end_can)
    #define BEG_BUF_CAN_ (*(((guard_t*) BUF_) - 1))
    #define END_BUF_CAN_ (*((guard_t*) (BUF_ + CAP_)))
    static void stack_set_cans_(Stack* stk);
    static int stack_check_cans_(const Stack* stk);
#endif // CANARY

Stack_err stack_verify_(const Stack* const stk)
{
    int err = Stack_err::NOERR;
    
    if(!stk)
        return Stack_err::NULLPTR;

    if(SZ_ > CAP_)
        return Stack_err::SZ_OVR_CAP;
    
    if(BUF_ == BUF_POISON)
        return Stack_err::DSTRCTED;

    if(!BUF_ && (SZ_ || CAP_))
        return Stack_err::BAD_BUF;

#ifdef CANARY
    err |= stack_check_cans_(stk);
#endif
#ifdef STACK_HASH
    err |= stack_check_stkhash_(stk);
#endif
#ifdef BUFFER_HASH
    err |= stack_check_bufhash_(stk);
#endif

    return (Stack_err) err;
}
#endif // PROTECT ////////////////////////////////////////////////

Stack_err stack_init_(Stack* stk, ssize_t preset_cap 
              DUMP_ON(const char func[], const char file[], int line))
{
    int err = Stack_err::NOERR;

#ifdef PROTECT
    ASSERT(stk, Stack_err::NULLPTR);

    ASSERT(BUF_ != BUF_POISON, Stack_err::DSTRCTED);

    ASSERT(!BUF_, Stack_err::REINIT);

#ifdef HASH
    ASSERT(STK_HASH_, Stack_err::REINIT);
#endif
#endif // PROTECT

    if(preset_cap)
    {
        if(preset_cap < 0)
            preset_cap = 0;
        
        size_t capacity = stack_init_cap_(preset_cap);

        ASSERT(stack_resize_(stk, capacity) == 0, Stack_err::BAD_ALLOC);        
    }

#ifdef PROTECT
#ifdef DUMP
    stk->init_func = func;
    stk->init_file = file;
    stk->init_line = line;
#endif // DUMP

#ifdef STACK_HASH
    stack_set_stkhash_(stk);
#endif
#ifdef BUFFER_HASH
    stack_set_bufhash_(stk);
#endif
#ifdef CANARY
    stack_set_cans_(stk);
#endif

    err |= stack_verify_(stk);
    DO_DUMP;
#endif // PROTECT

    return (Stack_err) err;
}

Stack_err stack_push_(Stack* stk, Elem_t elem
              DUMP_ON(const char func[], const char file[], int line))
{
    int err = Stack_err::NOERR;

#ifdef PROTECT
    err = stack_verify_(stk);
    ASSERT(!err, err);
#endif // PROTECT

    if(CAP_ == SZ_)
        ASSERT(stack_resize_(stk, CAP_ * STACK_CAP_MULTPLR) == 0, Stack_err::BAD_ALLOC);

    BUF_[SZ_++] = elem;

#ifdef PROTECT
#ifdef STACK_HASH
    stack_set_stkhash_(stk);
#endif 
#ifdef BUFFER_HASH
    stack_set_bufhash_(stk);
#endif

    err = stack_verify_(stk);
    DO_DUMP;
#endif // PROTECT

    return (Stack_err) err;
}

Stack_err stack_pop_(Stack* stk, Elem_t* elem
             DUMP_ON(const char func[], const char file[], int line))
{
    int err = Stack_err::NOERR;

#ifdef PROTECT
    err = stack_verify_(stk);
    ASSERT(!err, err);

    ASSERT(elem, Stack_err::NULLPTR);
#endif // PROTECT
    
    ASSERT(SZ_, Stack_err::POP_EMPT_STK);

    *elem = BUF_[--SZ_];

#ifdef PROTECT
    memset(&BUF_[SZ_], BYTE_POISON, sizeof(Elem_t));

    if(SZ_ * STACK_CAP_MULTPLR * STACK_CAP_MULTPLR <= CAP_)
        ASSERT(stack_resize_(stk, CAP_ / STACK_CAP_MULTPLR) == 0, Stack_err::BAD_ALLOC);

#ifdef BUFFER_HASH
    stack_set_bufhash_(stk);
#endif
#ifdef STACK_HASH
    stack_set_stkhash_(stk);
#endif

    err = stack_verify_(stk);
    DO_DUMP;
#endif // PROTECT

    return (Stack_err) err;
}

Stack_err stack_dstr_(Stack* stk
              DUMP_ON(const char func[], const char file[], int line))
{
#ifdef PROTECT
    int err = stack_verify_(stk);
    
    ASSERT(BUF_ != BUF_POISON, Stack_err::DSTRCTED);

    CAP_ = SIZE_POISON;
    SZ_  = SIZE_POISON;

#ifdef STACK_HASH
    STK_HASH_ = SIZE_POISON;
#endif
#ifdef BUFFER_HASH
    BUF_HASH_ = SIZE_POISON;
#endif

#ifdef CANARY
    BEG_STK_CAN_ = (guard_t) SIZE_POISON;
    END_STK_CAN_ = (guard_t) SIZE_POISON;
#endif

#ifdef DUMP
    stk->init_file = nullptr;
    stk->init_func = nullptr;
    stk->init_line = -1;
#endif // DUMP

    if(BUF_)
    {
#ifdef CANARY
        BUF_ = (Elem_t*) &BEG_BUF_CAN_;
#endif

        free(BUF_);
    }

    memcpy(&BUF_, &BUF_POISON, sizeof(Elem_t*));

    DO_DUMP;
    return Stack_err::NOERR;
#else /////////////////////
    free(BUF_);

    return Stack_err::NOERR;
#endif // PROTECT ///////////
}

static int stack_resize_(Stack* stk, size_t new_capacity)
{
    assert(stk);

    if(new_capacity < STACK_MIN_CAP)
        new_capacity = STACK_MIN_CAP;
    if(new_capacity == CAP_)
        return 0;

#ifdef CANARY
    void* temp_buffer = nullptr;
    size_t byte_cap = 0;
    size_t byte_new_cap = new_capacity * sizeof(Elem_t) + 2 * sizeof(guard_t);

    if(BUF_)
    {
        temp_buffer = ((char*) BUF_) - sizeof(guard_t);
        byte_cap = CAP_ * sizeof(Elem_t) + 2 * sizeof(guard_t);
    }

    temp_buffer = recalloc(temp_buffer, &byte_cap, byte_new_cap, 1);

    if(temp_buffer == nullptr)
        return -1;

    temp_buffer = (void*) ((char*) temp_buffer + sizeof(guard_t));
    byte_cap = (byte_cap - 2 * sizeof(guard_t)) / (sizeof(Elem_t));
    CAP_ = byte_cap;
    BUF_ = (Elem_t*) temp_buffer;

    stack_set_cans_(stk);
#else /////////////////////
    Elem_t* temp_buffer = (Elem_t*) recalloc(BUF_, &CAP_, new_capacity, sizeof(Elem_t));

    if(temp_buffer == nullptr)
        return -1;

    BUF_ = temp_buffer;
#endif // CANARY //////////

    return 0;
}

////////////////////////////////////////////////////////////////
#ifdef PROTECT
#ifdef STACK_HASH
static void stack_set_stkhash_(Stack* stk)
{
    assert(stk);
    
    STK_HASH_ = qhashfnv1_64(((char*) stk) + 2 * sizeof(guard_t), sizeof(Stack) - 4 * sizeof(guard_t));
}

static int stack_check_stkhash_(const Stack* stk)
{
    assert(stk);

    if(STK_HASH_ != qhashfnv1_64(((const char*) stk) + 2 * sizeof(guard_t), sizeof(Stack) - 4 * sizeof(guard_t)))
        return Stack_err::BAD_STK_HSH;

    return Stack_err::NOERR;
}
#endif // STACK_HASH

#ifdef BUFFER_HASH
static void stack_set_bufhash_(Stack* stk)
{
    assert(stk);
    BUF_HASH_ = qhashfnv1_64(BUF_, CAP_ * sizeof(Elem_t));
}

static int stack_check_bufhash_(const Stack* stk)
{
    assert(stk);

    if(BUF_)
        if(BUF_HASH_ != qhashfnv1_64(BUF_, CAP_ * sizeof(Elem_t)))
            return Stack_err::BAD_BUF_HSH;

    return Stack_err::NOERR;
}
#endif // BUFFER_HASH

#ifdef CANARY
static void stack_set_cans_(Stack* stk)
{
    assert(stk);

    BEG_STK_CAN_ = DEFAULT_CANARY;
    END_STK_CAN_ = DEFAULT_CANARY;

    if(BUF_)
    {
        BEG_BUF_CAN_ = DEFAULT_CANARY;
        END_BUF_CAN_ = DEFAULT_CANARY;
    }
}

static int stack_check_cans_(const Stack* stk)
{
    assert(stk);

    int err = 0;

    if(BEG_STK_CAN_ != DEFAULT_CANARY || END_STK_CAN_ != DEFAULT_CANARY)
        err |= Stack_err::BAD_STK_CAN;

    if(BUF_)
        if(BEG_BUF_CAN_ != DEFAULT_CANARY || END_BUF_CAN_ != DEFAULT_CANARY)
            err |= Stack_err::BAD_BUF_CAN;

    return err;
}
#endif // CANARY
#endif // PROTECT ////////////////////////////////////////////////


static size_t stack_init_cap_(size_t capacity)
{
    if(capacity < STACK_MIN_CAP)
        return STACK_MIN_CAP;

    size_t iter = STACK_MIN_CAP;
    while(iter < capacity)
        iter *= STACK_CAP_MULTPLR;

    return iter;
}

static void* recalloc(void* ptr, size_t* nobj, size_t new_nobj, size_t size)
{
    assert(nobj);
    
    if(new_nobj == 0 || size == 0)
    {
        free(ptr);
        return nullptr;
    }

    char* new_ptr = (char*) realloc(ptr, new_nobj * size);
    if(new_ptr == nullptr)
        return nullptr;
    if(new_nobj > *nobj)
        memset(new_ptr + (*nobj) * size, BYTE_POISON, (new_nobj - (*nobj)) * size);

    *nobj = new_nobj;

    return new_ptr;
}
