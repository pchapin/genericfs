/*!
 * \file global.c
 * \author Peter Chapin <spicacality@kelseymountain.org>
 *
 * \brief This file contains the definitions of the global data used by GenericFS. Normally this
 * material would be accessed via the superblock object so that each mounted partition would
 * have its own. However, setting that up is annoying. By making this data global we are
 * limiting ourselves to mounting a single GenericFS partition. For now that is acceptable.
 */

#include <generated/autoconf.h>

// Pretty much required for a kernel module.
#include <linux/kernel.h>
#include <linux/module.h>

// Need for general file system stuff.
#include <linux/fs.h>
#include <linux/slab.h>

// Project specific.
#include "global.h"

MODULE_AUTHOR("Peter Chapin");
MODULE_DESCRIPTION("A Generic File System");
MODULE_LICENSE("GPL");

// Points at the buffer in the buffer cache containing superblock.
struct buffer_head *super_buffer;

// Points at the genericfs_super_block in the above buffer.
struct genericfs_super_block *gsb;

// Slab cache for supplementary inode data.
struct kmem_cache *gfs_icache;

// Used for inode version numbers.
int global_event = 0;
