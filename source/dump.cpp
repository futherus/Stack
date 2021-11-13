#include "include/config.h"
#include "include/Stack.h"
#include "include/dump.h"

#ifndef __USE_MINGW_ANSI_STDIO
#define __USE_MINGW_ANSI_STDIO 1
#endif
#include <stdio.h>
#include <string.h>
#include <assert.h>

#ifdef DUMP

static const char HTML_INTRO[] = "<html>"
                                 "<head>"
                                    "<title>"
                                      "Stack log"
                                    "</title>"
                                    "<style>"
                                      ".ok {"
                                        "color: springgreen;"
                                        "font-weight: bold;"
                                      "}"
                                      ".error{"
                                        "color: red;"
                                        "font-weight: bold;"
                                      "}"
                                      ".log{"
                                        "color: #C5D0E6;"
                                      "}"
                                      ".title{"
                                        "color: #E59E1F;"
                                        "text-align: center;"
                                        "font-weight: bold;"
                                      "}"
                                    "</style>"
                                  "</head>"
                                  "<body bgcolor=\"#2F353B\">"
                                  "<pre class = \"log\">";

static const char HTML_OUTRO[] = "</pre>"
                                 "</body>"
                                 "</html>";

static const char HTML_OK[] = "<strong class = \"ok\">"
                                "ok"
                              "</strong>";

#define HTML_DUMP_MSG(MSG) fprintf(logstream, "%s%s %s", "<span class = title>", MSG, "</span>")

static FILE* DUMP_STREAM = nullptr;
static void (*PRINT_ELEM)(FILE*, const Elem_t*) = nullptr;

//WARNING: not checked against overflow
const size_t ERR_MSG_SZ = 4096;

static void set_message_(char err_msg[], Stack_err err)
{
    if(err == Stack_err::NOERR)
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
    if(err & POP_EMPT_STK)
        strcat(err_msg, POP_EMPTY_STACK);
    if(err & NULLPTR)
        strcat(err_msg, NULLPOINTER);
}

#define BUF_ (stk->buffer)
#define SZ_ (stk->size)
#define CAP_ (stk->capacity)

#ifdef STACK_HASH
    #define STK_HASH_ (stk->stk_hash) 
#endif // STACK_HASH
#ifdef BUFFER_HASH
    #define BUF_HASH_ (stk->buf_hash)
#endif // BUFFER_HASH

#ifdef CANARY
    #define BEG_STK_CAN_ (stk->beg_can)
    #define END_STK_CAN_ (stk->end_can)
    #define BEG_BUF_CAN_ (*(((guard_t*) BUF_) - 1))
    #define END_BUF_CAN_ (*((guard_t*) (BUF_ + CAP_)))
#endif // CANARY

