/*!
 * \file genericfs.h
 * \author Peter Chapin <spicacality@kelseymountain.org>
 *
 * \brief This file defines GenericFS on-disk data structures and some associated types and
 * macros.
 *
 * \todo Consider renaming this file `gfs.h` for consistency with the way other GenericFS
 * datastructures are named.
 */

#ifndef GENERICFS_H
#define GENERICFS_H

// Get nice typenames for exact sized integers.
#if defined(__KERNEL__)
#include <linux/types.h>
#else
#include <stdint.h>
#endif

#define BLOCKSIZE     4096
#define BLOCKSIZEBITS   12

// This structure describes the superblock on disk.
struct gfs_super_block {
  uint32_t magic_number;
  uint32_t block_size;
  uint32_t total_blocks;
  uint32_t inodefreemap_blocks;
  uint32_t blockfreemap_blocks;
  uint32_t inodetable_blocks;
};

// This structure describes the inodes on disk.
struct gfs_inode {
  uint32_t nlinks;
  uint32_t owner_id;
  uint32_t group_id;
  uint32_t mode;
  uint32_t file_size;
  uint32_t atime;
  uint32_t mtime;
  uint32_t ctime;
  uint32_t blocks[4];
  uint32_t first_indirect;
  uint32_t second_indirect;
  uint32_t unused[2];
};

#if defined(__KERNEL__)
// Is there some reason why this material isn't in a driver-specific header?

#include <linux/fs.h>

// This structure contains file system specific inode information as represented in kernel
// memory. Notice that one of its members is a VFS inode structure.
//
struct gfs_inode_info {
  uint32_t blocks[4];
  uint32_t first_indirect;
  uint32_t second_indirect;
  struct inode vfs_inode;
};

#endif  //  __KERNEL__

#endif
