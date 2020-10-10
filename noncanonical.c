/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

#define DELIM 0x7E  //delimiter
#define A_RC  0x01  //commands sent by 'Receptor' and answers sent by 'Emissor'
#define A_EM  0x03  //commands sent by 'Emissor' and answers sent by the 'Receptor'

#define SET   0x03  //set command
#define DISC  0x09  //disconect command
#define UA    0x07  //unnumbered acknowledgment
#define RR    0x05  //receiver ready
#define REJ   0x01  //reject 

int stateMachine(int fd) {

  enum state {START, FLAG_RCV, A_RCV, C_RCV, BCC_OK, STOP};

  enum state st = START;
  //read message sent by writenoncanonical
    int n = 0;
    unsigned char buf[6];

    int i = 0;
    bool end = false;
    while (!end) {   
      //read field sent by writenoncanonical
      int res = read(fd,buf[i],1);  

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

          unsigned char bcc = buf[1]^buf[2];

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
  
    return 0;
}

volatile int STOP=FALSE;

int main(int argc, char** argv)
{
    int fd,c, res;
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

    // //read message sent by writenoncanonical
    // int n = 0;
    // char msg[255];
    // while (true) {   
    //   res = read(fd,buf,1);  
    //   strcat(msg, buf);
    //   if (msg[n++] == '\0')
    //     break;
    // }
    // printf("> %s\n", msg);


    // //resend message
    // res = write(fd,msg,n);   
    // printf("%d bytes written\n", res);

    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);
    return 0;
}
