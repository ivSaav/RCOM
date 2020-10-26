#include "linkLayer.h"

#define C_START 0x02    //control field
#define C_END   0x03    //control field

#define TLV_SIZE        0x00    //control Flag for TLV field
#define TLV_FILENAME    0x01  //control Flag for TLV field

#define DATA_MAX_SIZE   1024

typedef struct {
    int fd;     //file descriptor of image
    int fileSize; //size of image file
    int port;   //file descriptor of serial port
    int status; //EMT_STAT | RCV_STAT
    char *filename;
} App;

int sendControlPacket(unsigned char controlFlag);