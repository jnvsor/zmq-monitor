#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <zmq.h>
#include <linux/input.h>

#include "zmq-monitor.h"

static inline int ispressed(keymap * state, int key){
  return state->map[key/8] & (1 << (key % 8) );
}

int getactive(keymap * state, shortcut * sc){
  int i = 0;
  while(i < 6){
    if(sc->keycombo[i] == -1){
      break;
    }
    if(!ispressed(state, sc->keycombo[i++])){
      return 0;
    }
  }
  return 1;
}

shortcut newsc(char * argv[]){
  int keycount = 0;
  shortcut sc;
  char * cursor = strtok(argv[0], " ,+");
  
  while(cursor != NULL){
    sc.keycombo[keycount++] = atoi(cursor);
    
    cursor = strtok(NULL, " ,+");
    
    if(keycount > 6){
      break;
    }
    else if(cursor == NULL){
      sc.keycombo[keycount] = -1;
    }
  }
  
  sc.keydown = argv[1];
  sc.keyup = argv[2];
  sc.keyup_delay = atoi(argv[3]);
  sc.waited = -1;
  sc.previous = 0;
  
  return sc;
}

keymap getstate(FILE * kbd){
  keymap state;
  memset(&state, 0, sizeof(state));
  ioctl(fileno(kbd), EVIOCGKEY(sizeof(state)), &state);
  return state;
}

int sendmessage(char * message, void * socket){
  if(message[0] == '\0'){
    printf("Sent no message\n");
    return 0;
  }
  
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
  if(argc < 6 || (argc-2)%4){
    printf("Error: Expected 1 argument plus 4 per shortcut, got %d\n", argc-1);
    return 1;
  }
  
  FILE * kbd = fopen(argv[1], "r");
  if(!kbd){
    printf("Error: No permissions to open %s\n", argv[1]);
    return 1;
  }
  
  int shortcut_count = (argc-2)/4, i=0;
  shortcut shortcuts[shortcut_count];
  for(i = 0; i < shortcut_count; i++){
    shortcuts[i] = newsc(&argv[2+i*4]);
  }
  
  void * context = zmq_ctx_new();
  void * socket = newsock(context);
  
  struct timespec sleep = { 0, 10000000L };
  int rc=0, thisactive=0;
  keymap state;
  shortcut * thissc;
  
  do{
    state = getstate(kbd);
    
    // Check keydowns
    for(i = 0; i < shortcut_count; i++){
      thissc = &shortcuts[i];
      thisactive = getactive(&state, thissc);
      
      // If pressed and waiting after keyup, stop waiting
      if(thissc->previous != thisactive){
        if(thisactive){
          if(thissc->waited > -1){
            thissc->waited--;
          }
          else {
            rc = sendmessage(thissc->keydown, socket);
            if(rc == 11){
              zmq_close (socket);
              socket = newsock(context);
            }
            else if(rc){
              goto exit;
            }
          }
        }
        else {
          thissc->waited = thissc->keyup_delay;
        }
        thissc->previous = thisactive;
      }
      
      if(thissc->waited > -1 && !--thissc->waited){
        rc = sendmessage(thissc->keyup, socket);
        if(rc == 11){
          zmq_close (socket);
          socket = newsock(context);
        }
        else if(rc){
          goto exit;
        }
        thissc->waited = -1;
      }
    }
    nanosleep(&sleep, NULL);
  }while(1);
  
exit:
  printf("Errno: %d\n", errno);
  zmq_close (socket);
  zmq_ctx_destroy (context);
  return 0;
}
