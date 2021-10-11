/** \file
 *  \brief File containing settings of stack protection and dump
 */

#include <stdint.h>

#ifndef CONFIG_H
#define CONFIG_H

                typedef size_t Elem_t;

                /// Path to file for logs
                const char LOGFILE[] = "log.txt";

                /// \brief Turn on protection for stack
                #define DEBUG

#ifdef DEBUG
                /// \brief Turn on canary protection (Does not work without DEBUG define)
                #define CANARY

                /// \brief Turn on error dumps and user dumps (Does not work without DEBUG define)
                #define DUMP

#ifdef DUMP
                /// \brief Turn on all dumps (Does not work without DEBUG and DUMP defines)
                #define DUMP_ALL
#endif

                /// \brief Turn on stack hash (Does not work without DEBUG define)
                #define STACK_HASH
    
                /// \brief Turn on data buffer hash (Does not work without DEBUG define)
                #define BUFFER_HASH
#endif

#endif // CONFIG_H
