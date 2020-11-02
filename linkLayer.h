#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <strings.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */

#define MAX_ATTEMPTS 3  // Number of maximum attemps to send a frame
#define WAIT_TIME    1  // Time to wait before resending in seconds

#define MAX_SIZE 1024   // Maximum size of a single frame

#define BUF_SIZE 5      // Standard size of buufer

#define EMT_STAT 0      // Emissor status
#define RCV_STAT 1      // Receiver status

#define DELIM 0x7E      // Delimiter Flag
#define A_RC  0x01      // Flag that identifies commands sent by 'Receptor' and answers sent by 'Emissor'
#define A_EM  0x03      // Flag that identifies commands sent by 'Emissor' and answers sent by the 'Receptor'

#define SET   0x03      // Set command
#define DISC  0x09      // Disconect command
#define UA    0x07      // Unnumbered acknowledgment

#define C_0   0x00      // S=0
#define C_1   0x40      // S=1
#define RR    0x85      // Receiver Ready
#define RJ    0x81      // Reject frame

#define ESC_OCT 0x7d    // Escape octet
#define ESC_MASK 0x20   // Escape mask



typedef struct {
    int status;                     /*TRANSMITER | RECEIVER*/
    struct termios oldtio;          /*Termios struct before starting the connection*/
    unsigned int waitTime;          /*Timer value: 1 s*/
    unsigned int numTransmissions;  /*Number of tries before closing the connection*/
    int attempts;                   /*Number of attempts made*/
    bool timeout;                   /*Flag to check if a timeout occured*/
    bool send;                      /*Flag to check if it is sending information*/
    unsigned char frame[MAX_SIZE];  /*Buffer to hold the frame receveid or to be sent*/
} linkLayer;

enum state {START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, DESTUFFING};

/*
 * Open serial port device for reading and writing and not as controlling tty
 * because we don't want to get killed if linenoise sends CTRL-C.
 * 
 * char *port :- indentifier of the port to be used in the connection.
 * int status :- value of the status to be savend into linkLayer struct.
 * 
 * returns the file descriptor that was open for the connection.
*/
int initLinkLayer(char *port, int status);

/*
 * Sends and acknowledgement throudh the file descriptor fd with the specified flag.
 * 
 * int fd :- file descriptor through where the acknowledgement is sent.
 * unsigned char flag :- address flag.
 * unsigned char expectedControl :- control flag.
 * 
 * returns the number of bytes writen to fd.
*/
int sendAcknowledgement(int fd, unsigned char flag, unsigned char expectedControl);

/*
 * Read a frame from the file descriptor fd. The frame should have the addres flag and
 * control flag given in the arguments of this fuction.
 * 
 * int fd :- file descriptor through where the acknowledgement is sent.
 * unsigned char expectedFlag :- expected address flag, will be compared to the one received.
 * unsigned char expectedControl :- expected control flag, will be compared to the one received.
*/
int receiveFrame(int fd, unsigned char expectedFlag, unsigned char expectedControl);

/*
 * Function to be used in the application level to open the connection
 * between two devices.
 * 
 * int fd :- file descriptor through which the connection will be made.
 * int status :- flag representing if it's the Receiver os Transmitter
 * 
 * returns the identifier of the conection or a negative value in case of 
 * error.
 */
int llopen(int fd, int status);

/*
 * Function to be used in the application level to read frames.
 * 
 * int fd :- file descriptor through which the frame was sented.
 * unsigned char *buffer :- buffer to save the frame read
 * 
 * returns the lenght of the buffer (number of characters read)
 *  or a negative value in case of error.
 */
int llread(int fd, unsigned char *buffer);

/*
 * Function to be used in the application level to send frames.
 * 
 * int fd :- file descriptor through which the frame will be sented.
 * unsigned char *data :- buffer where the data to be sented is saved.
 * int size :- length of the data array.
 * 
 * returns the number of characters written or a negative value in case of error.
 */
int llwrite(int fd, unsigned char *data, int size);

/*
 * Function to be used in the application level to close the connection
 * between two devices.
 * 
 * int fd :- file descriptor which identifies the connection link.
 * 
 * returns a positive value in case of success or a negative value 
 * in case of error.
 */
int llclose(int fd);

/*
 * Function to be used by the protocol to setup the connection in the
 * emitter side
 * 
 * int fd :- file descriptor which identifies the connection link.
 * 
 * returns 0 in case os success, or 1 otherwise.
 */
int EmtSetupConnection(int fd);

/*
 * Function to be used by the protocol to setup the connection in the
 * receiver side
 * 
 * int fd :- file descriptor which identifies the connection link.
 * 
 * returns 0 in case os success, or 1 otherwise.
 */
int RcvSetupConnection(int fd) ;

/*
 * Function to be used by the protocol to close the connection in the
 * emitter side
 * 
 * int fd :- file descriptor which identifies the connection link.
 * 
 * returns 0 in case os success, or 1 otherwise.
 */
int EmtCloseConnection(int fd);

/*
 * Function to be used by the protocol to close the connection in the
 * receiver side
 * 
 * int fd :- file descriptor which identifies the connection link.
 * 
 * returns 0 in case os success, or 1 otherwise.
 */
int RcvCloseConnection(int fd);

/*
 * Calculate the respective BCC2 of a buffer in a recursive way.This one
 * should be use in the emitter side.
 * 
 * unsigned char *buffer :- buffer with the values used to calculate bcc2.
 * int i :- integer representing the indice of the buffer to use.
 * usigned char first :- bcc2 value up until now.
 * int size :- number of data to be used to calculate bcc2
 * 
 * returns Bcc2 value
 */
unsigned char  calcBcc2(unsigned char *buffer, int i, unsigned char first, int size);

/*
 * Stuffs the data buffer so the data get worngly recognized as a flag.
 * 
 * unsigned char *buffer :- buffer with the data.
 * int size :- size of the buffer.
 * usigned  char*stuffed :- array where we save the stuffed data
 * 
 * returns the size of the stuffed data.
 */
int stuffBytes(unsigned char *buffer, int size, unsigned char *stuffed);

/**
 * Simple signal handler to permit us to timeout and resend a frame.
 */
void signalHandler();