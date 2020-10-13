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
#define RR    0x05  //receiver ready
#define REJ   0x01  //reject 

#define C_0 0x00
#define RR 0x85
#define RJ 0x81

#define MAX_ATTEMPTS 3

volatile int STOP=FALSE;
int attemps = 0;
bool tryToSend = true, timeout = false;

static enum state {START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, STOP};

void signalHandler(){
  printf("signal\n");
  timeout = true;

  if(attemps == MAX_ATTEMPTS){
    tryToSend = false;
    printf("max try\n");
  }
  else{
    attemps++;
  }
}

int llopen(int fd) {

  enum state st = START;
  //read message sent by writenoncanonical

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

        if (buf[i] == UA) {
          st = C_RCV;
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

int llwrite(int fd) {	//TODO trocar para receber dados por argumento
							//verificar recursivamente BCC2 xor entre bytes de dados
	
	//sen data packages
	unsigned char buffer = {DELIM, A_EM, C_0, A_EM^C_0, 0x21, 0x21, DELIM};
	int n = write(fd, buffer, 6);
	
	unsigned char bcc = 0;
	bool end = false;
  while (!end) { 
     
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

      case FLAG_RCV:

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

        if (buf[i] == RR) { //received package
          st = C_RCV;
          i++;
        }
        else if (buf[i] == RJ) {
			//TODO resend data
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
          st = START; //resend data
          i = 0;
        }

      break;

      }
    }
	
	
	
}
int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    unsigned char buf[BUF_SIZE];
    int i, sum = 0, speed = 0;
    
     if ( (argc < 2) || 
  	     ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	       (strcmp("/dev/ttyS1", argv[1])!=0) )) {
       printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
       exit(1);
     }
    (void) signal(SIGALRM, signalHandler);

  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */

    fd = open(argv[1], O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd <0) {perror(argv[1]); exit(-1); }

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

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 5;   /* blocking read until 5 chars received */



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

	//TODO: organize - put in llopen
    unsigned char buffer[5] = {DELIM, A_EM, SET,  A_EM^SET, DELIM};
    res = write(fd,buffer,BUF_SIZE); 

	
    while(tryToSend){
      alarm(3);
      if(llopen(fd) == 0){
        break;
      }
      printf("try to read again\n");
    }

    printf("exited\n");
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

	  sleep(1);

    close(fd);
    return 0;
}
