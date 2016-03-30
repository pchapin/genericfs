/****************************************************************************
FILE   : super.c
SUBJECT: Module initialization and super block methods.
AUTHOR : (C) Copyright 2011 by Peter C. Chapin

This file contains the functions to initialize and clean up the module as well as the
definitions of the super block methods.

Please send comments or bug reports to

     Peter C. Chapin
     Computer Information Systems
     Vermont Technical College
     Randolph Center, VT 05061
     PChapin@vtc.vsc.edu
****************************************************************************/

#include <generated/autoconf.h>

// Pretty much required for a kernel module.
#define __NO_VERSION__
#include <linux/kernel.h>
#include <linux/module.h>

// Need for general file system stuff.
#include <linux/buffer_head.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/statfs.h>

// Project specific.
#include "global.h"
#include "inode.h"
#include "ops.h"
#include "super.h"

// Declare the function that reads a gfs superblock.
static
struct dentry *gfs_mount(
  struct file_system_type *fs_type, int flags, const char *dev_name, void *data);

// Declare the file system type structure.
struct file_system_type gfs_type = {
  .name     = "genericfs",
  .fs_flags = FS_REQUIRES_DEV,
  .mount    = gfs_mount,
  .kill_sb  = kill_block_super,
  .owner    = THIS_MODULE
};


static void init_once(void *foo)
{
  struct gfs_inode_info *gi = (struct gfs_inode_info *)foo;

  inode_init_once(&gi->vfs_inode);
}


// +++++
// Module "main" functions
// +++++
// 
int init_module(void)
{
  int rc;

  ENTERED

  // Create a slab cache for the supplementary inode objects.
  gfs_icache = kmem_cache_create(
    "gfs_inode_cache",
    sizeof(struct gfs_inode_info),
    0,
    SLAB_RECLAIM_ACCOUNT | SLAB_MEM_SPREAD,
    init_once);

  if (gfs_icache == 0) {
    printk(KERN_ERR "GenericFS: can't allocate inode cache.\n");
    return -ENOMEM;
  }

  // Now register the file system. Should check for error return and avoid printing the nice
  // "initialized" message if the filesystem fails to register.
  // 
  rc = register_filesystem(&gfs_type);

  printk(KERN_INFO "GenericFS: initialized.\n");
  return rc;
}


void cleanup_module(void)
{
  ENTERED

  // Clean up on the way out the door.
  unregister_filesystem(&gfs_type);
  kmem_cache_destroy(gfs_icache);
  printk(KERN_INFO "GenericFS: cleaned up.\n");
}


// +++++
// Superblock methods
// +++++

struct inode *gfs_alloc_inode(struct super_block *sb)
{
  struct gfs_inode_info *gi;

  ENTERED
  gi = (struct gfs_inode_info *)kmem_cache_alloc(gfs_icache, GFP_KERNEL);
  if (!gi)
    return NULL;

  return &gi->vfs_inode;
}


void gfs_destroy_inode(struct inode *inode)
{
  ENTERED
  kmem_cache_free(gfs_icache, GFS_I(inode));
}


//
// Do something useful.
//
int gfs_write_inode(struct inode *inode, struct writeback_control *wbc)
{
  ENTERED
  return 0;
}


//
// The following function is called when a file system is unmounted.  This makes it the
// complement to the read_super method above. This function needs to release any memory and any
// blocks in the buffer cache that are being held to support the activity of this particular
// mounted file system (thus this version is incomplete).
// 
void gfs_put_super(struct super_block *sb)
{
  struct gfs_ssupplementary *super_sup =
    (struct gfs_ssupplementary *)sb->s_fs_info;

  ENTERED

  brelse(super_sup->super_buffer);
  kfree(sb->s_fs_info);
  module_put(THIS_MODULE);
}


// Counts the bits in an unsigned integer.
// Stan Brinkerhoff
// 
// Originally: http://www-db.stanford.edu/~manku/bitcount/bitcount.html
// 
static
int gfs_bitcount(unsigned int n)  
{  
  int count=0;    
  while (n)
  {
    count += n & 0x1u;
    n >>= 1 ;
  }
  return count;
}


// This function is due to Stan Brinkerhoff. Fall semester 2003.
static
int gfs_freespace(struct super_block *sb)
{
  int x, m, counter = 0;
  struct buffer_head *bh;
  unsigned char block;

  struct gfs_ssupplementary *super_sup =
    (struct gfs_ssupplementary *)sb->s_fs_info;

  // Read the blocks out of the blockfreemap, a block at a time, and then count the number of
  // '1's listed in the resulting read.
  // 
  for(x = 0; x < super_sup->gsb->blockfreemap_blocks; x++){
    if (!(bh = sb_bread(sb, 1 + super_sup->gsb->inodefreemap_blocks + x))){
      return 0;
    }
    
    // This needs to be updated to check to make sure it doesn't overread the last block.
    // 
    for(m =0; m < 4096; m++){
      block = *(bh->b_data + m);    
      counter += gfs_bitcount(block);
    }
  }
  
  return counter;
}


