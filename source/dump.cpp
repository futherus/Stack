#include "include/Stack.h"

#ifdef DEBUG
#ifdef DUMP

#include <string.h>
#include <assert.h>
#include <stdio.h>

size_t ERR_MSG_SZ = 8192;

static void set_message(char err_msg[], ErrType err)
{
    if(err == ErrType::NOERR)
        return;

    err_msg[0] = '\0';

    if(err & BAD_ALLOC)
        strcat(err_msg, BAD_ALLOCATION);
    if(err & BAD_BUF)
        strcat(err_msg, BAD_BUFFER);
    if(err & BAD_STK_HSH)
        strcat(err_msg, BAD_STACK_HASH);
    if(err & BAD_BUF_HSH)
        strcat(err_msg, BAD_BUFFER_HASH);
    if(err & BAD_STK_CAN)
        strcat(err_msg, BAD_STACK_CANARY);
    if(err & BAD_BUF_CAN)
        strcat(err_msg, BAD_BUFFER_CANARY);
    if(err & REINIT)
        strcat(err_msg, REINITIALIZING);
    if(err & REDESTR)
        strcat(err_msg, REDESTRUCTING);
    if(err & DSTRCTED)
        strcat(err_msg, DESTRUCTED);
    if(err & SZ_OVR_CAP)
        strcat(err_msg, SIZE_OVER_CAP);
    if(err & CAP_OVR_SZ)
        strcat(err_msg, CAP_OVER_SIZE);
    if(err & POP_EMPT_STCK)
        strcat(err_msg, POP_EMPTY_STACK);
    if(err & NULLPTR)
        strcat(err_msg, NULLPOINTER);
    if(err & UNEXPCTD_ERR)
        strcat(err_msg, UNEXPECTED_ERROR);
}

#define BUF_ (stk->buffer)
#define SZ_ (stk->size)
#define PRESET_CAP_ (stk->preset_cap)
#define CAP_ (stk->capacity)

#ifdef HASH
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
#endif // CANARY

static void close_logfile();

static FILE* open_logfile(const char logfile[])
{
    static int first_call = 1;
    static FILE* logstream = stderr;
    
    if(first_call)
    {
        logstream = fopen(logfile, "w");
        first_call = 0;

        if(!logstream)
        {
            fprintf(stderr, "Can't open log file\n");

            logstream = stderr;
        }

        atexit(&close_logfile);
    }

    return logstream;
}

static void close_logfile()
{
    FILE* temp_stream = open_logfile(LOGFILE);

    if(temp_stream != stderr)
        fclose(temp_stream);
}

void dump(const Stack* const stk,  ErrType err, int detailed, const char msg[],
          const char func[], const char file[], int line)
{
    FILE* logstream = open_logfile(LOGFILE);
    
    if(msg[0])
    {
        fprintf(logstream, "%s ", msg);
    }
    
    fprintf(logstream, "Stack [%p] ", stk);
    
    if(err)
    {
        detailed = 1;

        char err_msg[ERR_MSG_SZ];
        set_message(err_msg, err);

        fprintf(logstream, "ERROR (code %.4x)\n", err);
        
        fprintf(logstream, err_msg);
    }
    else 
    {
        fprintf(logstream, "ok\n");
    }

    if(detailed)
    {
        fprintf(logstream, "   called from: %s at %s (%d)\n", func, file, line);

        if(!stk)
            fprintf(logstream, "   nullptr to stack\n");
        else
        {
            if(stk->init_func && stk->init_file && stk->init_line)
                fprintf(logstream, "   initialized: %s at %s (%d)\n\n", stk->init_func, stk->init_file, stk->init_line);
            else
                fprintf(logstream, "   initialized: UNKNOWN\n\n");

            fprintf(logstream, "   buffer[%p]\n", BUF_);
            fprintf(logstream, "   size     = %Iu\n", SZ_);
            fprintf(logstream, "   capacity = %Iu\n", CAP_);
            fprintf(logstream, "   preset capacity = %Iu\n\n", PRESET_CAP_);

            fprintf(logstream, "   Guards:\n");

        #ifdef CANARY
            fprintf(logstream, "     stack  begin = %p\n", BEG_STK_CAN_);
            fprintf(logstream, "     stack  end   = %p\n", END_STK_CAN_);

            if(BUF_ != BUF_POISON && BUF_)
            {
                fprintf(logstream, "     buffer begin = %p\n", BEG_BUF_CAN_);
                fprintf(logstream, "     buffer end   = %p\n", END_BUF_CAN_);
            }
        #endif // CANARY

        #ifdef HASH
            fprintf(logstream, "     stack  hash  = %p\n", STK_HASH_);
        #ifdef BUFFER_HASH
            fprintf(logstream, "     buffer hash  = %p\n\n", BUF_HASH_);
        #endif // BUFFER_HASH
        #endif // HASH

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
    fflush(logstream);
}

void stack_dump_(const Stack* const stk, const char msg[],
                 const char func[], const char file[], int line)
{
    assert(stk && msg && func && file && line);

    ErrType err = stack_verify_(stk);
    
    dump(stk, err, 1, msg, func, file, line);
}

#undef BUF_HASH_
#undef STK_HASH_
#undef BEG_STK_CAN_
#undef END_STK_CAN_
#undef BEG_BUF_CAN_
#undef END_BUF_CAN_
#undef BUF_
#undef SZ_
#undef PRESET_CAP_
#undef CAP_

#endif // DUMP
#endif // DEBUG