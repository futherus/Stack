#include "include/Stack.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

static size_t stack_init_cap(size_t capacity);
static int stack_resize(Stack* stk, size_t new_capacity);

static void* recalloc(void* ptr, size_t* nobj, size_t new_nobj, size_t size);

#define BUF_ (stk->buffer)
#define SZ_ (stk->size)
#define PRESET_CAP_ (stk->preset_cap)
#define CAP_ (stk->capacity)

#ifdef DEBUG
#ifdef DUMP
    #ifdef DUMP_ALL
        #define DO_DUMP dump(stk, (ErrType) err, DumpLevel::BRIEF, __func__, func, file, line)
    #else
        #define DO_DUMP dump(stk, (ErrType) err, DumpLevel::ONLYERR, __func__, func, file, line)
    #endif // DUMP_ALL
#else
    #define DO_DUMP 
#endif // DUMP

    static guard_t calculate_hash(const void* obj, size_t size);

#ifdef STACK_HASH
    #define STK_HASH_ (stk->stk_hash)
    static void stack_set_stkhash(Stack* stk);
    static int stack_check_stkhash(const Stack* stk);
#endif // STACK_HASH

#ifdef BUFFER_HASH
    #define BUF_HASH_ (stk->buf_hash)
    static void stack_set_bufhash(Stack* stk);
    static int stack_check_bufhash(const Stack* stk);
#endif // BUFFER_HASH

#ifdef CANARY
    #define BEG_STK_CAN_ (stk->beg_can)
    #define END_STK_CAN_ (stk->end_can)
    #define BEG_BUF_CAN_ (*(((guard_t*) BUF_) - 1))
    #define END_BUF_CAN_ (*((guard_t*) (BUF_ + CAP_)))
    static void stack_set_cans(Stack* stk);
    static int stack_check_cans(const Stack* stk);
#endif // CANARY

ErrType stack_verify_(const Stack* const stk)
{
    int err = 0;

    if(!stk)
    {
        err |= ErrType::NULLPTR;
        return (ErrType) err;
    }

    if(SZ_ > CAP_)
        err |= ErrType::SZ_OVR_CAP;
    
    if(BUF_ == BUF_POISON)
    {
        err |= ErrType::DSTRCTED;
        return (ErrType) err;
    }

    if(!BUF_ && (SZ_ || CAP_))
    {
        err |= ErrType::BAD_BUF;
        return (ErrType) err;
    }

    enable_canary(err |= stack_check_cans(stk);)
    enable_stkhash(err |= stack_check_stkhash(stk);)
    enable_bufhash(err |= stack_check_bufhash(stk);)

    return (ErrType) err;
}
#endif // DEBUG

ErrType stack_init_(Stack* stk, size_t preset_cap 
        enable_dump(const char func[], const char file[], int line))
{
    int err = ErrType::NOERR;

#ifdef DEBUG
    if(!stk)
    {
        err |= ErrType::NULLPTR;
        DO_DUMP;
        return (ErrType) err;
    }

    if(BUF_ == BUF_POISON)
    {
        err |= ErrType::DSTRCTED;
        DO_DUMP;
        return (ErrType) err;
    }
    if(BUF_)
    {
        err |= ErrType::REINIT;
        DO_DUMP;
        return (ErrType) err;
    }

#ifdef HASH
    if(STK_HASH_)
    {
        err |= ErrType::REINIT;
        DO_DUMP;
        return (ErrType) err;
    }
#endif // HASH
#endif // DEBUG

    if(preset_cap)
    {
        preset_cap = stack_init_cap(preset_cap);

        if(stack_resize(stk, preset_cap) != 0)
        {
            err |= ErrType::BAD_ALLOC;
            return (ErrType) err;
        }

        PRESET_CAP_ = preset_cap;
    }

#ifdef DEBUG
#ifdef DUMP
    stk->init_func = func;
    stk->init_file = file;
    stk->init_line = line;
#endif // DUMP

    enable_stkhash(stack_set_stkhash(stk);)
    enable_bufhash(stack_set_bufhash(stk);)

    enable_canary(stack_set_cans(stk);)

    err |= stack_verify_(stk);
    DO_DUMP;
#endif // DEBUG

    return (ErrType) err;
}

ErrType stack_push_(Stack* stk, Elem_t elem
        enable_dump(const char func[], const char file[], int line))
{
    int err = ErrType::NOERR;

#ifdef DEBUG
    err = stack_verify_(stk);
    if(err)
    {
        DO_DUMP;
        return (ErrType) err;
    }
#endif // DEBUG

    if(CAP_ == SZ_)
        if(stack_resize(stk, CAP_ * STACK_CAP_MULTPLR) != 0)
        {
            err = ErrType::BAD_ALLOC;
            DO_DUMP;
            return (ErrType) err;
        }

    BUF_[SZ_++] = elem;

#ifdef DEBUG
    enable_stkhash(stack_set_stkhash(stk);)
    enable_bufhash(stack_set_bufhash(stk);)

    err = stack_verify_(stk);
    DO_DUMP;
#endif // DEBUG

    return (ErrType) err;
}

