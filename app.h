#include "linkLayer.h"

typedef struct {
    int fd;     //file descriptor of serial port
    int status; //EMT_STAT | RCV_STAT
} App;