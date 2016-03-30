/*!
 * \file initialize.c
 * \author Peter C. Chapin <PChapin@vtc.vsc.edu>
 *
 * \brief Functions that initialize a partition with GenericFS.
 */

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <curses.h>
#include <unistd.h>
#include <sys/stat.h>

#include "tool.h"

#define UNUSED_SPACE 0x55

/*! Erase every byte on the partition.
 *
 * This function overwrites every byte on the partition with UNUSED_SPACE. The use of this
 * arbitrary value makes it easier to recognize changes in raw disk dumps.
 *
 * Although erasing the entire partition can take some time it is useful for debugging and
 * testing purposes. Any left over data on a partition from earlier tests is thrown away when
 * the partition is re-initialized.
 */
static void clear_partition( int fd )
{
    int i;
    unsigned char workspace[BLOCKSIZE];

    mvaddstr( 1, 1, "Clearing partition..." ); refresh( );

    lseek( fd, 0, SEEK_SET );
    memset( workspace, UNUSED_SPACE, BLOCKSIZE );
    for( i = 0; i < block_count; ++i ) {
        write( fd, workspace, BLOCKSIZE );
    }
}


static void write_freemaps(int fd)
{
  long i;
  long total_preallocated;
  long leftovers;
  int  mask;
  unsigned char workspace[BLOCKSIZE];

  mvaddstr(2, 1, "Writing free maps..."); refresh();

  // Skip over the super block.
  lseek(fd, 1*BLOCKSIZE, SEEK_SET);

  // Zero bits in the free maps mean "not allocated". Set bit zero in the first entry of the
  // inode freemap to account for the allocation of inode zero to the root directory.
  // 
  memset(workspace, 0, BLOCKSIZE);
  workspace[0] |= 0x01;
  write(fd, workspace, BLOCKSIZE);
  memset(workspace, 0, BLOCKSIZE);
  for (i = 0; i < freemap_blocksize - 1; ++i) {
    write(fd, workspace, BLOCKSIZE);
  }

  // Now set bits in the block freemap to account for the various file system datastructures I
  // have preallocated.
  // 
  total_preallocated = 1 + 2*freemap_blocksize + inodetable_blocksize + 1;
    // One for the superblock and one for the root directory.

  // I'm not prepared to deal with the case where the preallocated blocks require more than one
  // block of free map space.
  // 
  if (total_preallocated > BLOCKSIZE*8) {
    mvaddstr(2, 1, "There are more preallocated blocks than I can handle!");
    CONTINUE_MESSAGE;
    exit(1);
  }
  memset(workspace, 0xFF, total_preallocated/8);
  leftovers = total_preallocated % 8;
  mask = 0x01;
  while (leftovers > 0) {
    workspace[total_preallocated/8] |= mask;
    mask <<= 1;
    --leftovers;
  }
  write(fd, workspace, BLOCKSIZE);
  memset(workspace, 0, BLOCKSIZE);
  for (i = 0; i < freemap_blocksize - 1; ++i) {
    write(fd, workspace, BLOCKSIZE);
  }
}


static void create_root(int fd)
{
  unsigned char workspace[BLOCKSIZE];
  struct gfs_inode *root_node = (struct gfs_inode *)workspace;
  time_t now = time(NULL);

  mvaddstr(3, 1, "Creating root directory..."); refresh();

  // Fill in the root directory's inode;
  memset(workspace, UNUSED_SPACE, BLOCKSIZE);
  root_node->nlinks = 2;
  root_node->owner_id = 0;
  root_node->group_id = 0;
  root_node->mode = S_IFDIR|S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;
  root_node->file_size = BLOCKSIZE;
  root_node->atime = now;
  root_node->mtime = now;
  root_node->ctime = now;
  root_node->blocks[0] = 1 + 2*freemap_blocksize + inodetable_blocksize;
  root_node->blocks[1] = 0;
  root_node->blocks[2] = 0;
  root_node->blocks[3] = 0;
  root_node->first_indirect  = 0;
  root_node->second_indirect = 0;
  root_node->unused[0] = 0;
  root_node->unused[1] = 0;

  // Now write the block containing this inode. (The other inodes in this block are irrelevant
  // since they are all unallocated).
  // 
  lseek(fd, (1 + 2*freemap_blocksize)*BLOCKSIZE, SEEK_SET);
  write(fd, workspace, BLOCKSIZE);


  // Now use the workspace to create the root directory block itself.
  memset(workspace, UNUSED_SPACE, BLOCKSIZE);
  *(((unsigned int *)workspace) + 0) = 10;
  *(((unsigned int *)workspace) + 1) = 0;
  *(workspace + 8) = 1;
  *(workspace + 9) = '.';
  *(((unsigned int *)(workspace + 10)) + 0) = 0;
  *(((unsigned int *)(workspace + 10)) + 1) = 0;
  *(workspace + 18) = 2;
  *(workspace + 19) = '.';
  *(workspace + 20) = '.';

  // Now write it out.
  lseek(
    fd, (1 + 2*freemap_blocksize + inodetable_blocksize)*BLOCKSIZE, SEEK_SET);
  write(fd, workspace, BLOCKSIZE);
}


static void write_super(int fd)
{
  unsigned char workspace[BLOCKSIZE];
  struct gfs_super_block *my_super = (struct gfs_super_block *)workspace;

  mvaddstr(4, 1, "Writing super block..."); refresh();

  memset(workspace, UNUSED_SPACE, BLOCKSIZE);
  my_super->magic_number = 0xDEADBEEF;
  my_super->block_size   = BLOCKSIZE;
  my_super->total_blocks = block_count;
  my_super->inodefreemap_blocks = freemap_blocksize;
  my_super->blockfreemap_blocks = freemap_blocksize;
  my_super->inodetable_blocks   = inodetable_blocksize;

  lseek(fd, 0, SEEK_SET);
  write(fd, workspace, BLOCKSIZE);
}


void initialize( int fd )
{
    clear( );
    refresh( );
    clear_partition( fd );
    write_freemaps( fd );
    create_root( fd );
    write_super( fd );
    CONTINUE_MESSAGE;
}
