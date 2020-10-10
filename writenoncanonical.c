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


volatile int STOP=FALSE;

int main(int argc, char** argv)
{
    int fd,c, res;
    struct termios oldtio,newtio;
    unsigned char buf[BUF_SIZE];
    int i, sum = 0, speed = 0;
    
    // if ( (argc < 2) || 
  	//      ((strcmp("/dev/ttyS0", argv[1])!=0) && 
  	//       (strcmp("/dev/ttyS1", argv[1])!=0) )) {
    //   printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
    //   exit(1);
    // }


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

    buf[0] = DELIM;
    buf[1] = A_EM;
    buf[2] = SET;
    buf[3] = buf[1]^buf[2];
    buf[4] = DELIM;

    res = write(fd,buf,BUF_SIZE); 
    printf("%d iiuah\n", res);  

       // //send message
	  // fgets(buf, 255, stdin);
    // int nbytes = 0;
    // for (; nbytes < 255; nbytes++) {
    //   if (buf[nbytes] == '\n'){
    //     buf[nbytes] = '\0';
    //     break;
    //   }
    // }
    // printf("%d bytes written\n", res);

    // //read message sent by noncanonical
    // int n = 0;
    // char msg[255];
    // char rcv_buf[255];
    // while (true) {   
    //   res = read(fd, rcv_buf, 1);
    //   strcat(msg, rcv_buf);
    //   if (msg[n++] == '\0')
    //     break;
    // }
    // printf("> %s\n", msg);
   
    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

	  sleep(10);

    close(fd);
    return 0;
}