void dump_(const Stack* const stk,  Stack_err err, Stack_dump_lvl level, const char msg[],
           const char func[], const char file[], int line)
{
    if(err)
        level = Stack_dump_lvl::DETAILED;
    
    if(level == Stack_dump_lvl::ONLYERR)
        return;

    FILE* logstream = DUMP_STREAM;
    if(!logstream)
        return;
        
    if(msg[0])
    {
        HTML_DUMP_MSG(msg);
    }
    
    fprintf(logstream, "Stack [%p] ", stk);
    
    if(err)
    {
        char err_msg[ERR_MSG_SZ];
        set_message_(err_msg, err);

        fprintf(logstream, "<span class = \"error\">ERROR (code %.4d)\n%s</span>", err, err_msg);
    }        
    else 
    {
        fprintf(logstream, "%s\n", HTML_OK);
    }

    if(level == Stack_dump_lvl::DETAILED)
    {
        fprintf(logstream, "    date:        %s %s\n", __DATE__, __TIME__);
        fprintf(logstream, "    called from: %s at %s (%d)\n", func, file, line);

        if(!stk)
            fprintf(logstream, "    nullptr to stack\n");
        else
        {
            if(stk->init_func && stk->init_file && stk->init_line)
                fprintf(logstream, "    initialized: %s at %s (%d)\n\n", stk->init_func, stk->init_file, stk->init_line);
            else
                fprintf(logstream, "    initialized: UNKNOWN\n\n");

            fprintf(logstream, "    buffer[%p]\n", BUF_);
            fprintf(logstream, "    size          = %llu\n", SZ_);
            fprintf(logstream, "    capacity      = %llu\n", CAP_);

            fprintf(logstream, "    Guards:\n");

#ifdef CANARY
            fprintf(logstream, "     stack  begin = %llx\n", BEG_STK_CAN_);
            fprintf(logstream, "     stack  end   = %llx\n", END_STK_CAN_);

            if(BUF_ != BUF_POISON && BUF_)
            {
                fprintf(logstream, "     buffer begin = %llx\n", BEG_BUF_CAN_);
                fprintf(logstream, "     buffer end   = %llx\n", END_BUF_CAN_);
            }
#endif

#ifdef STACK_HASH
            fprintf(logstream, "     stack  hash  = %llx\n", STK_HASH_);
#endif
#ifdef BUFFER_HASH
            fprintf(logstream, "     buffer hash  = %llx\n", BUF_HASH_);
#endif 

            if(BUF_ && BUF_ != BUF_POISON)
            {
                if(!PRINT_ELEM)
                {
                    fprintf(logstream, "    NO FUNCTION FOR PRINTING (use stack_dump_set_print())\n\n");
                    fflush(logstream);

                    return;
                }
                
                fprintf(logstream, "    {\n");

                size_t iter = 0;

                while(iter < CAP_ || iter < SZ_)
                {
                    if(iter < SZ_)
                        fprintf(logstream, "    #");
                    else
                        fprintf(logstream, "     ");

                    fprintf(logstream, "%7.1llu: ", iter);
                    PRINT_ELEM(logstream, &stk->buffer[iter]);
                    fprintf(logstream, "\n");

                    iter++;
                }

                fprintf(logstream, "    }\n");
            }
        }
    }
    fflush(logstream);
}

static void close_dumpfile_()
{
    fprintf(DUMP_STREAM, "%s", HTML_OUTRO);

    if(fclose(DUMP_STREAM) != 0)
        perror("Stack dump file can't be succesfully closed");
}

void stack_dump_init(FILE* dumpstream, void (*print_func)(FILE*, const Elem_t*))
{
    if(print_func)
        PRINT_ELEM = print_func;

    if(dumpstream)
    {
        DUMP_STREAM = dumpstream;
        return;
    }

    if(STACK_DUMPFILE[0] != 0)
    {
        DUMP_STREAM = fopen(STACK_DUMPFILE, "w");

        if(DUMP_STREAM)
        {
            fprintf(DUMP_STREAM, "%s", HTML_INTRO);

            atexit(&close_dumpfile_);
            return;
        }
    }

    perror("Can't open dump file");
    DUMP_STREAM = stderr;

    return;
}

Stack_err stack_dump_(const Stack* const stk, const char msg[],
                      const char func[], const char file[], int line)
{
    assert(stk && msg && func && file && line);

    Stack_err err = stack_verify_(stk);
    
    dump_(stk, err, Stack_dump_lvl::DETAILED, msg, func, file, line);

    return err;
}

#ifdef BUFFER_HASH
    #undef BUF_HASH_
#endif // BUFFER_HASH
#ifdef STACK_HASH
    #undef STK_HASH_
#endif // STACK_HASH

#ifdef CANARY
    #undef BEG_STK_CAN_
    #undef END_STK_CAN_
    #undef BEG_BUF_CAN_
    #undef END_BUF_CAN_
#endif // CANARY

#undef BUF_
#undef SZ_
#undef PRESET_CAP_
#undef CAP_

#else // DUMP

void stack_dump_init(FILE* dumpstream, void (*print_func)(FILE*, const Elem_t*))
{
    void(0);
}

#endif // DUMP
