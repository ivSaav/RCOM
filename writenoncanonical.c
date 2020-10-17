/*Non-Canonical Input Processing*/

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
#include <stdbool.h>

#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define BUF_SIZE 5

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


#define MAX_ATTEMPTS 3

static int attemps = 0;
static bool tryToSend = true, timeout = false;

enum state {START, FLAG_RCV, A_RCV, C_RCV, BCC_OK};

void signalHandler(){
  printf("Timeout.\n");
  timeout = true;

  if(attemps == MAX_ATTEMPTS){
    tryToSend = false;
    printf("Exceded max tries.\n");
  }
  else{
    attemps++;
  }
}

int receiveAck(int fd, unsigned char expectedControl){

  static enum state st = START;

  //get acknowledgement
  int n = 0;
  unsigned char buf[6];

  int i = 0;
  bool end = false;
  timeout = false;

  unsigned char bcc = 0;
  while (!(end || timeout)) { 
     
    //read field sent by writenoncanonical
    unsigned char byte;
    int res = read(fd,&byte,1);
    buf[i] = byte;

    //printf("st: %d  buf: %X\n", st, buf[i]); 

    switch (st) {

      case START:

        if (buf[i] == DELIM) {
          st = FLAG_RCV;
          i++;
        }
        break;

      case FLAG_RCV:  //TODO: define other control commands

        if (buf[i] == A_EM) {
          st = A_RCV;
          i++;
        }
        else if (buf[i] == FLAG_RCV) {
          continue;
        }
        else {
          i = 0;
          st = START;
        }
        break;

      case A_RCV:

        if (buf[i] == expectedControl) {
          st = C_RCV;
          i++;
        }
        else if (buf[i] == RJ) {
          st = START;
          printf("Rejected data package.\n");
          return 1;
        }
        else if (buf[i] == FLAG_RCV) {
            st = FLAG_RCV;
            i = 1;
        }
        else {
            st = START;
            i = 0;
        }
        break;

      case C_RCV:

        bcc = buf[1]^buf[2];

        if (buf[i] == bcc) {
            st = BCC_OK;
            i++;
        }
        else if (buf[i] == FLAG_RCV) {
          st = FLAG_RCV;
          i = 1;
        }
        else {
          st = START;
          i = 0;
        }
        break;

      case BCC_OK:

        if (buf[i] == DELIM) {
          return 0;
        }
        else {
          st = START;
          i = 0;
        }

      break;

    }
  }

  return 1;

}

int llopen(int fd) {

  while(tryToSend){
    
    int res = 0;
    //send set frame
    unsigned char buffer[5] = {DELIM, A_EM, SET,  A_EM^SET, DELIM};
    res = write(fd,buffer,BUF_SIZE); 

    alarm(3);

    if (!receiveAck(fd, UA)) {
        tryToSend = false;
        return 0; //success
    }
    else {
      printf("Invalid acknowledgement.\n");
    }

  }
    
    return 1; //failure
}

unsigned char  calcBcc2(unsigned char *buffer, int i, unsigned char first) {

  if (buffer[i] == '\0') {  //reached last element
    return first;
  }
  
  first = first^buffer[i+1];
  return calcBcc2(buffer, ++i, first);
}

int llwrite(int fd /*, unsigned char *data, int size*/) {	
  
  bool rcvRR = false;
  int numTries = 0;
  static unsigned char ns = C_0;

  bool sentData = false;

	do {
    unsigned char dataBuffer[3] = {0x21, 0x12, '\0'}; //TODO trocar para receber dados por argumento
    int size = 2;
    unsigned char bcc2 = calcBcc2(dataBuffer, 0, dataBuffer[0]);

    //intialize data frame
    unsigned char buffer[size + 6]; /*= {DELIM, A_EM, ns, A_EM^C_0, dataBuffer, bcc2, DELIM};*/
    buffer[0] = DELIM;
    buffer[1] = A_EM;
    buffer[2] = ns;
    buffer[3] = A_EM^C_0; //TODO change according to ns

    int buffPos = 3;
    for (int i = 0; i < size; i++) {  //concatenating data buffer
        buffer[i+buffPos] = dataBuffer[i];
    }
    buffPos += size;
    buffer[buffPos] = bcc2;
    buffer[++buffPos] = DELIM;

    //send data
    int nbytes = size + 6;
    int n = write(fd, buffer, nbytes);

    alarm(3);

    if (!receiveAck(fd, RR)) {
        tryToSend = false;
        return 0; //success
    }
    else {
      numTries++;
    }
  
  } while (!sentData && (numTries < 3));

   return 1;
}
	
	

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    unsigned char buf[BUF_SIZE];
    int i, sum = 0, speed = 0;
    
    //  if ( (argc < 2) || 
  	//      ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	//        (strcmp("/dev/ttyS1", argv[1])!=0) )) {
    //    printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
    //    exit(1);
    //  }
    (void) signal(SIGALRM, signalHandler);

  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */

    fd = open(argv[1], O_RDWR | O_NOCTTY);
    if (fd <0) { perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 3;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 0;   /* blocking read until 5 chars received */



  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) pr�ximo(s) caracter(es)
  */

    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

  
    printf("New termios structure set\n");
	
    
    if (llopen(fd)) {
      perror("Couldn't open connection to noncanonical.\n");
      exit(-1);
    }

    printf("Connection established.\n");
    
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

	  sleep(1);

    close(fd);
    return 0;
}
