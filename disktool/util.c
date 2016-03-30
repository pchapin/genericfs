/*!
 * \file tool.h
 * \author Peter C. Chapin <PChapin@vtc.vsc.edu>
 *
 * \brief Utility functions.
 */

#include <stdlib.h>

#include <sys/types.h>
#include <unistd.h>

#include "genericfs.h"
#include "tool.h"

// This function is due to Daron Hume
//
//
static void mark_freemap(
    int      fd,
    uint32_t start_block,
    uint32_t end_block,
    uint32_t block,
    int      byte,
    int      bit)
{
    uint8_t freemap_block[BLOCKSIZE];

    lseek(fd, (start_block + block)*BLOCKSIZE, SEEK_SET);
    read(fd, freemap_block, BLOCKSIZE);

    freemap_block[byte] |= (1 << bit);

    lseek(fd, (start_block + block)*BLOCKSIZE, SEEK_SET);
    write(fd, freemap_block, BLOCKSIZE);
}


// This function is due to Daron Hume
//
static void scan_freemap(
    int       fd,
    uint32_t  start_block,
    uint32_t  end_block,
    uint32_t *block,
    int      *byte,
    int      *bit)
{
    uint8_t  freemap_block[BLOCKSIZE];
    uint32_t current_block;
    int      current_byte;
    int      current_bit;

    lseek(fd, BLOCKSIZE * start_block, SEEK_SET);

    current_bit = 0;
    for (current_block = 0; current_block < (end_block - start_block); ++current_block) {
        read(fd, freemap_block, BLOCKSIZE);

        for (current_byte = 0; current_byte < BLOCKSIZE; ++current_byte) {
            if (!(freemap_block[current_byte] & 0x80)) current_bit = 8;
            if (!(freemap_block[current_byte] & 0x40)) current_bit = 7;
            if (!(freemap_block[current_byte] & 0x20)) current_bit = 6;
            if (!(freemap_block[current_byte] & 0x10)) current_bit = 5;
            if (!(freemap_block[current_byte] & 0x08)) current_bit = 4;
            if (!(freemap_block[current_byte] & 0x04)) current_bit = 3;
            if (!(freemap_block[current_byte] & 0x02)) current_bit = 2;
            if (!(freemap_block[current_byte] & 0x01)) current_bit = 1;
            if (current_bit) break;
        }
        if (current_bit) break;
    }
    --current_bit;

    // return something block, byte, bit?
    *block = current_block;
    *byte  = current_byte;
    *bit   = current_bit;
}


//! Allocates a free block.
/*!
 * This function locates a free block in the block freemap. It returns the block number
 * allocated and updates the free map to mark that block as allocated.
 *
 * \param fd An open file handle to the GenericFS partition.
 * \return The block number of a newly allocated block.
 * \todo Add error reporting for the case when there are no available blocks.
 */
uint32_t allocate_block(int fd)
{
    uint32_t block;
    int      byte;
    int      bit;

    scan_freemap(fd, 1 + freemap_blocksize, 1 + 2 * freemap_blocksize, &block, &byte, &bit);
    mark_freemap(fd, 1 + freemap_blocksize, 1 + 2 * freemap_blocksize,  block,  byte,  bit);
    return (block * BLOCKSIZE * 8) + (byte * 8) + bit;
}


//! Allocates a free inode.
/*!
 * This function locates a free inode in the inode freemap. It returns the inode number
 * allocated and updates the free map to mark that inode as allocated.
 *
 * \param fd An open file handle to the GenericFS partition.
 * \return The inode number of a newly allocated inode.
 * \todo Add error reporting for the case when there are no available inodes.
 */
uint32_t allocate_inode(int fd)
{
    uint32_t block;
    int      byte;
    int      bit;

    scan_freemap(fd, 1, 1 + freemap_blocksize, &block, &byte, &bit);
    mark_freemap(fd, 1, 1 + freemap_blocksize,  block,  byte,  bit);
    return (block * BLOCKSIZE * 8) + (byte * 8) + bit;
}


// File name checking.
int valid_filename(const char *name)
{
  // For now, all names are okay.
  return 1;
}

// Endianness management. On IA-32 these functions do nothing.
uint32_t htod32(uint32_t value)
{
  return value;
}

uint32_t dtoh32(uint32_t value)
{
  return value;
}

// Directory management.
int name_exists(int fd, uint32_t dir_inode, const char *name)
{
  // For now, the name does not exist in the directory.
  return 0;
}

int add_entry(int fd, uint32_t dir_inode, const char *name, uint32_t inode)
{
  // For now claim that we failed.
  return 0;
}

int check_consistency(int fd, uint32_t dir_inode)
{
  // For now claim that the directory is consistent.
  return 1;
}

uint8_t *get_directory(int fd, struct gfs_inode *dir_inode)
{
  uint32_t i;               // Loop index variable.
  uint32_t dir_blocks;      // Number of blocks in directory file.
  uint8_t *raw;             // Pointer to start of directory memory.
  uint8_t *block_stepper;   // Points at next memory for next block.
  uint32_t block_no;        // Current block number.
  uint8_t *indirect = NULL; // Points at memory copy of 1st indirect.

  // How many blocks are in the directory file?
  dir_blocks = dir_inode->file_size/BLOCKSIZE;
  if ((dir_inode->file_size % BLOCKSIZE) != 0) dir_blocks++;

  // Allocate space for the required number of blocks.
  raw = malloc(dir_blocks * BLOCKSIZE);
  if (raw == NULL) return NULL;
  block_stepper = raw;

  // Get each block from the disk.
  for (i = 0; i < dir_blocks; ++i) {
    if (i < 4) {
      block_no = dir_inode->blocks[i];
      lseek(fd, block_no * BLOCKSIZE, SEEK_SET);
      read(fd, block_stepper, BLOCKSIZE);
    }
    else if (i < 4 + 1024) {
      // If this is the first time, load the 1st indirect block.
      if (indirect == NULL) {
        block_no = dir_inode->first_indirect;
        indirect = malloc(BLOCKSIZE);
        if (indirect == NULL) {
          free(raw);
          return NULL;
        }
        lseek(fd, block_no * BLOCKSIZE, SEEK_SET);
        read(fd, indirect, BLOCKSIZE);
      }
      // Get block via 1st indirection pointer.
      block_no = ((uint32_t *)indirect)[i - 4];
      lseek(fd, block_no * BLOCKSIZE, SEEK_SET);
      read(fd, block_stepper, BLOCKSIZE);
    }
    else {
      // Get block via 2nd indirection pointer.
      // Bail for now. Who makes directories this large anyway?
      free(raw);
      free(indirect);
      return NULL;
    }
    block_stepper += BLOCKSIZE;
  }

  free(indirect);
  return raw;
}
