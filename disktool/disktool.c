/*!
 * \file disktool.c
 * \author Peter C. Chapin <PChapin@vtc.vsc.edu>
 *
 * \brief This program can be used to manually create a generic file system on a partition and
 * for inspecting GenericFS data structures. It is useful for testing the GenericFS kernel
 * module.
 */

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include <curses.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "tool.h"

static void compute_sizes(long block_count)
{
  // Assume one inode for every 4 KBytes of disk space. Thus inode count and block count are the
  // same. This causes the block free map and the inode free map to be the same size. The code
  // below assumes disk inodes are 64 bytes in size.
  //
  freemap_bytesize = block_count / 8;
  if (block_count % 8 != 0) ++freemap_bytesize;
  freemap_blocksize = freemap_bytesize / BLOCKSIZE;
  if (freemap_bytesize % BLOCKSIZE != 0) ++freemap_blocksize;

  inodetable_bytesize = block_count * 64;
  inodetable_blocksize = inodetable_bytesize / BLOCKSIZE;
  if (inodetable_bytesize % BLOCKSIZE != 0) ++inodetable_blocksize;
}


static int menu(void)
{
  // Define the menu.
  static const char * const menu_options[] = {
    "0: Exit",
    "1: Initialize partition",
    "2: Show superblock",
    "3: Show inode map",
    "4: Show block map",
    "5: Show inode",
    "6: Show block",
    "7: Show root directory",
    "8: Show file",
    "9: Create file",
    "A: Create directory",
    "B: Verify file system",
    NULL
  };

  int choice;
  int invalid;
  int row = 7;
  uint32_t available_blocks;
  const char * const *p = menu_options;

  clear();
  mvprintw(2, 1,
    "Partition  : %u bytes (%u blocks)", block_count * BLOCKSIZE, block_count);
  mvprintw(3, 1,
    "Free Map   : %u bytes (%u blocks)", freemap_bytesize, freemap_blocksize);
  mvprintw(4, 1,
    "Inode Table: %u bytes (%u blocks)", inodetable_bytesize, inodetable_blocksize);
  available_blocks = block_count - 2*freemap_blocksize - inodetable_blocksize;
  mvprintw(5, 1, 
    "Available  : %u bytes (%u blocks)", available_blocks * BLOCKSIZE, available_blocks);

  // Display the menu.
  while (*p) {
    mvaddstr(row, 1, *p);
    ++row;
    ++p;
  }

  // Get the user's selection.
  row += 2;
  mvaddstr(row, 1, "Enter choice: ");
  do {
    invalid = 0;
    refresh();
    choice = toupper(getch());
    if (isdigit(choice)) {
      choice = choice - '0';
    }
    else if (choice == 'A') {
      choice = 10;
    }
    else if (choice == 'B') {
      choice = 11;
    }
    else {
      mvaddstr(row + 1, 1, "Invalid choice. Select again!");
      invalid = 1;
    }
  } while (invalid);
  return choice;
}


static void show_super(int fd)
{
  unsigned char workspace[BLOCKSIZE];
  struct gfs_super_block *my_super = (struct gfs_super_block *)workspace;

  clear();
  lseek(fd, 0, SEEK_SET);
  read(fd, workspace, BLOCKSIZE);

  mvprintw(2, 1, "Magic Number:       0x%X", my_super->magic_number);
  mvprintw(3, 1, "Total Blocks:       %d"  , my_super->total_blocks);
  mvprintw(4, 1, "Block Size:         %d bytes" , my_super->block_size);
  mvprintw(5, 1, "Inode Freemap Size: %d blocks", my_super->inodefreemap_blocks);
  mvprintw(6, 1, "Block Freemap Size: %d blocks", my_super->blockfreemap_blocks);
  mvprintw(7, 1, "Inode Table Size:   %d blocks", my_super->inodetable_blocks);
  CONTINUE_MESSAGE;
}

// The check_super function takes a filehandle for the device and examines it for a correct
// magic number of 0xDEADBEEF. This function should probably be extended to do more extensive
// sanity checking.
//
static int check_super(int fd)
{
  unsigned char workspace[BLOCKSIZE];
  struct gfs_super_block *my_super = (struct gfs_super_block *)workspace;

  lseek(fd, 0, SEEK_SET);
  read(fd, workspace, BLOCKSIZE);

  return (my_super->magic_number == 0xDEADBEEF ? 1 : 0);
}


int main(int argc, char **argv)
{
  int     fd;           // File descriptor of open device file.
  long    size;         // Size of partition in sectors.
  int     choice;       // Item selected from menu.
  struct stat file_information;  // Used when finding partition size.

  // The jump table contains a function for each menu option.
  static void (*jump_table[])(int) = {
    NULL,
    initialize, 
    show_super,
    show_inode_freemap,
    show_block_freemap,
    show_inode,
    show_block,
    show_root_dir,
    show_file,
    create_file,
    create_dir,
    verify_file_system
  };

  printf("GenericFS Disk Tool, v0.2, Compiled: %s at %s\n\n", __DATE__, __TIME__);

  // Make sure I got the parameter I want.
  if (argc != 2) {
    printf("Expected a partition name on the command line.\n");
    return 1;
  }

  // Open the device file.
  fd = open(argv[1], O_RDWR);
  if (fd == -1) {
    printf("Can't open %s for read/write.\n", argv[1]);
    return 1;
  } 

  // Verify that we have a GenericFS file system.
  if (!check_super(fd)){
    printf("Warning: %s does not have a valid GenericFS signature, continue? ", argv[1]);
    if (getchar() != 'y') {
      printf("Exiting\n");
      return 1;
    }
    while (getchar() != '\n') ;
  }

  // Determine the partition size in blocks.
  if (ioctl(fd, BLKGETSIZE, &size) >= 0) {
    block_count = size / (BLOCKSIZE / 512);
  }
  else if (fstat(fd, &file_information) == 0) {
    block_count = file_information.st_size / BLOCKSIZE;
  }
  else {
    printf("Can't figure out partition size!\n");
    return 1;
  }

  // Figure out how big things are.
  compute_sizes(block_count);

  // Initialize curses.
  initscr(); cbreak(); noecho(); nonl();

  // Display the menu to the user and let him/her select menu items.
  while (1) {
    choice = menu();
    if (choice == 0) break;
    jump_table[choice](fd);
  }

  // Clean up curses.
  endwin();

  // Close the file.
  close(fd);
  return 0;
}
