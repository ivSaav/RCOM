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

#define MAX_ATTEMPTS 3  
#define WAIT_TIME    1  //wait time in seconds

#define MAX_SIZE 1024

#define BUF_SIZE 5

#define EMT_STAT 0  //emissor status
#define RCV_STAT 1  //receiver status

#define DELIM 0x7E  //delimiter
#define A_RC  0x01  //commands sent by 'Receptor' and answers sent by 'Emissor'
#define A_EM  0x03  //commands sent by 'Emissor' and answers sent by the 'Receptor'

#define SET   0x03  //set command
#define DISC  0x09  //disconect command
#define UA    0x07  //unnumbered acknowledgment

#define C_0   0x00  //S=0
#define C_1   0x40  //S=1
#define RR    0x85
#define RJ    0x81   //TODO alternate between 0 and 1 according to ns

#define ESC_OCT 0x7d
#define ESC_MASK 0x20



typedef struct {
    int status;
    struct termios oldtio;
    unsigned int waitTime;           /*Valor do temporizador: 1 s*/
    unsigned int numTransmissions;   /*Número de tentativas em caso defalha*/
    int attempts;
    bool timeout;
    bool send;
    unsigned char frame[MAX_SIZE];
} linkLayer;

enum state {START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, DESTUFFING};

int initLinkLayer(char *port, int status);
int sendAcknowledgement(int fd, unsigned char flag, unsigned char expectedControl);

int receiveFrame(int fd, unsigned char expectedFlag, unsigned char expectedControl);

int llopen(int fd, int status);
int llread(int fd, unsigned char *buffer);
int llwrite(int fd, unsigned char *data, int size);
int llclose(int fd);

int EmtSetupConnection(int fd);
int RcvSetupConnection(int fd) ;

int EmtCloseConnection(int fd);
int RcvCloseConnection(int fd);

unsigned char  calcBcc2(unsigned char *buffer, int i, unsigned char first, int size);
unsigned char RcvCalcBcc2(unsigned char *buffer, int i, unsigned char first, int last_data_index);

int stuffBytes(unsigned char *buffer, int size, unsigned char *stuffed);



void signalHandler();