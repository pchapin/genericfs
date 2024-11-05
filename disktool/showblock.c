/*!
 * \file showblock.c
 * \author Peter Chapin <spicacality@kelseymountain.org>
 *
 * \brief Function to display a single block.
 *
 * \todo It would be nice to have some editing features (scrolling forwards and backwards...
 * ability to modify any byte... etc).
 *
 * \todo It should be possible to look at another block without backing out to the main menu.
 */

#include <ctype.h>
#include <time.h>

#include <curses.h>
#include <unistd.h>
#include <sys/stat.h>

#include "tool.h"

void show_block( int fd )
{
    unsigned char workspace[BLOCKSIZE];
    unsigned int choice;   // The block number to display.
    unsigned int offset;   // Offset into the block.
    unsigned int line_offset;  // Offset into the current line.
    unsigned int total;    // Total number of blocks.
    char printables[16+1]; // Printable version of the block's bytes.
    int row = 1;

    clear( );
  
    struct gfs_super_block *my_super = (struct gfs_super_block *)workspace;

    lseek( fd, 0, SEEK_SET );
    read( fd, workspace, BLOCKSIZE );

    total = my_super->total_blocks;
    printables[16] = '\0';   // Be sure this array is null terminated.

    // Ask the user for a block number.
    mvprintw( 1, 1, "Enter block number (0 - %u): ", total - 1 );
    echo( ); scanw( "%u", &choice ); noecho( );

    clear( );

    // Do some error handling. (What an idea!)
    if( choice > total - 1 ) {
        mvprintw( 1, 1, "Error: Block %u out of range. Maximum = %u\n", choice, total );
        CONTINUE_MESSAGE;
    }

    // Read the block in question.
    lseek( fd, choice * BLOCKSIZE, SEEK_SET );
    read( fd, workspace, BLOCKSIZE );
    
    // Loop over the entire block, printing bytes as we go.
    for( offset = 0; offset < BLOCKSIZE; offset += 16 ) {

        // Pause the display and give the user a chance to quit now.
        if( offset % ( 16 * ( LINES - 2 ) ) == 0 ) {
            mvprintw( LINES-1, 1, "Press Enter to continue; 'q' to quit... " );
            refresh( );
            row = 1;
            if( getch( ) == 'q' ) {
                clear( );
                break;
            }
            clear( );
        }

        // Print a line header.
        mvprintw( row++, 1, "%03X:", offset );
    
        for( line_offset = 0; line_offset < 16; line_offset++ ) {

            // Put the printable version of this byte into the printables array.
            if( isprint( workspace[offset + line_offset] ) ) {
                printables[line_offset] = workspace[offset + line_offset];
            }
            else {
                printables[line_offset] = '.';
            }

            // Display the hex version of the bytes.
            if( line_offset % 8 == 0 ) printw( " " );
            printw( "%02X ", (unsigned char)workspace[offset + line_offset] );
        }

        // Display the ASCII version of the bytes.
        printw( "|%s|\n", printables );
    }

    CONTINUE_MESSAGE;
}
