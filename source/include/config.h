/** \file
 *  \brief File containing settings of stack protection and dump
 */

#include <stdint.h>

#ifndef CONFIG_H
#define CONFIG_H

                typedef double Elem_t;

                /// Path to file for logs (can be replaced using stack_dump_set_stream)
                const char STACK_DUMPFILE[] = "log.html";

                /// \brief Turn on protection for stack
                #define PROTECT

#ifdef PROTECT
                /// \brief Turn on canary protection (Does not work without PROTECT define)
                #define CANARY

                /// \brief Turn on error dumps and user dumps (Does not work without PROTECT define)
                #define DUMP

#ifdef DUMP
                /// \brief Turn on all dumps (Does not work without PROTECT and DUMP defines)
                #define DUMP_ALL
#endif

                /// \brief Turn on stack hash (Does not work without PROTECT define)
                #define STACK_HASH
    
                /// \brief Turn on data buffer hash (Does not work without PROTECT define)
                #define BUFFER_HASH
#endif

#endif // CONFIG_H