ErrType stack_pop_(Stack* stk, Elem_t* elem
        enable_dump(const char func[], const char file[], int line))
{
    int err = ErrType::NOERR;
#ifdef DEBUG
    err = stack_verify_(stk);
    if(err)
    {
        DO_DUMP;
        return (ErrType) err;
    }

    if(!elem)
    {
        err |= ErrType::NULLPTR;
        DO_DUMP;
        return (ErrType) err;
    }
#endif // DEBUG
    
    if(SZ_ == 0)
    {
        DO_DUMP;
        return ErrType::POP_EMPT_STK;
    }

    *elem = BUF_[--SZ_];

#ifdef DEBUG
    memset(&BUF_[SZ_], BYTE_POISON, sizeof(Elem_t));

    if(SZ_ * STACK_CAP_MULTPLR * STACK_CAP_MULTPLR <= CAP_)
        if(stack_resize(stk, CAP_ / STACK_CAP_MULTPLR) != 0)
        {
            err = ErrType::BAD_ALLOC;
            DO_DUMP;
            return (ErrType) err;
        }

    enable_bufhash(stack_set_bufhash(stk);)
    enable_stkhash(stack_set_stkhash(stk);)

    err = stack_verify_(stk);
    DO_DUMP;
#endif // DEBUG

    return (ErrType) err;
}

ErrType stack_dstr_(Stack* stk
        enable_dump(const char func[], const char file[], int line))
{
#ifdef DEBUG
    int err = stack_verify_(stk);
    
    if(BUF_ == BUF_POISON)
    {
        err = ErrType::REDESTR;
        DO_DUMP;
        return (ErrType) err;
    }

    CAP_ = SIZE_POISON;
    SZ_  = SIZE_POISON;
    PRESET_CAP_ = SIZE_POISON;
    
    enable_stkhash(STK_HASH_ = SIZE_POISON;)
    enable_bufhash(BUF_HASH_ = SIZE_POISON;)

    enable_canary(BEG_STK_CAN_ = (guard_t) SIZE_POISON;)
    enable_canary(END_STK_CAN_ = (guard_t) SIZE_POISON;)

#ifdef DUMP
    stk->init_file = nullptr;
    stk->init_func = nullptr;
    stk->init_line = -1;
#endif // DUMP

    if(BUF_)
    {
        enable_canary(BUF_ = (Elem_t*) &BEG_BUF_CAN_;)

        free(BUF_);
    }

    memcpy(&BUF_, &BUF_POISON, sizeof(Elem_t*));

    DO_DUMP;
    return ErrType::NOERR;
#else 
    free(BUF_);

    return ErrType::NOERR;
#endif // DEBUG
}

static int stack_resize(Stack* stk, size_t new_capacity)
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

    stack_set_cans(stk);
    
#else
    Elem_t* temp_buffer = (Elem_t*) recalloc(BUF_, &CAP_, new_capacity, sizeof(Elem_t));

    if(temp_buffer == nullptr)
        return -1;

    BUF_ = temp_buffer;
#endif // CANARY

    return 0;
}

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
static void stack_set_stkhash(Stack* stk)
{
    assert(stk);
    
    STK_HASH_ = calculate_hash(((char*) stk) + 2 * sizeof(guard_t), sizeof(Stack) - 4 * sizeof(guard_t));
}

static int stack_check_stkhash(const Stack* stk)
{
    assert(stk);

    if(STK_HASH_ != calculate_hash(((const char*) stk) + 2 * sizeof(guard_t), sizeof(Stack) - 4 * sizeof(guard_t)))
        return ErrType::BAD_STK_HSH;

    return ErrType::NOERR;
}
#endif // STACK_HASH

    
#ifdef BUFFER_HASH
static void stack_set_bufhash(Stack* stk)
{
    assert(stk);

    BUF_HASH_ = calculate_hash(BUF_, CAP_ * sizeof(Elem_t));
}

static int stack_check_bufhash(const Stack* stk)
{
    assert(stk);

    if(BUF_)
        if(BUF_HASH_ != calculate_hash(BUF_, CAP_ * sizeof(Elem_t)))
            return ErrType::BAD_BUF_HSH;

    return ErrType::NOERR;
}
#endif // BUFFER_HASH

#ifdef CANARY
static void stack_set_cans(Stack* stk)
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

static int stack_check_cans(const Stack* stk)
{
    assert(stk);

    int err = 0;

    if(BEG_STK_CAN_ != DEFAULT_CANARY || END_STK_CAN_ != DEFAULT_CANARY)
        err |= ErrType::BAD_STK_CAN;

    if(BUF_)
        if(BEG_BUF_CAN_ != DEFAULT_CANARY || END_BUF_CAN_ != DEFAULT_CANARY)
            err |= ErrType::BAD_BUF_CAN;

    return err;
}
#endif // CANARY
#endif // DEBUG

static size_t stack_init_cap(size_t capacity)
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

    char* new_ptr = (char*) realloc(ptr, new_nobj * size);
    if(new_ptr == nullptr)
        return nullptr;
    if(new_nobj > *nobj)
        memset(new_ptr + (*nobj) * size, BYTE_POISON, (new_nobj - (*nobj)) * size);

    *nobj = new_nobj;

    return new_ptr;
}

#ifdef BUF_HASH_
    #undef BUF_HASH_
#endif // BUF_HASH_
#ifdef STK_HASH_
    #undef STK_HASH_
#endif // STK_HASH_

#ifdef CANARY
    #undef BEG_STK_CAN_
    #undef END_STK_CAN_
    #undef BEG_BUF_CAN_
    #undef END_BUF_CAN_
#endif // CANARY