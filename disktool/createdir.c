/*!
 * \file createdir.c
 * \author Peter C. Chapin <PChapin@vtc.vsc.edu>
 *
 * \brief Functions for creating a subdirectory in the root directory.
 */

#include <curses.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>

#include "tool.h"

// This function is due to Daron Hume
//
// This funtion now works... sort of. it only creates /mydir. if you run it over and over it
// could mess things up, so don't do that.
//
void create_dir(int fd)
{
  unsigned char workspace[BLOCKSIZE];
  struct gfs_inode *parent_node;
  struct gfs_inode *current_node;
  time_t now = time(NULL);
  unsigned current_inode;
  unsigned parent_inode;
  unsigned current_block;
  unsigned pblock;
  unsigned char *offset;
  unsigned char *parent_block;

  clear();
  mvaddstr(1, 1, "Not implemented");
  CONTINUE_MESSAGE;
  return;

  // FIX THIS for non-root parents maybe probably not
  parent_inode = 0;

  // FIX THIS so non-root parents can be used... maybe get parent dir
  lseek(fd, (1 + 2*freemap_blocksize)*BLOCKSIZE + 64*parent_inode, SEEK_SET);
  read(fd, workspace, 64);
  parent_node = (struct gfs_inode *)workspace;

  // new dir's '..' links to parent
  parent_node->nlinks += 1;

  // write changed parent
  lseek(fd, (1 + 2*freemap_blocksize)*BLOCKSIZE + 64*parent_inode, SEEK_SET);
  write(fd, workspace, 64);

  // huh?
  pblock = parent_node->blocks[0];

  // get parent's block so we can change it (always block 0?)
  lseek(fd, (pblock)*BLOCKSIZE, SEEK_SET);
  read(fd, workspace, BLOCKSIZE);

  parent_block = workspace;

  // THIS IS BROKEN... i think
  // look thru dir entries offset things
  while (*(unsigned int *)parent_block != 0)
    parent_block = workspace + (*(unsigned int *)parent_block);

  // FIX THIS... or not
  // crappy way to do this... only works if last dir entry is at the end
  offset = parent_block + 4 + 4;  // skip past offset + inode #
  offset += (*offset) + 1;        // skip past size + name

  // set old last entry offset thing to point to the new one... yeah
  (*(unsigned int *)parent_block) = (unsigned int)(offset - workspace);

  // new entry's offset to next is zero
  *(((unsigned int *)offset) + 0) = 0;

  // find first free inode.
  current_inode = allocate_inode(fd);

  // new entry's inode
  *(((unsigned int *)offset) + 1) = current_inode;

  // FIX THIS... like, the whole function and stuff so it can be different
  // new entry's size + name
  *(offset + 8) = 5;
  *(offset +  9) = 'm';
  *(offset + 10) = 'y';
  *(offset + 11) = 'd';
  *(offset + 12) = 'i';
  *(offset + 13) = 'r';

  // write the changed parent dir info back
  lseek(fd, pblock*BLOCKSIZE, SEEK_SET);
  write(fd, workspace, BLOCKSIZE);

  // debug
  // printf("inode stuff: %d %d %d; %d\n", block, byte, bit, current_inode);

  // mess with our current node
  current_node = (struct gfs_inode *)workspace;

  // find first free block.
  current_block = allocate_block(fd);

  // Fill in the current directory's inode;
  current_node->nlinks = 2;
  current_node->owner_id = 0;
  current_node->group_id = 0;
  current_node->mode = S_IFDIR|S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;
  current_node->file_size = BLOCKSIZE;
  current_node->atime = now;
  current_node->mtime = now;
  current_node->ctime = now;
  current_node->blocks[0] = current_block;
  current_node->blocks[1] = 0;
  current_node->blocks[2] = 0;
  current_node->blocks[3] = 0;
  current_node->first_indirect  = 0;
  current_node->second_indirect = 0;
  current_node->unused[0] = 0;
  current_node->unused[1] = 0;

  lseek(fd, (1 + 2*freemap_blocksize)*BLOCKSIZE + 64*current_inode, SEEK_SET);
  write(fd, workspace, 64);

  // Now use the workspace to create the current directory block itself.
  *(((unsigned int *)workspace) + 0) = 10;
  *(((unsigned int *)workspace) + 1) = current_inode;
  *(workspace + 8) = 1;
  *(workspace + 9) = '.';
  *(((unsigned int *)(workspace + 10)) + 0) = 0;
  *(((unsigned int *)(workspace + 10)) + 1) = parent_inode;
  *(workspace + 18) = 2;
  *(workspace + 19) = '.';
  *(workspace + 20) = '.';

  // Now write it out.
  lseek(fd, current_block*BLOCKSIZE, SEEK_SET);
  write(fd, workspace, BLOCKSIZE);

  CONTINUE_MESSAGE;
}
