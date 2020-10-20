/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define BAUDRATE B38400
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
#define RR    0x85  //S=1
#define RJ    0x81

volatile int STOP=FALSE;

int sendAcknowledgement(int fd, unsigned char expectedControl){
  unsigned char buffer[5] = {DELIM, A_EM, expectedControl,  A_EM^expectedControl, DELIM};

  int res = write(fd,buffer,BUF_SIZE);

  return res;
}

unsigned char calcBcc2(unsigned char *buffer, int i, unsigned char first, int last_data_index) {

  if (i >= last_data_index-1) {  //reached last element
    return first;
  }
  
  first = first^buffer[i+1];
  return calcBcc2(buffer, ++i, first, last_data_index);
}

int llopen(int fd) {
  
  enum state {START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, STOP};

  enum state st = START;
  //read message sent by writenoncanonical

  int n = 0;
  unsigned char buf[6];

  int i = 0;
  bool end = false;

  unsigned char bcc = 0;
  while (!end) { 
     
    //read field sent by writenoncanonical
    unsigned char byte;
    int res = read(fd,&byte,1);
    buf[i] = byte;
    printf("st: %d  buf: %X\n", st, buf[i]);  

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

        if (buf[i] == SET) {
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
          end = true;
        }
        else {
          st = START;
          i = 0;
        }

      break;

      }
    }
    
    if(sendAcknowledgement(fd, UA) == -1){
      perror("Error sending acknowledgement!");
      exit(3);
    }


    printf("%s", buf);
  
    return 0;
}

int llread(int fd, char * buffer){
  enum state {START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, DATA, BCC2_OK};

  enum state st = START;

  unsigned char byte;
  int i = 0, res, data_received = 0;
  unsigned char bcc = 0;

  static bool nr_set = 1;

  while (true) { 
     
    //read field sent by writenoncanonical
    res = read(fd,&byte,1);
    buffer[i] = byte;
    printf("st: %d  buf: %X\n", st, buffer[i]);  

    switch (st) {

      case START: //validate header

        if (buffer[i] == DELIM) {
          st = FLAG_RCV;
          i++;
        }
        break;

      case FLAG_RCV:  

        if (buffer[i] == A_EM) {
          st = A_RCV;
          i++;
        }
        else if (buffer[i] == FLAG_RCV) {
          continue;
        }
        else {
          i = 0;
          st = START;
        }
        break;

      case A_RCV:

        if (buffer[i] == C_0 || buffer[i] == C_1) {
          nr_set = (buffer[i] == C_1); //alternate between 1 and 0
          st = C_RCV;
          i++;
        }
        else if (buffer[i] == FLAG_RCV) {
            st = FLAG_RCV;
            i = 1;
        }
        else {
            st = START;
            i = 0;
        }
        break;

      case C_RCV:

        bcc = buffer[1]^buffer[2];

        if (buffer[i] == bcc) {
            st = BCC_OK;
            i++;
        }
        else if (buffer[i] == FLAG_RCV) {
          st = FLAG_RCV;
          i = 1;
        }
        else {
          st = START;
          i = 0;
        }
        break;

      case BCC_OK:  //start reading data 
      
        if (buffer[i] == DELIM) { //reached end of frame
          data_received--;

          unsigned char bcc2 = calcBcc2(buffer, 4, buffer[4], 4 + data_received);

          if(buffer[i-1] == bcc2){    //accepted frame

            
            if (sendAcknowledgement(fd, nr_set ? (0x0F & RR) : RR) <= 0) { //if S=1 send S=0
              perror("Couldn't send acknowledgement.\n");
              exit(-1);
            }
  
            st = START; //for further use
            return i;
          }
          else{   //rejected frame
      
            if (sendAcknowledgement(fd, nr_set ? (0x0F & RJ) : RJ) <= 0) {
                perror("Couldn't send acknowledgement.\n");
                exit(-1);
              }

            st = START;
            i = 0;
            data_received = 0;
          }
        }
        else {              //add byte to buffer
          data_received++;
          i++;

          buffer = (char*) realloc(buffer, (6 + data_received)*sizeof(char));
        }

      break;
      }
    }
  
    return -1;
}

int main(int argc, char** argv)
{
    int fd,c;
    struct termios oldtio,newtio;
    char buf[255];

     if ( (argc < 2) || 
  	      ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	       (strcmp("/dev/ttyS1", argv[1])!=0) )) {
       printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
     }


  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */
  
    
    fd = open(argv[1], O_RDWR | O_NOCTTY );
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

    if (llopen(fd)) {
      perror("stateMachine");
      exit(1);
    }

    char * buffer = (char*) malloc(6 * sizeof(char));

    if(llread(fd, buffer) == -1){
      perror("Error reading data!");
      exit(2);
    }

    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
