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


int sendAcknowledgement(int fd, unsigned char flag, unsigned char expectedControl){
  unsigned char buffer[5] = {DELIM, flag, expectedControl,  flag^expectedControl, DELIM};

  int res = write(fd,buffer,BUF_SIZE);

  return res;
}


int receiveFrame(int fd, unsigned char expectedFlag, unsigned char expectedControl){

   enum state st = START;

  // Get acknowledgement
  int n = 0;
  unsigned char buf[6];

  int i = 0;
  bool end = false;
  
  setAlarmFlags();

  unsigned char bcc = 0;

  while (!(end || ll.timeout)) {

    // Read field sent by writenoncanonical
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
          st = START; // For further use
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
        case EMT_STAT: // Emissor
            if (EmtSetupConnection(fd)) {
                perror("Couldn't setup connection.\n");
                exit(1);
            }
            break;
        case RCV_STAT:  // Receiver
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


int EmtSetupConnection(int fd) {

     while(ll.send){

        int res = 0;
        // Send set frame
        unsigned char buffer[5] = {DELIM, A_EM, SET,  A_EM^SET, DELIM};
        res = write(fd,buffer,BUF_SIZE);

        alarm(3);

        if (!receiveFrame(fd, A_EM, UA)) { // Receive acknowledgement from receiver
            return 0; // Success
        }
    else {
        printf("Invalid acknowledgement.\n");
    }

  }

    return 1; // Failure
}


int RcvSetupConnection(int fd) {

    int n = 0;
    unsigned char buf[6];

    int i = 0;
    bool end = false;

    if (receiveFrame(fd, A_EM, SET)) { // Receive SET frame from writer
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

      // Send set frame
      unsigned char buffer[5] = {DELIM, A_EM, DISC,  A_EM^DISC, DELIM};
      res = write(fd,buffer,BUF_SIZE);

      alarm(3);

      if (receiveFrame(fd, A_RC, DISC)) { // Receive DISC from receiver
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

    if (receiveFrame(fd, A_EM, DISC)) { // Receive DISC from emissor
        printf("Invalid frame (llclose).\n");
    }

    int res = 0;

    // Send DISC frame
    unsigned char buffer[5] = {DELIM, A_RC, DISC,  A_RC^DISC, DELIM};
    res = write(fd,buffer,BUF_SIZE);
    if (res <= 0) {
      perror("write error\n");
      exit(1);
    }

    alarm(3);

    if (!receiveFrame(fd, A_RC, UA)) { // Receive UA from emissor
        printf("Connection closed\n");
        return 0; // Success
    }
    else {
      printf("Invalid frame (llclose).\n");
    }


  }

  return 1;

}


unsigned char RcvCalcBcc2(unsigned char *buffer, int i, unsigned char first, int last_data_index) {

  if (i >= last_data_index-1) {  // Reached last element
    return first;
  }

  first = first^buffer[i+1];

  return RcvCalcBcc2(buffer, ++i, first, last_data_index);
}

unsigned char  calcBcc2(unsigned char *buffer, int i, unsigned char first, int size) {

  if (i >= size-2) {  // Reached last element
    return first;
  }
  
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
  static bool ns_set = false; // S starts at 0

  bool sentData = false;

	do {
    
    unsigned char bcc2 = calcBcc2(data, 0, data[0], size);

    unsigned char stuffed[MAX_SIZE]; 
    int ndata = stuffBytes(data, size, stuffed);
    
    // Intialize data frame header
    unsigned char buffer[ndata + 6];
    buffer[0] = DELIM;
    buffer[1] = A_EM;
    buffer[2] = ns_set ? 0x40 : 0x00; // Control flag: S=1 --> 0x40 ; S=0 --> 0x00
    buffer[3] = buffer[1]^buffer[2];

    int buffPos = 4;
    // Concatenating stuffed data buffer
    for (int i = 0; i < ndata; i++) {  
        buffer[i+buffPos] = stuffed[i];
    }

    buffPos += ndata;
    buffer[buffPos] = bcc2;
    buffer[++buffPos] = DELIM;

     
    // Send data
    int nbytes = ndata + 6;

    int n = write(fd, buffer, nbytes);

    alarm(3);

    // Receive acknowledgement from receiver
    if (!receiveFrame(fd, A_EM, ns_set ? (0x0F & RR) : RR)) { // If S=0 expect to receive S=1
        ns_set = !ns_set;
        return n; // Success
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

  int index = 0; // Index for return buffer
  while (true) {
    // Read field sent by writenoncanonical
    res = read(fd,&byte,1);
    ll.frame[i] = byte;

    switch (st) {

      case START: // Validate header

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
          nr_set = (ll.frame[i] == C_1); // Alternate between 1 and 0
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

      case BCC_OK:  // Start reading data

        if (ll.frame[i] == DELIM) { // Reached end of frame
          data_received--;

          unsigned char bcc2 = RcvCalcBcc2(ll.frame, 4, ll.frame[4], 4 + data_received);  

          if(ll.frame[i-1] == bcc2){    // Accepted frame

            if (sendAcknowledgement(fd, A_EM,  nr_set ? (0x0F & RR) : RR) <= 0) { // If S=1 send S=0
              perror("Couldn't send acknowledgement.\n");
              exit(-1);
            }

            st = START; // For further use
            return i;
          }
          else{   // Rejected frame

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
        else {              // Add byte to frame
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
