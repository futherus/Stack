/// \file

#ifndef CONFIG_H
#define CONFIG_H

typedef int Elem_t;

const char LOGFILE[] = "log.txt";

#define ELEM_FORMAT "%d"

/// \brief Turn on protection for stack
#define DEBUG
/// \brief Turn on canary protection (Doesn't work without DEBUG define)
#define CANARY
/// \brief Turn on dump (Doesn't work without DEBUG define)
#define DUMP
/// \brief Turn on hashes (Doesn't work without DEBUG define)
#define HASH
/// \brief Turn on data buffer hash (Doesn't work without HASH and DEBUG defines)
#define BUFFER_HASH

#endif // CONFIG_H
