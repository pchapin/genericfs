/*!
 * \file showrootdir.c
 * \author Peter Chapin <spicacality@kelseymountain.org>
 *
 * \brief Function to display root directory details.
 */

#include <curses.h>
#include <unistd.h>

#include "tool.h"

void show_root_dir( int fd )
{ 
    unsigned char workspace[BLOCKSIZE];
    char size;
    int i;
    int row;
    unsigned int local_freemap_blocksize;
    unsigned int local_inodetable_blocksize;
    uint8_t *root_block;

    clear( );
    
    struct gfs_super_block *my_super = (struct gfs_super_block *)workspace;

    lseek( fd, 0, SEEK_SET );
    read( fd, workspace, BLOCKSIZE );

    local_freemap_blocksize = my_super->blockfreemap_blocks;
    local_inodetable_blocksize = my_super->inodetable_blocks;
     
    lseek( fd, ( 1 + 2 * local_freemap_blocksize + local_inodetable_blocksize ) * BLOCKSIZE, SEEK_SET );
    read( fd, workspace, BLOCKSIZE );
    root_block = workspace;

    mvprintw( 1, 1, "Root directory in block #%d",
             1 + 2 * local_freemap_blocksize + local_inodetable_blocksize );
    mvprintw( 2, 1, "%10s %10s %10s %10s\n",
             "Offset", "Next", "Inode", "Filename" );
    mvprintw( 3, 1, "========== ========== ========== ==========" );

    row = 4;
    while( 1 ) {

        // Print the offset.
        mvprintw( row, 1, "%10ld ", root_block - workspace );

        // Print the next offset.
        printw( "%10d ", ( *(unsigned int *)root_block ) );

        // Print I-node number.
        printw( "%10d ", ( *(unsigned int *)(root_block + 4)));
        size = *( root_block + 8 );
    
        // Print filename.
        for( i = 0; i < size; i++ )
            printw( "%c", *( root_block + i + 9 ) );          
	
        if( *(unsigned int *)root_block == 0 ) break;

        root_block = workspace + ( *(unsigned int*)root_block );
        ++row;
    }

    CONTINUE_MESSAGE;
}
