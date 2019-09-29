#include<iostream>

#include<stdio.h>
#include<stdlib.h>
#include<sys/time.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>

#include<linux/input.h>

using namespace std;

int main( int argc, char *argv[] )
{
  fd_set         rfds;
  struct timeval tv;
  int            retval;
  int            ctrl_pressed  = 0; /* 0: release 1: press */
  int            shift_pressed = 0; /* 0: release 1: press */
  int            alt_pressed   = 0; /* 0: release 1: press */

  int fd = open( "/dev/input/event2", O_RDONLY );

  while(1){
    FD_ZERO( &rfds );
    FD_SET( fd, &rfds );

    tv.tv_sec  = 10;
    tv.tv_usec = 0;

    retval = select( fd+1, &rfds, NULL, NULL, &tv );

    if( retval == -1 ){
      perror( "select()" );
    } else if ( retval ){
      struct input_event ev;

      if( read( fd, &ev, sizeof(ev) ) != sizeof(ev) ){
        exit( EXIT_FAILURE );
      }

      switch( ev.type ){
      case EV_KEY:
        switch( ev.code ){
        case KEY_RIGHTCTRL:
          if( ev.value > 0 ){
            ctrl_pressed = 1;
          } else {
            ctrl_pressed = 0;
          }
          cout << "��CTRL������������ޤ���." << endl;
          break;
        case KEY_RIGHTSHIFT:
          if( ev.value > 0 ){
            shift_pressed = 1;
          } else {
            shift_pressed = 0;
          }
          cout << "��SHIFT������������ޤ���." << endl;
          break;
        case KEY_RIGHTALT:
          if( ev.value > 0 ){
            alt_pressed = 1;
          } else {
            alt_pressed = 0;
          }
          cout << "��ALT������������ޤ���." << endl;
          break;
        case KEY_A:
          if( ctrl_pressed && shift_pressed && alt_pressed ){
            cout << "CTRL + SHIFT + ALT + A������������ޤ���." << endl;
          } else if( ctrl_pressed && shift_pressed ){
            cout << "CTRL + SHIFT + A������������ޤ���." << endl;
          } else if( ctrl_pressed && alt_pressed ){
            cout << "CTRL + ALT + A������������ޤ���." << endl;
          } else if( shift_pressed && alt_pressed ){
            cout << "ALT + SHIFT + A������������ޤ���." << endl;
          } else if( ctrl_pressed ){
            cout << "CTRL + A������������ޤ���." << endl;
          } else if( shift_pressed ){
            cout << "SHIFT + A������������ޤ���." << endl;
          } else if( alt_pressed ){
            cout << "ALT + A������������ޤ���." << endl;
          } else {
            cout << "A������������ޤ���." << endl;
          }
          break;
        default:
          break;
        }
        break;

      default:
        break;
      }
    }
  }

  return EXIT_SUCCESS;
}
