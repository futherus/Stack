#include "include/config.h"
#include "include/Stack.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

////////////////////////////////////////////////////////////////
#ifdef CANARY
    #define CANARYS_ON(arg) arg
#else
    #define CANARYS_ON(arg)
#endif

#ifdef STACK_HASH
    #define STKHASH_ON(arg) arg
#else
    #define STKHASH_ON(arg)
#endif

#ifdef BUFFER_HASH
    #define BUFHASH_ON(arg) arg
#else
    #define BUFHASH_ON(arg)
#endif
////////////////////////////////////////////////////////////////

static size_t stack_init_cap_(size_t capacity);
static int stack_resize_(Stack* stk, size_t new_capacity);

static void* recalloc(void* ptr, size_t* nobj, size_t new_nobj, size_t size);

#define BUF_ (stk->buffer)
#define SZ_ (stk->size)
#define PRESET_CAP_ (stk->preset_cap)
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

////////////////////////////////////////////////////////////////
#ifdef DEBUG 
    static guard_t calculate_hash(const void* obj, size_t size);

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
    int err = 0;

    if(!stk)
    {
        err |= Stack_err::NULLPTR;
        return (Stack_err) err;
    }

    if(SZ_ > CAP_)
        err |= Stack_err::SZ_OVR_CAP;
    
    if(BUF_ == BUF_POISON)
    {
        err |= Stack_err::DSTRCTED;
        return (Stack_err) err;
    }

    if(!BUF_ && (SZ_ || CAP_))
    {
        err |= Stack_err::BAD_BUF;
        return (Stack_err) err;
    }

    CANARYS_ON(err |= stack_check_cans_(stk);)
    STKHASH_ON(err |= stack_check_stkhash_(stk);)
    BUFHASH_ON(err |= stack_check_bufhash_(stk);)

    return (Stack_err) err;
}
#endif // DEBUG ////////////////////////////////////////////////

Stack_err stack_init_(Stack* stk, size_t preset_cap 
              DUMP_ON(const char func[], const char file[], int line))
{
    int err = Stack_err::NOERR;

#ifdef DEBUG
    if(!stk)
    {
        err |= Stack_err::NULLPTR;
        DO_DUMP;
        return (Stack_err) err;
    }

    if(BUF_ == BUF_POISON)
    {
        err |= Stack_err::DSTRCTED;
        DO_DUMP;
        return (Stack_err) err;
    }
    if(BUF_)
    {
        err |= Stack_err::REINIT;
        DO_DUMP;
        return (Stack_err) err;
    }

#ifdef HASH
    if(STK_HASH_)
    {
        err |= Stack_err::REINIT;
        DO_DUMP;
        return (Stack_err) err;
    }
#endif // HASH
#endif // DEBUG

    if(preset_cap)
    {
        preset_cap = stack_init_cap_(preset_cap);

        if(stack_resize_(stk, preset_cap) != 0)
        {
            err |= Stack_err::BAD_ALLOC;
            return (Stack_err) err;
        }

        PRESET_CAP_ = preset_cap;
    }

#ifdef DEBUG
#ifdef DUMP
    stk->init_func = func;
    stk->init_file = file;
    stk->init_line = line;
#endif // DUMP

    STKHASH_ON(stack_set_stkhash_(stk);)
    BUFHASH_ON(stack_set_bufhash_(stk);)

    CANARYS_ON(stack_set_cans_(stk);)

    err |= stack_verify_(stk);
    DO_DUMP;
#endif // DEBUG

    return (Stack_err) err;
}

Stack_err stack_push_(Stack* stk, Elem_t elem
              DUMP_ON(const char func[], const char file[], int line))
{
    int err = Stack_err::NOERR;

#ifdef DEBUG
    err = stack_verify_(stk);
    if(err)
    {
        DO_DUMP;
        return (Stack_err) err;
    }
#endif // DEBUG

    if(CAP_ == SZ_)
        if(stack_resize_(stk, CAP_ * STACK_CAP_MULTPLR) != 0)
        {
            err = Stack_err::BAD_ALLOC;
            DO_DUMP;
            return (Stack_err) err;
        }

    BUF_[SZ_++] = elem;

#ifdef DEBUG
    STKHASH_ON(stack_set_stkhash_(stk);)
    BUFHASH_ON(stack_set_bufhash_(stk);)

    err = stack_verify_(stk);
    DO_DUMP;
#endif // DEBUG

    return (Stack_err) err;
}

Stack_err stack_pop_(Stack* stk, Elem_t* elem
             DUMP_ON(const char func[], const char file[], int line))
{
    int err = Stack_err::NOERR;
#ifdef DEBUG
    err = stack_verify_(stk);
    if(err)
    {
        DO_DUMP;
        return (Stack_err) err;
    }

    if(!elem)
    {
        err |= Stack_err::NULLPTR;
        DO_DUMP;
        return (Stack_err) err;
    }
#endif // DEBUG
    
    if(SZ_ == 0)
    {
        DO_DUMP;
        return Stack_err::POP_EMPT_STK;
    }

    *elem = BUF_[--SZ_];

#ifdef DEBUG
    memset(&BUF_[SZ_], BYTE_POISON, sizeof(Elem_t));

    if(SZ_ * STACK_CAP_MULTPLR * STACK_CAP_MULTPLR <= CAP_)
        if(stack_resize_(stk, CAP_ / STACK_CAP_MULTPLR) != 0)
        {
            err = Stack_err::BAD_ALLOC;
            DO_DUMP;
            return (Stack_err) err;
        }

    BUFHASH_ON(stack_set_bufhash_(stk);)
    STKHASH_ON(stack_set_stkhash_(stk);)

    err = stack_verify_(stk);
    DO_DUMP;
#endif // DEBUG

    return (Stack_err) err;
}

