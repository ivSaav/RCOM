#include "linkLayer.h"

static linkLayer ll;

void signalHandler(){
  printf("Timeout.\n");
  ll.timeout = true;

  if(ll.attempts >= ll.numTransmissions){
    ll.send = false;
    printf("Exceded max tries.\n");
  }
  else{
    ll.attempts++;
  }
}

void setAlarmFlags() {
  ll.attempts = 1;
  ll.timeout = false;
  ll.send = true;
}

/*
  Open serial port device for reading and writing and not as controlling tty
  because we don't want to get killed if linenoise sends CTRL-C.
*/
int initLinkLayer(char *port, int status) {
  
  setAlarmFlags();

  ll.waitTime = WAIT_TIME;
  ll.numTransmissions = MAX_ATTEMPTS;
  ll.status = status;

  int portfd;
  struct termios newtio;

  portfd = open(port, O_RDWR | O_NOCTTY);
  if (portfd <0) { perror(port); exit(-1); }

  // Save current port settings
  if (tcgetattr(portfd,&ll.oldtio) == -1) {
    perror("tcgetattr");
    exit(-1);
  }

  // Set new flags to be used in the port
  bzero(&newtio, sizeof(newtio));
  newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
  newtio.c_iflag = IGNPAR;
  newtio.c_oflag = 0;

  /* set input mode (non-canonical, no echo,...) */
  newtio.c_lflag = 0;

  newtio.c_cc[VTIME]    = ll.waitTime;   /* inter-character timer unused */
  newtio.c_cc[VMIN]     = 5;   /* blocking read until 5 chars received */

  tcflush(portfd, TCIOFLUSH);

  if (tcsetattr(portfd,TCSANOW,&newtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }

  printf("New termios structure set\n");
  return portfd;

}

//send supervision/numbered frame
int sendAcknowledgement(int fd, unsigned char flag, unsigned char expectedControl){
  unsigned char buffer[5] = {DELIM, flag, expectedControl,  flag^expectedControl, DELIM};

  int res = write(fd,buffer,BUF_SIZE);

  return res;
}

//receive supervision/numbered frame
int receiveFrame(int fd, unsigned char expectedFlag, unsigned char expectedControl){

   enum state st = START;

  //get acknowledgement
  int n = 0;
  unsigned char buf[6];

  int i = 0;
  bool end = false;
  
  setAlarmFlags();

  unsigned char bcc = 0;

  while (!(end || ll.timeout)) {

    //read field sent by writenoncanonical
    unsigned char byte;
    int res = read(fd,&byte,1);
    buf[i] = byte;

    switch (st) {

      case START:

        if (buf[i] == DELIM) {
          st = FLAG_RCV;
          i++;
        }
        break;

      case FLAG_RCV:  //TODO: define other control commands

        if (buf[i] == expectedFlag) {   //A_EM | A_RC
          st = A_RCV;
          i++;
        }
        else if (buf[i] == DELIM) {
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
          return 1;
        }
        else if (buf[i] == DELIM) {
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
        else if (buf[i] == DELIM) {
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
          st = START; //for further use
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

int llopen(int fd, int status) {

    switch (status) {
        case EMT_STAT: //emissor
            if (EmtSetupConnection(fd)) {
                perror("Couldn't setup connection.\n");
                exit(1);
            }
            break;
        case RCV_STAT:  //receiver
            if (RcvSetupConnection(fd)){
                perror("Couldn't setup connection.\n");
                exit(2);
            }
            break;
        default:
            perror("Invalid flag.\n");
            exit(3);
    }

    return 0;
}

//setup connection from writer
int EmtSetupConnection(int fd) {

     while(ll.send){

        int res = 0;
        //send set frame
        unsigned char buffer[5] = {DELIM, A_EM, SET,  A_EM^SET, DELIM};
        res = write(fd,buffer,BUF_SIZE);

        alarm(3);

        if (!receiveFrame(fd, A_EM, UA)) { //receive acknowledgement from receiver
            return 0; //success
        }
    else {
        printf("Invalid acknowledgement.\n");
    }

  }

    return 1; //failure
}

//setup connection from receiver
int RcvSetupConnection(int fd) {

    int n = 0;
    unsigned char buf[6];

    int i = 0;
    bool end = false;

    if (receiveFrame(fd, A_EM, SET)) { //receive SET frame from writer
        perror("Received invalid frame.\n");
        exit(1);
    }

    if(sendAcknowledgement(fd, A_EM, UA) == -1){
        perror("Error sending acknowledgement!");
        exit(2);
    }

    printf("Connection established.\n");
    return 0;
}

int llclose(int fd) {

  setAlarmFlags();

  int ret = 0;
  if (ll.status == EMT_STAT) {
    ret = EmtCloseConnection(fd);
  }
  else {
    ret = RcvCloseConnection(fd);
  }

  // Restore the port settings
  if ( tcsetattr(fd,TCSANOW,&ll.oldtio) == -1) {
    perror("tcsetattr");
    exit(-1);
  }

  return ret;

}

int EmtCloseConnection(int fd) {

  while(ll.send && (ll.attempts <= 3)){

      int res = 0;
      //send set frame
      unsigned char buffer[5] = {DELIM, A_EM, DISC,  A_EM^DISC, DELIM};
      res = write(fd,buffer,BUF_SIZE);

      alarm(3);

      if (receiveFrame(fd, A_RC, DISC)) { //receive DISC from receiver
        printf("Invalid acknowledgement.\n");
      }

      if (sendAcknowledgement(fd, A_RC,  UA) <= 0) {
        printf("Couldn't send acknowledgment.\n");
      }
      else {
        printf("Connection closed\n");
        return 0;
      }

  }

  return 1;

}

int RcvCloseConnection(int fd) {

  setAlarmFlags();

  while(ll.send){

    alarm(3);

    if (receiveFrame(fd, A_EM, DISC)) { //receive DISC from emissor
        printf("Invalid frame (llclose).\n");
    }

    int res = 0;
    //send DISC frame
    unsigned char buffer[5] = {DELIM, A_RC, DISC,  A_RC^DISC, DELIM};
    res = write(fd,buffer,BUF_SIZE);
    if (res <= 0) {
      perror("write error\n");
      exit(1);
    }

    alarm(3);

    if (!receiveFrame(fd, A_RC, UA)) { //receive UA from emissor
        printf("Connection closed\n");
        return 0; //success
    }
    else {
      printf("Invalid frame (llclose).\n");
    }


  }

  return 1;

}


unsigned char RcvCalcBcc2(unsigned char *buffer, int i, unsigned char first, int last_data_index) {

  if (i >= last_data_index-1) {  //reached last element
    return first;
  }
  //printf("first %X i %d  buff %c\n", first, i, buffer[i+1]);
  first = first^buffer[i+1];

  return RcvCalcBcc2(buffer, ++i, first, last_data_index);
}

unsigned char  calcBcc2(unsigned char *buffer, int i, unsigned char first, int size) {

  if (i >= size-2) {  //reached last element
    return first;
  }
  
  //printf("first %X i %d  buff %c\n", first, i, buffer[i+1]);
  first = first^buffer[i+1];
  return calcBcc2(buffer, ++i, first, size);
}

int stuffBytes(unsigned char *buffer, int size, unsigned char *stuffed) {

  int j = 0;
  for (int i = 0; i < size; i++) {

    if (buffer[i] == DELIM) {
      stuffed[j] = ESC_OCT;
      stuffed[++j] = buffer[i]^ESC_MASK;
    }
    else if (buffer[i] == ESC_OCT) {
      stuffed[j] = ESC_OCT;
      stuffed[++j] = buffer[i]^ESC_MASK;
    }
    else {
      stuffed[j] = buffer[i];
    }
    j++;
  }

  return j-1;
}


int llwrite(int fd, unsigned char *data, int size) {

  bool rcvRR = false;
  int numTries = 0;
  setAlarmFlags();
  static bool ns_set = false; //S starts at 0

  bool sentData = false;

	do {
    
    unsigned char bcc2 = calcBcc2(data, 0, data[0], size);

    unsigned char stuffed[MAX_SIZE]; 
    int ndata = stuffBytes(data, size, stuffed);
    
    //intialize data frame header
    unsigned char buffer[ndata + 6];
    buffer[0] = DELIM;
    buffer[1] = A_EM;
    buffer[2] = ns_set ? 0x40 : 0x00; //Control flag: S=1 --> 0x40 ; S=0 --> 0x00
    buffer[3] = buffer[1]^buffer[2];

    int buffPos = 4;
    for (int i = 0; i < ndata; i++) {  //concatenating stuffed data buffer
        buffer[i+buffPos] = stuffed[i];
    }

    buffPos += ndata;
    buffer[buffPos] = bcc2;
    buffer[++buffPos] = DELIM;

     
    //send data
    int nbytes = ndata + 6;

    int n = write(fd, buffer, nbytes);

    alarm(3);

    //receive acknowledgement from receiver
    if (!receiveFrame(fd, A_EM, ns_set ? (0x0F & RR) : RR)) { //if S=0 expect to receive S=1
        ns_set = !ns_set;
        return n; //success
    }
    else {
      printf("Invalid acknowledment\n");
      numTries++;
    }

  } while (!sentData && (numTries < 3));

   return -1;
}

int llread(int fd, unsigned char * buffer){

  enum state st = START;

  unsigned char byte;
  int i = 0, res, data_received = 0;
  unsigned char bcc = 0;

  static bool nr_set = 1;

  int index = 0; //index for return buffer
  while (true) {
    //read field sent by writenoncanonical
    res = read(fd,&byte,1);
    ll.frame[i] = byte;

    switch (st) {

      case START: //validate header

        if (ll.frame[i] == DELIM) {
          st = FLAG_RCV;
          i++;
        }
        break;

      case FLAG_RCV:

        if (ll.frame[i] == A_EM) {
          st = A_RCV;
          i++;
        }
        else if (ll.frame[i] == DELIM) {
          continue;
        }
        else {
          i = 0;
          st = START;
        }
        break;

      case A_RCV:

        if (ll.frame[i] == C_0 || ll.frame[i] == C_1) {
          nr_set = (ll.frame[i] == C_1); //alternate between 1 and 0
          st = C_RCV;
          i++;
        }
        else if (ll.frame[i] == DELIM) {
            st = FLAG_RCV;
            i = 1;
        }
        else {
            st = START;
            i = 0;
        }
        break;

      case C_RCV:

        bcc = ll.frame[1]^ll.frame[2];

        if (ll.frame[i] == bcc) {
            st = BCC_OK;
            i++;
        }
        else if (ll.frame[i] == DELIM) {
          st = FLAG_RCV;
          i = 1;
        }
        else {
          st = START;
          i = 0;
        }
        break;

      case BCC_OK:  //start reading data

        if (ll.frame[i] == DELIM) { //reached end of frame
          data_received--;

          unsigned char bcc2 = RcvCalcBcc2(ll.frame, 4, ll.frame[4], 4 + data_received);  

          //printf("bcc %X %X\n", bcc2, frame[i-1]);

          if(ll.frame[i-1] == bcc2){    //accepted frame  TODO destuffbytes

            if (sendAcknowledgement(fd, A_EM,  nr_set ? (0x0F & RR) : RR) <= 0) { //if S=1 send S=0
              perror("Couldn't send acknowledgement.\n");
              exit(-1);
            }

            //printf("////////\n");
            st = START; //for further use
            return i;
          }
          else{   //rejected frame

            if (sendAcknowledgement(fd, A_EM, nr_set ? (0x0F & RJ) : RJ) <= 0) {
                perror("Couldn't send acknowledgement.\n");
                exit(-1);
              }

            printf("Rejected data packet.\n");

            st = START;
            i = 0;
            data_received = 0;
          }
        }
        else {              //add byte to frame
          if(ll.frame[i] == 0x7d){
            st = DESTUFFING;
          }
          else{
            buffer[index++] = ll.frame[i];
            data_received++;
            i++;
          }
        }
        break;

      case DESTUFFING:

        if(ll.frame[i] == 0x5e){
          ll.frame[i] = 0x7e;
        }
        else if(ll.frame[i] == 0x5d){
          ll.frame[i] = 0x7d;
        }

        buffer[index++] = ll.frame[i];
        data_received++;
        i++;

        st = BCC_OK;

      break;

      }
    }

    return -1;
}
