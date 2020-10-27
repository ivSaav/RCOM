#include "linkLayer.h"

#include <sys/stat.h> 

#define C_START 0x02    //control field
#define C_END   0x03    //control field
#define C_DATA  0x01

#define TLV_LENGTH      0x00    //control Flag for TLV field
#define TLV_FILENAME    0x01  //control Flag for TLV field

#define BUFFER_MAX_SIZE   1024
#define BLOCK_SIZE      512

typedef struct {
    int fd;     //file descriptor of image
    int fileSize; //size of image file
    int port;   //file descriptor of serial port
    int status; //EMT_STAT | RCV_STAT
    int numBlocks; //number of 512B blocks
    char *filename;
} App;

int sendControlFrame(unsigned char controlFlag);
int receiveControlFrame(unsigned char controlFlag);

int sendDataFrames();