Stack_err stack_dstr_(Stack* stk
              DUMP_ON(const char func[], const char file[], int line))
{
#ifdef DEBUG
    int err = stack_verify_(stk);
    
    if(BUF_ == BUF_POISON)
    {
        err = Stack_err::REDESTR;
        DO_DUMP;
        return (Stack_err) err;
    }

    CAP_ = SIZE_POISON;
    SZ_  = SIZE_POISON;
    PRESET_CAP_ = SIZE_POISON;
    
    STKHASH_ON(STK_HASH_ = SIZE_POISON;)
    BUFHASH_ON(BUF_HASH_ = SIZE_POISON;)

    CANARYS_ON(BEG_STK_CAN_ = (guard_t) SIZE_POISON;)
    CANARYS_ON(END_STK_CAN_ = (guard_t) SIZE_POISON;)

#ifdef DUMP
    stk->init_file = nullptr;
    stk->init_func = nullptr;
    stk->init_line = -1;
#endif // DUMP

    if(BUF_)
    {
        CANARYS_ON(BUF_ = (Elem_t*) &BEG_BUF_CAN_;)

        free(BUF_);
    }

    memcpy(&BUF_, &BUF_POISON, sizeof(Elem_t*));

    DO_DUMP;
    return Stack_err::NOERR;
#else 
    free(BUF_);

    return Stack_err::NOERR;
#endif // DEBUG
}

static int stack_resize_(Stack* stk, size_t new_capacity)
{
    assert(stk);

    if(new_capacity < STACK_MIN_CAP)
        new_capacity = STACK_MIN_CAP;
    if(new_capacity < PRESET_CAP_)
        new_capacity = PRESET_CAP_;
    if(new_capacity == CAP_)
        return 0;

#ifdef CANARY
    char* temp_buffer = nullptr;
    size_t byte_cap = 0;
    size_t byte_new_cap = new_capacity * sizeof(Elem_t) + 2 * sizeof(guard_t);

    if(BUF_)
    {
        temp_buffer = ((char*) BUF_) - sizeof(guard_t);
        byte_cap = CAP_ * sizeof(Elem_t) + 2 * sizeof(guard_t);
    }

    temp_buffer = (char*) recalloc(temp_buffer, &byte_cap, byte_new_cap, 1);

    if(temp_buffer == nullptr)
        return -1;

    temp_buffer += sizeof(guard_t);
    byte_cap = (byte_cap - 2 * sizeof(guard_t)) / (sizeof(Elem_t));
    CAP_ = byte_cap;
    BUF_ = (Elem_t*) temp_buffer;

    stack_set_cans_(stk);
#else
    Elem_t* temp_buffer = (Elem_t*) recalloc(BUF_, &CAP_, new_capacity, sizeof(Elem_t));

    if(temp_buffer == nullptr)
        return -1;

    BUF_ = temp_buffer;
#endif // CANARY

    return 0;
}

////////////////////////////////////////////////////////////////
#ifdef DEBUG
static guard_t calculate_hash(const void* obj, size_t size)
{
    guard_t hash = 1;

    if(size == 0)
        return hash;

    assert(obj);

    const char* ptr = (const char*) obj;

    for(size_t iter = 0; iter < size; iter++)
        hash += iter * ptr[iter];

    hash ^= (guard_t) obj;

    return hash;
}

#ifdef STACK_HASH
static void stack_set_stkhash_(Stack* stk)
{
    assert(stk);
    
    STK_HASH_ = calculate_hash(((char*) stk) + 2 * sizeof(guard_t), sizeof(Stack) - 4 * sizeof(guard_t));
}

static int stack_check_stkhash_(const Stack* stk)
{
    assert(stk);

    if(STK_HASH_ != calculate_hash(((const char*) stk) + 2 * sizeof(guard_t), sizeof(Stack) - 4 * sizeof(guard_t)))
        return Stack_err::BAD_STK_HSH;

    return Stack_err::NOERR;
}
#endif // STACK_HASH

    
#ifdef BUFFER_HASH
static void stack_set_bufhash_(Stack* stk)
{
    assert(stk);

    BUF_HASH_ = calculate_hash(BUF_, CAP_ * sizeof(Elem_t));
}

static int stack_check_bufhash_(const Stack* stk)
{
    assert(stk);

    if(BUF_)
        if(BUF_HASH_ != calculate_hash(BUF_, CAP_ * sizeof(Elem_t)))
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
#endif // DEBUG ////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////////
// NOT CHECKED FOR NEGATIVE CAPACITY
///////////////////////////////////////////////////////////////////////////////////
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
