/*!
 * \file showfile.c
 * \author Peter C. Chapin <PChapin@vtc.vsc.edu>
 *
 * \brief Function to display the contents of a file.
 */

#include <curses.h>
#include <unistd.h>

#include "tool.h"

void show_file(int fd)
{ 
  clear();
  mvaddstr(1, 1, "Not implemented");
  CONTINUE_MESSAGE;
}
