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
    #include "include/dump.h"
    #define DO_DUMP dump(stk, (ErrType) err, 0, __func__, func, file, line)
#else // DUMP
    #define DO_DUMP ;
#endif // DUMP

#ifdef HASH
    static guard_t calculate_hash(const void* obj, size_t size);
    static void stack_sethash(Stack* stk);
    static int stack_checkhash(const Stack* stk);
    #define STK_HASH_ (stk->stk_hash)
    #ifdef BUFFER_HASH
        #define BUF_HASH_ (stk->buf_hash)
    #endif // BUFFER_HASH
#endif // HASH

#ifdef CANARY
    #define BEG_STK_CAN_ (stk->beg_can)
    #define END_STK_CAN_ (stk->end_can)
    #define BEG_BUF_CAN_ (*(((guard_t*) BUF_) - 1))
    #define END_BUF_CAN_ (*((guard_t*) (BUF_ + CAP_)))
    static void stack_setcans(Stack* stk);
    static int stack_checkcans(const Stack* stk);
#endif // CANARY

ErrType stack_verify_(const Stack* const stk)
{
    int err = 0;

    if(!stk)
    {
        err |= ErrType::NULLPTR;
        return ErrType (err);
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

#ifdef CANARY
    err |= stack_checkcans(stk);
#endif // CANARY
#ifdef HASH
    err |= stack_checkhash(stk);
#endif // HASH

    return (ErrType) err;
}

ErrType stack_init_(Stack* stk, size_t preset_cap, const char func[], const char file[], int line)
{
    int err = 0;

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

#ifdef DUMP
    stk->init_func = func;
    stk->init_file = file;
    stk->init_line = line;
#endif // DUMP

#ifdef HASH
    stack_sethash(stk);
#endif // HASH

#ifdef CANARY
    stack_setcans(stk);
#endif // CANARY

    err |= stack_verify_(stk);
    DO_DUMP;
    
    return (ErrType) err;
}

ErrType stack_push_(Stack* stk, Elem_t elem, const char func[], const char file[], int line)
{
    ErrType err = stack_verify_(stk);
    if(err)
    {
        DO_DUMP;
        return err;
    }

    if(CAP_ == SZ_)
        if(stack_resize(stk, CAP_ * STACK_CAP_MULTPLR) != 0)
        {
            err = ErrType::BAD_ALLOC;
            DO_DUMP;
            return err;
        }

    BUF_[SZ_++] = elem;

#ifdef HASH
    stack_sethash(stk);
#endif // HASH

    err = stack_verify_(stk);
    DO_DUMP;
    return err;
}

ErrType stack_pop_(Stack* stk, Elem_t* elem, const char func[], const char file[], int line)
{
    ErrType err = stack_verify_(stk);
    if(err)
    {
        DO_DUMP;
        return err;
    }

    if(!elem)
    {
        DO_DUMP;
        return ErrType::NULLPTR;
    }

    if(SZ_ == 0)
    {
        DO_DUMP;
        return ErrType::POP_EMPT_STCK;
    }

    *elem = BUF_[--SZ_];

    memset(&BUF_[SZ_], BYTE_POISON, sizeof(Elem_t));

    if(SZ_ * STACK_CAP_MULTPLR * STACK_CAP_MULTPLR <= CAP_)
        if(stack_resize(stk, CAP_ / STACK_CAP_MULTPLR) != 0)
        {
            err = ErrType::BAD_ALLOC;
            DO_DUMP;
            return err;
        }

#ifdef HASH
    stack_sethash(stk);
#endif // HASH

    err = stack_verify_(stk);
    DO_DUMP;
    return err;
}

ErrType stack_dstr_(Stack* stk, const char func[], const char file[], int line)
{
    ErrType err = stack_verify_(stk);
    
    if(BUF_ == BUF_POISON)
    {
        err = ErrType::REDESTR;
        DO_DUMP;
        return err;
    }

    CAP_ = SIZE_POISON;
    SZ_  = SIZE_POISON;
    
#ifdef HASH
    STK_HASH_ = SIZE_POISON;
#ifdef BUFFER_HASH
    BUF_HASH_ = SIZE_POISON;
#endif // BUFFER_HASH
#endif // HASH

#ifdef CANARY
    BEG_STK_CAN_  = (guard_t) SIZE_POISON;
    END_STK_CAN_  = (guard_t) SIZE_POISON;
#endif // CANARY

#ifdef DUMP
    stk->init_file = nullptr;
    stk->init_func = nullptr;
    stk->init_line = -1;
#endif // DUMP

    if(BUF_)
    {
#ifdef CANARY
        BUF_ = (Elem_t*) &BEG_BUF_CAN_;
#endif // CANARY

        free(BUF_);
    }

    memcpy(&BUF_, &BUF_POISON, sizeof(Elem_t*));

    DO_DUMP;
    return ErrType::NOERR;
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

    stack_setcans(stk);
#else // CANARY
    Elem_t* temp_buffer = (Elem_t*) recalloc(BUF_, &CAP_, new_capacity, sizeof(Elem_t));

    if(temp_buffer == nullptr)
        return -1;

    BUF_ = temp_buffer;
#endif // CANARY

    return 0;
}

#ifdef HASH
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

static void stack_sethash(Stack* stk)
{
#ifdef BUFFER_HASH
    BUF_HASH_ = calculate_hash(BUF_, CAP_ * sizeof(Elem_t));
#endif // BUFFER_HASH

    STK_HASH_ = calculate_hash(((char*) stk) + 2 * sizeof(guard_t), sizeof(Stack) - 4 * sizeof(guard_t));
}

static int stack_checkhash(const Stack* stk)
{
    assert(stk);

    int err = 0;

    if(STK_HASH_ != calculate_hash(((const char*) stk) + 2 * sizeof(guard_t), sizeof(Stack) - 4 * sizeof(guard_t)))
        err |= ErrType::BAD_STK_HSH;

#ifdef BUFFER_HASH
    if(BUF_)
        if(BUF_HASH_ != calculate_hash(BUF_, CAP_ * sizeof(Elem_t)))
            err |= ErrType::BAD_BUF_HSH;
#endif // BUFFER_HASH

    return err;
}
#endif // HASH

#ifdef CANARY
static void stack_setcans(Stack* stk)
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

static int stack_checkcans(const Stack* stk)
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

#undef BUF_HASH_
#undef STK_HASH_
#undef BEG_STK_CAN_
#undef END_STK_CAN_
#undef BEG_BUF_CAN_
#undef END_BUF_CAN_

#else // DEBUG /////////////////////////////////////////////////////////////////

ErrType stack_init_(Stack* stk, size_t size)
{
    if(size)
    {
        size = stack_init_cap(size);

        if(stack_resize(stk, size) != 0)
            return ErrType::BAD_ALLOC;

        PRESET_CAP_ = size;
    }

    return ErrType::NOERR;
}

ErrType stack_push_(Stack* stk, Elem_t elem)
{
    if(CAP_ == SZ_)
        if(stack_resize(stk, CAP_ * STACK_CAP_MULTPLR) != 0)
            return ErrType::BAD_ALLOC;

    BUF_[SZ_++] = elem;

    return ErrType::NOERR;
}

ErrType stack_pop_(Stack* stk, Elem_t* elem)
{
    if(SZ_ == 0)
        return ErrType::POP_EMPT_STCK;

    *elem = BUF_[--SZ_];

    if(SZ_ * STACK_CAP_MULTPLR * STACK_CAP_MULTPLR <= CAP_)
        if(stack_resize(stk, CAP_ / STACK_CAP_MULTPLR) != 0)
            return ErrType::BAD_ALLOC;

    return ErrType::NOERR;
}

ErrType stack_dstr_(Stack* stk)
{
    free(BUF_);
    return ErrType::NOERR;
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

    Elem_t* temp_buffer = (Elem_t*) recalloc(BUF_, &CAP_, new_capacity, sizeof(Elem_t));

    if(temp_buffer == nullptr)
        return -1;

    BUF_ = temp_buffer;

    return 0;
}

#endif // DEBUG ////////////////////////////////////////////////////////////////

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

#undef BUF_
#undef SZ_
#undef CAP_
#undef PRESET_CAP_
