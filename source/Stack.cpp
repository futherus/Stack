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
#define PRESET_SZ_ (stk->preset_size)
#define CAP_ (stk->capacity)

#ifdef DEBUG
    static void stack_sethash(Stack* stk);
    static int stack_checkhash(const Stack* stk);
    static void stack_setcans(Stack* stk);
    static int stack_checkcans(const Stack* stk);
    static guard_t calculate_hash(const void* obj, size_t size);

    #define STK_HASH_ (stk->stk_hash)
    #define BUF_HASH_ (stk->buf_hash)
    #define BEG_STK_CAN_ (stk->beg_can)
    #define END_STK_CAN_ (stk->end_can)
    #define BEG_BUF_CAN_ (*(((guard_t*) BUF_) - 1))
    #define END_BUF_CAN_ (*((guard_t*) (BUF_ + CAP_)))
#endif // DEBUG

ErrType stack_verify_(Stack* stk)
{
    int err = 0;

    if(!stk)
        return ErrType::NULLPTR;

    if(SZ_ > CAP_)
        err |= ErrType::SZ_OVR_CAP;

#ifdef DEBUG
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

    err |= stack_checkcans(stk);

    err |= stack_checkhash(stk);
#endif //DEBUG

    return (ErrType) err;
}

#ifdef DEBUG
ErrType stack_init_(Stack* stk, size_t size, const char func[], const char file[], int line)
{
    if(!stk)
        return ErrType::NULLPTR;

    if(BUF_ == BUF_POISON)
        return ErrType::DSTRCTED;

    if(BUF_ || STK_HASH_)
        return ErrType::REINIT;

    if(size)
    {
        size = stack_init_cap(size);

        if(stack_resize(stk, size) != 0)
            return ErrType::BAD_ALLOC;

        PRESET_SZ_ = size;
    }

    stk->init_func = func;
    stk->init_file = file;
    stk->init_line = line;

    stack_sethash(stk);

    stack_setcans(stk);

    return stack_verify_(stk);
}
#else
ErrType stack_init_(Stack* stk, size_t size)
{
    if(!stk)
        return ErrType::NULLPTR;

    if(size)
    {
        size = stack_init_cap(size);

        if(stack_resize(stk, size) != 0)
            return ErrType::BAD_ALLOC;

        PRESET_SZ_ = size;
    }

    return stack_verify_(stk);
}
#endif // DEBUG

ErrType stack_push_(Stack* stk, Elem_t elem)
{
    ErrType err = stack_verify_(stk);
    if(err)
        return err;

    if(CAP_ == SZ_)
        if(stack_resize(stk, CAP_ * STACK_CAP_MULTPLR) != 0)
            return ErrType::BAD_ALLOC;

    BUF_[SZ_++] = elem;

#ifdef DEBUG
    stack_sethash(stk);
#endif // DEBUG

    return stack_verify_(stk);
}

ErrType stack_pop_(Stack* stk, Elem_t* elem)
{
    ErrType err = stack_verify_(stk);
    if(err)
        return err;

#ifdef DEBUG
    if(!elem)
        return ErrType::NULLPTR;
#endif // DEBUG

    if(SZ_ == 0)
        return ErrType::POP_EMPT_STCK;

    *elem = BUF_[--SZ_];

#ifdef DEBUG
    memset(&BUF_[SZ_], BYTE_POISON, sizeof(Elem_t));
#endif // DEBUG

    if(SZ_ * STACK_CAP_MULTPLR * STACK_CAP_MULTPLR <= CAP_)
        if(stack_resize(stk, CAP_ / STACK_CAP_MULTPLR) != 0)
            return ErrType::BAD_ALLOC;

#ifdef DEBUG
    stack_sethash(stk);

#endif // DEBUG

    return stack_verify_(stk);
}

ErrType stack_dstr_(Stack* stk)
{
    if(!stk)
        return ErrType::NULLPTR;

    if(!BUF_)
        return ErrType::BAD_BUF;

#ifdef DEBUG
    if(BUF_ == BUF_POISON)
        return ErrType::REDESTR;

    CAP_ = SIZE_POISON;
    SZ_  = SIZE_POISON;
    STK_HASH_ = SIZE_POISON;
    BUF_HASH_ = SIZE_POISON;
    BEG_STK_CAN_  = (guard_t) SIZE_POISON;
    END_STK_CAN_  = (guard_t) SIZE_POISON;

    BUF_ = (Elem_t*) &BEG_BUF_CAN_;
#endif //DEBUG

    free(BUF_);

#ifdef DEBUG
    memcpy(&BUF_, &BUF_POISON, sizeof(Elem_t*));

    stk->init_file = nullptr;
    stk->init_func = nullptr;
    stk->init_line = -1;
#endif //DEBUG

    return ErrType::NOERR;
}

static size_t stack_init_cap(size_t capacity)
{
    if(capacity < STACK_MIN_CAP)
        return STACK_MIN_CAP;

    size_t iter = STACK_MIN_CAP;
    while(iter < capacity)
        iter *= STACK_CAP_MULTPLR;

    return iter;
}

static int stack_resize(Stack* stk, size_t new_capacity)
{
    assert(stk);

    if(new_capacity < STACK_MIN_CAP)
        new_capacity = STACK_MIN_CAP;
    if(new_capacity < PRESET_SZ_)
        new_capacity = PRESET_SZ_;
    if(new_capacity == CAP_)
        return 0;

#ifdef DEBUG
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

    stack_setcans(stk);
#else
    Elem_t* temp_buffer = (Elem_t*) recalloc(BUF_, &CAP_, new_capacity, sizeof(Elem_t));

    if(temp_buffer == nullptr)
        return -1;

    BUF_ = temp_buffer;
#endif // DEBUG

    return 0;
}

#ifdef DEBUG
static void stack_sethash(Stack* stk)
{
    BUF_HASH_ = calculate_hash(BUF_, CAP_ * sizeof(Elem_t));
    STK_HASH_ = calculate_hash(((char*) stk) + 2 * sizeof(guard_t), sizeof(Stack) - 4 * sizeof(guard_t));
}

static int stack_checkhash(const Stack* stk)
{
    assert(stk);

    int err = 0;

    if(STK_HASH_ != calculate_hash(((const char*) stk) + 2 * sizeof(guard_t), sizeof(Stack) - 4 * sizeof(guard_t)))
        err |= ErrType::BAD_STK_HSH;

    if(BUF_)
        if(BUF_HASH_ != calculate_hash(BUF_, CAP_ * sizeof(Elem_t)))
            err |= ErrType::BAD_BUF_HSH;

    return err;
}

static void stack_setcans(Stack* stk)
{
    assert(stk);

    BEG_STK_CAN_ = CANARY;
    END_STK_CAN_ = CANARY;

    if(BUF_)
    {
        BEG_BUF_CAN_ = CANARY;
        END_BUF_CAN_ = CANARY;
    }
}

static int stack_checkcans(const Stack* stk)
{
    assert(stk);

    int err = 0;

    if(BEG_STK_CAN_ != CANARY || END_STK_CAN_ != CANARY)
        err |= ErrType::BAD_STK_CAN;

    if(BUF_)
        if(BEG_BUF_CAN_ != CANARY || END_BUF_CAN_ != CANARY)
            err |= ErrType::BAD_BUF_CAN;

    return err;
}
#undef BUF_HASH_
#undef STK_HASH_
#undef BEG_STK_CAN_
#undef END_STK_CAN_
#undef BEG_BUF_CAN_
#undef END_BUF_CAN_

#endif // DEBUG

#undef BUF_
#undef SZ_
#undef CAP_

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
#endif //DEBUG
