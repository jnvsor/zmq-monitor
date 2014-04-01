#include <linux/input.h>

typedef struct { char map[KEY_MAX/8+1]; } keymap;

typedef struct {
  char * keydown;
  char * keyup;
  int keycombo[6]; // Fixed length arrays, because fuck linked lists for something this simple
  int keyup_delay;
  int waited;
  int previous;
} shortcut;
