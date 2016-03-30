/*!
 * \file showfreemaps.c
 * \author Peter C. Chapin <PChapin@vtc.vsc.edu>
 *
 * \brief Functions to diplay the block and inode free maps.
 *
 * \todo The two functions in this file are very similar. Probably their commonality could be
 * factored out and implemented using a single function. This would make it easier to update
 * both freemap displays consistently.
 *
 * \todo A feature should be added that would allow the user to jump to an arbitrary location in
 * the freemaps.
 *
 * \todo It should be possible to scroll both backwards and forwards in the freemaps.
 *
 * \todo It should be possible to edit the freemaps here so that errors in their structure can
 * be fixed (or introduced for purposes of testing).
 */

#include <curses.h>
#include <unistd.h>

#include "tool.h"


//
// The following function displays the inode freemap in a "nice" way.
//
void show_inode_freemap(int fd)
{
  unsigned char workspace[BLOCKSIZE];
  struct gfs_super_block *my_super = (struct gfs_super_block *)workspace;

  unsigned int total_inodes;  // The total number of inodes on the disk.
  unsigned int inode_counter = 0; // Number of inode bits displayed so far.
  unsigned int map_blocks;    // The number of blocks in the inode freemap.
  unsigned int block_index;   // Index of current block in freemap.
  int block_offset;           // Index of current byte in current block.
  int done = 0;               // =1 when the user doesn't want any more.
  int i;
  int row = 1;

  clear();
  
  // Read the superblock to learn how large the inode freemap is.
  lseek(fd, 0, SEEK_SET);
  read(fd, workspace, BLOCKSIZE);
  total_inodes = my_super->total_blocks;
  map_blocks = my_super->inodefreemap_blocks;

  // Position the device at the start of the freemap.
  lseek(fd, BLOCKSIZE, SEEK_SET);

  // Loop for all blocks in the freemap.
  for (block_index = 0; block_index < map_blocks && !done; block_index++) {

    // For each block, read the block and loop over all bytes in the block.
    read(fd, workspace, BLOCKSIZE);
    for (block_offset = 0; block_offset < BLOCKSIZE && !done; block_offset++) {

      // Pause the display and give the user a chance to quit now.
      if (inode_counter % (32*(LINES-2)) == 0) {
        mvaddstr(LINES-1, 1, "Press Enter to continue; 'q' to quit... ");
        refresh();
        row = 1;
        if (getch() == 'q') {
          done = 1;
          clear();
          break;
        }
        clear();
      }

      // Display a line header if necessary.
      if (block_offset % 4 == 0) {
        mvprintw(row++, 1, "  %09d: ", inode_counter);
      }
      else {
        printw("  ");
      }

      // Display allocation information stored in the current byte.
      for (i = 0; i < 8; i++) {
        if (workspace[block_offset] & (0x1 << i))
          printw("X");
        else
          printw("-");

        if (++inode_counter == total_inodes) { done = 1; break; }
      }
    }
  }
  CONTINUE_MESSAGE;
}


//
// The following function displays the block freemap in a "nice" way.
//
void show_block_freemap(int fd)
{
  unsigned char workspace[BLOCKSIZE];
  struct gfs_super_block *my_super = (struct gfs_super_block *)workspace;

  unsigned int total_blocks;  // The total number of blocks on the disk.
  unsigned int block_counter = 0; // Number of block bits displayed so far.
  unsigned int map_blocks;    // The number of blocks in the block freemap.
  unsigned int block_index;   // Index of current block in freemap.
  int block_offset;           // Index of current byte in current block.
  int done = 0;               // =1 when the user doesn't want any more.
  int i;
  int row = 1;

  clear();
  
  // Read the superblock to learn how large the block freemap is.
  lseek(fd, 0, SEEK_SET);
  read(fd, workspace, BLOCKSIZE);
  total_blocks = my_super->total_blocks;
  map_blocks = my_super->blockfreemap_blocks;

  // Position the device at the start of the freemap.
  lseek(fd, BLOCKSIZE*(1 + my_super->inodefreemap_blocks), SEEK_SET);

  // Loop for all blocks in the freemap.
  for (block_index = 0; block_index < map_blocks && !done; block_index++) {

    // For each block, read the block and loop over all bytes in the block.
    read(fd, workspace, BLOCKSIZE);
    for (block_offset = 0; block_offset < BLOCKSIZE && !done; block_offset++) {

      // Pause the display and give the user a chance to quit now.
      if (block_counter % (32*(LINES-2)) == 0) {
        mvaddstr(LINES-1, 1, "Press Enter to continue; 'q' to quit... ");
        refresh();
        row = 1;
        if(getch() == 'q') {
          done = 1;
          clear();
          break;
        }
        clear();
      }

      // Display a line header if necessary.
      if (block_offset % 4 == 0) {
        mvprintw(row++, 1, "  %09d: ", block_counter);
      }
      else {
        printw("  ");
      }

      // Display allocation information stored in the current byte.
      for (i = 0; i < 8; i++) {
        if (workspace[block_offset] & (0x1 << i))
          printw("X");
        else
          printw("-");

        if (++block_counter == total_blocks) { done = 1; break; }
      }
    }
  }
  CONTINUE_MESSAGE;
}
