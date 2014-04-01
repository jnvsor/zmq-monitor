#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <zmq.h>
#include <linux/input.h>

typedef struct { char map[KEY_MAX/8+1]; } keymap;

static inline int ispressed (keymap state, int key );

/* #defines, and getptt() can be edited for config purposes. The filter ID
 * can be changed below in main() at the sendmessage() calls */
#define KEYUP_DELAY 30
#define POLL_DELAY 0.01

static inline int getptt(keymap state){
  return ispressed(state, KEY_LEFTALT) && ispressed(state, KEY_LEFTCTRL);
}

static inline int ispressed(keymap state, int key){
  return state.map[key/8] & (1 << (key % 8) );
}

keymap getstate(FILE * kbd){
  keymap state;
  memset(&state, 0, sizeof(state));
  ioctl(fileno(kbd), EVIOCGKEY(sizeof(state)), &state);
  return state;
}

int sendmessage(char * message, void * socket){
  if(zmq_send(socket, message, strlen(message), 0) == -1){
    goto error;
  }
  
  printf("Sent\t\t: %s\n", message);
  char reply[11];
  
  if(zmq_recv(socket, reply, 10, 0) == -1){
    goto error;
  }
  
  reply[10] = 0;
  printf("Receieved\t: %s\n", reply);
  return 0;

error:
  printf("Error\t\t: %s\n", zmq_strerror(errno));
  return errno;
}

void * newsock(void * context){
  void * socket = zmq_socket(context, ZMQ_REQ);
  int recv_timeout = 100, socket_timeout = 1000;
  zmq_setsockopt(socket, ZMQ_RCVTIMEO, (const void *)&recv_timeout, 4);
  zmq_setsockopt(socket, ZMQ_LINGER, (const void *)&socket_timeout, 4);
  zmq_connect(socket, "tcp://localhost:5555");
  return socket;
}

int main(int argc, char * argv[]){
  if(argc != 4){
    printf("Error: Expected 3 arguments, got %d\n", argc-1);
    return 1;
  }
  
  char * keyboard = argv[1], * keydown = argv[2], * keyup = argv[3];
  
  FILE * kbd = fopen(keyboard, "r");
  if(!kbd){
    printf("Error: No permissions to open %s\n", keyboard);
    return 1;
  }
  
  void * context = zmq_ctx_new();
  void * socket = newsock(context);
  
  struct timespec sleep = { 0, POLL_DELAY  * 1000000000L };
  int ptt = 0, ptt_prev = 0, ptt_wait = 0, rc=0;
  
  do{
    ptt = getptt(getstate(kbd));
    
    if(ptt != ptt_prev){
      if(ptt){
        if(ptt_wait){
          // If pressed and waiting after keyup, stop waiting
          ptt_wait=0;
        }
        else {
          // Otherwise just send signal
          rc = sendmessage(keydown, socket);
          if(rc == 11){
            zmq_close(socket);
            socket = newsock(context);
          }
          else if(rc) {
            goto exit;
          }
      }
    }
      else {
        // Start waiting for a new keydown
        ptt_wait = KEYUP_DELAY;
      }
    }
    
    // Decrement wait to 0 then send the signal
    if(ptt_wait && --ptt_wait <= 0){
      rc = sendmessage(keyup, socket);
      if(rc == 11){
        zmq_close(socket);
        socket = newsock(context);
      }
      else if(rc) {
        goto exit;
      }
    }
    
    ptt_prev=ptt;
    nanosleep(&sleep, NULL);
  }while(1);
  
exit:
  printf("Errno: %d\n", errno);
  zmq_close (socket);
  zmq_ctx_destroy (context);
  return 0;
}
