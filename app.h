#include "linkLayer.h"

#include <sys/stat.h> 
#include <math.h>

#define C_START 0x02    //control field
#define C_END   0x03    //control field
#define C_DATA  0x01

#define TLV_SIZE      0x00    //control Flag for TLV field
#define TLV_FILENAME    0x01  //control Flag for TLV field
#define TLV_BLOCK       0x02

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

/**
 * Converts and integer into hexadecimal
 * 
 * unsigned char *ret :- variable to store the hexadecimal value
 * int value :- value of the integer to be converted
 * 
 * returns the length of the hexadecimal
 */
int intToHexa(unsigned char *ret, int value);

/**
 * Sends a Control Frame through the file descriptor
 * described in struct App as fd
 * 
 * using char controlFlag :- controlFlag representing if
 * it's the begging or end of the connection
 * 
 * returns 0 if it was sent successfully
 */
int sendControlFrame(unsigned char controlFlag);

/**
 * Receives a Control Frame through the file descriptor
 * described in struct App as fd
 * 
 * using char controlFlag :- control flag to be compared to
 * the one received
 * 
 * returns 0 if it was sent successfully
 */
int receiveControlFrame(unsigned char controlFlag);

/**
 * Sends a Data Frames through the file descriptor
 * described in struct App as fd
 * 
 * returns 0 if it was sent successfully
 */
int sendDataFrames();

/**
 * Sends a Data Frames through the file descriptor
 * described in struct App as fd
 * 
 * returns 0 if it was sent successfully
 */
int receiveDataFrames();