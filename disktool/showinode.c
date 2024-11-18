/*!
 * \file showinode.c
 * \author Peter Chapin <spicacality@kelseymountain.org>
 *
 * \brief Function to display an inode.
 */

#include <time.h>

#include <curses.h>
#include <unistd.h>
#include <sys/stat.h>

#include "tool.h"

void show_inode( int fd )
{
    unsigned char workspace[BLOCKSIZE];
    int choice;
    int blocknum;
    int offset;
    int total;
    int freemap_size;
    time_t temptime;

    clear( );
  
    struct gfs_inode *my_inode;
    struct gfs_super_block *my_super = (struct gfs_super_block *)workspace;

    lseek( fd, 0, SEEK_SET );
    read( fd, workspace, BLOCKSIZE );  

    total = my_super->total_blocks;
    freemap_size = my_super->inodefreemap_blocks;
  
    mvprintw( 1, 1, "Enter inode (0 - %d): ", total - 1 );
    echo( ); scanw( "%d", &choice ); noecho( );

    clear( );

    blocknum = choice / 64;
    offset = (choice % 64) * 64;
  
    // Add 1 for the superblock and 2 times the freemap size (one freemap for the inodes and one
    // for the blocks).
    // 
    blocknum = blocknum + 1 + ( freemap_size * 2 );
    // printf( "\n%d, %d\n", blocknum, offset );

    lseek( fd, blocknum * BLOCKSIZE, SEEK_SET );
    read( fd, workspace, BLOCKSIZE );

    my_inode = (struct gfs_inode *)( workspace + offset );

    mvprintw( 1, 1, "nlinks        : %d\n", my_inode->nlinks );
    mvprintw( 2, 1, "Owner Id      : %d\n", my_inode->owner_id );
    mvprintw( 3, 1, "Group Id      : %d\n", my_inode->group_id );
    mvprintw( 4, 1, "Mode          : %o: ", my_inode->mode );
  
    if     ( S_ISSOCK( my_inode->mode ) ) printw( "socket" );
    else if( S_ISLNK ( my_inode->mode ) ) printw( "link" );
    else if( S_ISREG ( my_inode->mode ) ) printw( "regular" );
    else if( S_ISDIR ( my_inode->mode ) ) printw( "directory" );
    else if( S_ISCHR ( my_inode->mode ) ) printw( "character device" );
    else if( S_ISBLK ( my_inode->mode ) ) printw( "block device" );
    else if( S_ISFIFO( my_inode->mode ) ) printw( "fifo" );

    if( my_inode->mode & S_ISUID ) printw( ", UID bit set" );
    if( my_inode->mode & S_ISGID ) printw( ", GID bit set" );
    if( my_inode->mode & S_ISVTX ) printw( ", sticky bit set" );

    mvprintw( 5, 1, "File Size     : %d\n", my_inode->file_size );
    temptime = my_inode->atime;
    mvprintw( 6, 1, "Access Time   : %s", ctime( &temptime ) );
    temptime = my_inode->mtime;
    mvprintw( 7, 1, "Modified Time : %s", ctime( &temptime ) );
    temptime = my_inode->ctime;
    mvprintw( 8, 1, "Meta Mod Time : %s", ctime( &temptime ) );
    mvprintw( 9, 1, "First Blocks  : %d, %d, %d, %d\n",
             my_inode->blocks[0], my_inode->blocks[1], 
             my_inode->blocks[2], my_inode->blocks[3]);
    mvprintw( 10, 1, "First Indirection Pointer : %d\n",my_inode->first_indirect );
    mvprintw( 11, 1, "Second Indirection Pointer: %d\n",my_inode->second_indirect );

    CONTINUE_MESSAGE;
}
