/*!
 * \file createfile.c
 * \author Peter C. Chapin <PChapin@vtc.vsc.edu>
 *
 * \brief Functions for creating a file in the root directory.
 *
 * \todo This code assumes that only one block wil be used for the root directory. That
 * restriction should be lifted.
 *
 * \todo The size of the created file is limited to 4 KiB. That limitation should be lifted.
 *
 * \todo The provided name should be checked. Very long names should be truncated or rejected.
 *
 * \todo A way should be provided to allow the user to copy the contents of an ordinary file (on
 * another file system) into a GenericFS file.
 */

#include <string.h>
#include <time.h>

#include <curses.h>
#include <unistd.h>
#include <sys/stat.h>

#include "tool.h"

// This function creates a file in the root directory. The int empty is just a 1 if the file is
// empty, or a 0 if it's not

void create_file(int fd)
{
  unsigned char workspace[BLOCKSIZE];
  char name[128];

  struct gfs_inode *parent_node;        // Inode of parent directory.
  struct gfs_inode *current_node;       // Inode of new file.
  time_t now = time(NULL);              // Current time.
  uint32_t file_size = 0;               // Size of new file.
  uint32_t current_inode;               // Inode number of new file.
  const uint32_t parent_inode = 0;      // Parent inode number.
  uint32_t current_block = 0;
  uint32_t pblock;                      // Block # where parent dir located.
  uint8_t *parent_block;                // Points into first block of parent.
  uint8_t *offset;                      // Points into first block of parent.

  clear();

  // Accept inputs.
  mvprintw(1, 1, "File name: ");
  echo(); scanw("%s", name); noecho();
  mvprintw(2, 1, "File size: ");
  echo(); scanw("%u", &file_size); noecho();
  if (file_size > 4096) {
      mvprintw(3, 1, "Creating files larger than 4096 bytes is not supported");
      CONTINUE_MESSAGE;
      return;
  }

  clear();

  // Get parent directory inode.
  lseek(fd,
        (1 + 2 * freemap_blocksize) * BLOCKSIZE + sizeof(struct gfs_inode) * parent_inode,
        SEEK_SET);
  read(fd, workspace, sizeof(struct gfs_inode));
  parent_node = (struct gfs_inode *)workspace;

  // First block of parent directory file.
  pblock = parent_node->blocks[0];

  // Get parent's block location so we can change it.
  lseek(fd, pblock * BLOCKSIZE, SEEK_SET);
  read(fd, workspace, BLOCKSIZE);

  parent_block = workspace;

  // Look through dir entries offset things.
  while (dtoh32(*(uint32_t *)parent_block) != 0)
      parent_block = workspace + dtoh32(*(uint32_t *)parent_block);

  // Crappy way to do this... only works if last dir entry is at the end.
  offset = parent_block + 4 + 4;  // skip past offset + inode #
  offset += (*offset) + 1;        // skip past size + name

  // Set old last entry offset thing to point to the new one... yeah.
  (*(uint32_t *)parent_block) = htod32(offset - workspace);

  // New entry's offset to next is zero.
  *(((uint32_t *)offset) + 0) = 0;

  // Find first free inode.
  current_inode = allocate_inode(fd);

  // New entry's inode.
  *(((uint32_t *)offset) + 1) = htod32(current_inode);
  // new entry's size + name
  *(offset + 8) = strlen(name);
  memcpy(offset + 9, name, strlen(name));

  // Write the changed parent dir info back.
  lseek(fd, pblock * BLOCKSIZE, SEEK_SET);
  write(fd, workspace, BLOCKSIZE);

  // Mess with our current node.
  current_node = (struct gfs_inode *)workspace;

  // Don't need this for empty file.
  if (file_size > 0) {
      current_block = allocate_block(fd);
  }

  // Fill in the current file's inode.
  current_node->nlinks = 1;
  current_node->owner_id = 0;
  current_node->group_id = 0;
  current_node->mode = S_IFREG|S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH;
  current_node->file_size = file_size;
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

  // Writes the above inode.
  lseek(fd,
        (1 + 2*freemap_blocksize)*BLOCKSIZE + sizeof(struct gfs_inode)*current_inode,
        SEEK_SET);
  write(fd, workspace, sizeof(struct gfs_inode));

  if (file_size > 0) {
      int  i;
      char data = 'A';

      // Now use the workspace to create the file itself.
      memset(workspace, 0, BLOCKSIZE);
      for (i = 0; i < file_size; ++i) {
          workspace[i] = data;
          if (++data > 'Z') data = 'A';
      }

      // Now write it out.
      lseek(fd, current_block*BLOCKSIZE, SEEK_SET);
      write(fd, workspace, BLOCKSIZE);
  }

  mvprintw(1, 1, "Created file '%s' in the root directory.\n", name);
  CONTINUE_MESSAGE;
}
