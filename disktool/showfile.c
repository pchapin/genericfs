/*!
 * \file showfile.c
 * \author Peter Chapin <spicacality@kelseymountain.org>
 *
 * \brief Function to display the contents of a file.
 */

#include <curses.h>
#include <unistd.h>

#include "tool.h"

void show_file( int fd )
{ 
    clear();
    mvaddstr( 1, 1, "Not implemented" );
    CONTINUE_MESSAGE;
}
