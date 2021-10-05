#include "include/Stack.h"

#ifdef DEBUG

#include <string.h>
#include <assert.h>
#include <stdio.h>

static char MESSAGE[8192] = "";
static ErrType STACK_ERRNO = ErrType::NOERR;

static void set_message(ErrType err)
{
    if(err == ErrType::NOERR)
        return;

    MESSAGE[0] = '\0';
    strcat(MESSAGE, "ERROR\n");

    if(err & BAD_ALLOC)
        strcat(MESSAGE, BAD_ALLOCATION);
    if(err & BAD_BUF)
        strcat(MESSAGE, BAD_BUFFER);
    if(err & BAD_STK_HSH)
        strcat(MESSAGE, BAD_STACK_HASH);
    if(err & BAD_BUF_HSH)
        strcat(MESSAGE, BAD_BUFFER_HASH);
    if(err & BAD_STK_CAN)
        strcat(MESSAGE, BAD_STACK_CANARY);
    if(err & BAD_BUF_CAN)
        strcat(MESSAGE, BAD_BUFFER_CANARY);
    if(err & REINIT)
        strcat(MESSAGE, REINITIALIZING);
    if(err & REDESTR)
        strcat(MESSAGE, REDESTRUCTING);
    if(err & DSTRCTED)
        strcat(MESSAGE, DESTRUCTED);
    if(err & SZ_OVR_CAP)
        strcat(MESSAGE, SIZE_OVER_CAP);
    if(err & CAP_OVR_SZ)
        strcat(MESSAGE, CAP_OVER_SIZE);
    if(err & POP_EMPT_STCK)
        strcat(MESSAGE, POP_EMPTY_STACK);
    if(err & NULLPTR)
        strcat(MESSAGE, NULLPOINTER);
    if(err & UNEXPCTD_ERR)
        strcat(MESSAGE, UNEXPECTED_ERROR);
}

#define BUF_ (stk->buffer)
#define SZ_ (stk->size)
#define PRESET_SZ_ (stk->preset_size)
#define CAP_ (stk->capacity)

#define STK_HASH_ (stk->stk_hash)
#define BUF_HASH_ (stk->buf_hash)
#define BEG_STK_CAN_ (stk->beg_can)
#define END_STK_CAN_ (stk->end_can)
#define BEG_BUF_CAN_ (*(((guard_t*) BUF_) - 1))
#define END_BUF_CAN_ (*((guard_t*) (BUF_ + CAP_)))

void stack_dump_(const Stack* const stk, const char message[],
                 const char func[], const char file[], int line) //void (*print_elem)(const void*), FILE* errstream)
{
    static int first_open = 1;
    FILE* logstream = stderr;
    if(logfile)
    {
        if(first_open)
        {
            logstream = fopen(logfile, "w");
            first_open = 0;
        }
        else
            logstream = fopen(logfile, "a");

        if(!logstream)
        {
            logstream = stderr;
            fprintf(stderr, "Can't open log file\n");
        }
    }

    fprintf(logstream, "\n");

    if(message)
        fprintf(logstream, "%s\n", message);

    fprintf(logstream, "Stack [%p] ", stk);
    if(STACK_ERRNO)
    {
        fprintf(logstream, "ERROR (code %.4x)\n", STACK_ERRNO);
        STACK_ERRNO = NOERR;
    }
    else
        fprintf(logstream, "\n");
    //    fprintf(logstream, "ok\n");

    fprintf(logstream, "   called from: %s at %s (%d)\n", func, file, line); //get from define

    if(!stk)
        fprintf(logstream, "   pointer to stack is nullptr\n");
    else
    {
        if(stk->init_func && stk->init_file && stk->init_line)
            fprintf(logstream, "   initialized: %s at %s (%d)\n\n", stk->init_func, stk->init_file, stk->init_line); //stored in stk
        else
            fprintf(logstream, "   initialized: UNKNOWN\n\n");

        fprintf(logstream, "   buffer[%p]\n", BUF_);
        fprintf(logstream, "   size     = %Iu\n", SZ_);
        fprintf(logstream, "   capacity = %Iu\n\n", CAP_);
        fprintf(logstream, "   Guards:\n");
        fprintf(logstream, "     stack  begin = %p\n", BEG_STK_CAN_);
        fprintf(logstream, "     stack  end   = %p\n", END_STK_CAN_);
        if(BUF_ != BUF_POISON && BUF_)
        {
            fprintf(logstream, "     buffer begin = %p\n", BEG_BUF_CAN_);
            fprintf(logstream, "     buffer end   = %p\n", END_BUF_CAN_);
        }
        fprintf(logstream, "     stack  hash  = %p\n", STK_HASH_);
        fprintf(logstream, "     buffer hash  = %p\n\n", BUF_HASH_);

        if(BUF_ && BUF_ != BUF_POISON)
        {
            fprintf(logstream, "   {\n");

            size_t iter = 0;

            while(iter < CAP_ || iter < SZ_)
            {
                if(iter < SZ_)
                    fprintf(logstream, "   #");
                else
                    fprintf(logstream, "    ");

                fprintf(logstream, "%7.1Iu: " ELEM_FORMAT "\n", iter, stk->buffer[iter]);

                iter++;
            }

            fprintf(logstream, "   }\n");
        }
    }
}
#undef BUF_HASH_
#undef STK_HASH_
#undef BEG_STK_CAN_
#undef END_STK_CAN_
#undef BEG_BUF_CAN_
#undef END_BUF_CAN_
#undef BUF_
#undef SZ_
#undef PRESET_SZ_
#undef CAP_

void dump_exit_(const Stack* const stk, ErrType err,
                const char func[], const char file[], int line)
{
    assert(func && file && line);

    STACK_ERRNO = err;
    set_message(err);

    stack_dump_(stk, MESSAGE, func, file, line);
#ifdef EXIT
    exit(err);
#endif // EXIT
}

#endif // DEBUG