// This function is due to Stan Brinkerhoff. Fall semester 2003.
static
int gfs_inodefreespace(struct super_block *sb)
{
  int x, m, counter = 0;
  struct buffer_head *bh;
  unsigned char block;

  struct gfs_ssupplementary *super_sup =
    (struct gfs_ssupplementary *)sb->s_fs_info;

  // Read the blocks out of the blockfreemap, a block at a time, and then count the number of
  // '1's listed in the resulting read.
  // 
  for(x = 0; x < super_sup->gsb->blockfreemap_blocks; x++){
    if (!(bh = sb_bread(sb, 1 + x))){
      return 0;
    }

    // This needs to be updated to check to make sure it doesn't overread the last block.
    // 
    for(m =0; m < 4096; m++){
      block = *(bh->b_data + m);
      counter += gfs_bitcount(block);
    }
  }

  return counter;
}


// This method is due to Stan Brinkerhoff. Fall semester 2003.
int gfs_statfs(struct dentry *de, struct kstatfs *buf)
{
  struct gfs_ssupplementary *super_sup =
    (struct gfs_ssupplementary *)de->d_sb->s_fs_info;

  ENTERED

  buf->f_type = de->d_sb->s_magic;
  buf->f_bsize = de->d_sb->s_blocksize;
  buf->f_bfree = buf->f_bavail =
    super_sup->gsb->total_blocks - gfs_freespace(de->d_sb);
  buf->f_blocks = super_sup->gsb->total_blocks;
  buf->f_namelen = 64; // Not sure what this is.
  buf->f_ffree = gfs_inodefreespace(de->d_sb);
  return 0;

}


// +++++
// Read super
// +++++

// This function is called at mount time to read the super block from the file system. If the
// super block can't be read it returns NULL (and the mount fails). This function does not need
// to be compli- cated because we aren't supporting any real mount options.
// 
static
int gfs_fill_super(struct super_block *sb, void *data, int flag)
{
  int    rc;
  struct gfs_ssupplementary *super_sup;

  ENTERED

  try_module_get(THIS_MODULE);

  // Tell the buffer cache how big the blocks are on this device.
  rc = set_blocksize(sb->s_bdev, BLOCKSIZE);
  if (rc < 0) {
    sb->s_dev = 0;
    printk(KERN_ERR "GenericFS: Unable to set block size on device.\n");
    module_put(THIS_MODULE);
    return rc;
  }

  // Allocate a supplementary super block structure.
  super_sup = kmalloc(GFP_KERNEL, sizeof(struct gfs_ssupplementary));
  if (super_sup == NULL) {
    sb->s_dev = 0;
    printk(KERN_ERR "GenericFS: Out of memory during mount operation.\n");
    module_put(THIS_MODULE);
    return rc;
  }

  // Read the superblock.
  if (!(super_sup->super_buffer = sb_bread(sb, 0))) {
    sb->s_dev = 0;
    printk(KERN_ERR "GenericFS: unable to read superblock.\n");
    kfree(super_sup);
    module_put(THIS_MODULE);
    return rc;
  }

  // Treat the block like a GenericFS super block. Does it look right?
  super_sup->gsb =
    (struct gfs_super_block *)(super_sup->super_buffer->b_data);

  if (le32_to_cpu(super_sup->gsb->magic_number) != 0xDEADBEEF) {
    sb->s_dev = 0;
    //printk(KERN_ERR
    //  "GenericFS: can't find a GenericFS on %s.\n", bdevname(sb->sb_dev));
    brelse(super_sup->super_buffer);
    kfree(super_sup);
    module_put(THIS_MODULE);
    return rc;
  }

  // It would probably be a good idea to check the "sanity" of some of the other superblock
  // fields as well. If the data in the position of the generic magic number just happened to be
  // 0xDEADBEEF but the system was otherwise nonsense... we don't want to try using it.

  // Set up the super operations.
  sb->s_op = &gfs_super_operations;

  // Set up the other fields of the superblock that I need to mess with.
  sb->s_magic          = le32_to_cpu(super_sup->gsb->magic_number);
  sb->s_blocksize      = le32_to_cpu(super_sup->gsb->block_size);
  sb->s_blocksize_bits = BLOCKSIZEBITS;
  sb->s_fs_info        = super_sup;

  // Try to read the root directory's i-node. If the i-node can't be read, then the mount fails.
  sb->s_root = d_make_root(gfs_iget(sb, 0));
  if (!sb->s_root) {
    sb->s_dev = 0;
    printk(KERN_ERR "GenericFS: unable to read root inode.\n");
    brelse(super_sup->super_buffer);
    kfree(super_sup);
    module_put(THIS_MODULE);
    return rc;
  }

  // Is this necessary? I don't change anything in the superblock.
  // sb->s_dirt = 1;

  return 0;
}


static
struct dentry *gfs_mount(
  struct file_system_type *fs_type,
  int         flags,
  const char *dev_name,
  void        *data)
{
  return mount_bdev(fs_type, flags, dev_name, data, gfs_fill_super);
}
