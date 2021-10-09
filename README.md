# stack
Software implemented stack.
### description
Data structure with property **LIFO** (last in - first out).
Includes non-cryptic hashing and canary protection.
### usage
Change **config.h** to configure:
* `Elem_t`              - elements' type 
* `ELEM_FORMAT`         - format for printing elements in dump
* `LOGFILE`             - path to file for log dump
* `#define DEBUG`       - comment line to turn off all protection and dump
* `#define CANARY`      - comment line to turn off canary protection
* `#define DUMP`        - comment line to turn off dump
* `#define HASH`        - comment line to turn off all hashes
* `#define BUFFER_HASH` - comment line to turn off data buffer hash

Usage of stack functions is described in documentation
