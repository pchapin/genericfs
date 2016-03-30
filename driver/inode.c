/****************************************************************************
FILE   : inode.c
SUBJECT: Inode operation methods for the generic file system.
AUTHOR : (C) Copyright 2011 by Peter C. Chapin

This file contains the inode operation methods for the generic file system.

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
#include <linux/fs.h>
#include <linux/buffer_head.h>
#include <linux/mpage.h>
#include <linux/slab.h>

// Project specific.
#include "global.h"
#include "inode.h"
#include "ops.h"

// +++++
// Inode methods
// +++++

struct inode *gfs_iget(struct super_block *sb, unsigned long ino)
{
    unsigned int block_number;   // Block number of where inode is located.
    unsigned int block_offset;   // Offset into block of inode.
    struct buffer_head     *bh;  // Points at the block containing the inode.
    struct gfs_inode_info  *gi;  // Points at the entire inode structure.
    struct gfs_inode       *gin; // Points at the inode from the disk.
    struct gfs_ssupplementary *super_sup; // Points at a supplementary super block struct.
    struct inode *inode;

    ENTERED

    super_sup = (struct gfs_ssupplementary *)sb->s_fs_info;

    inode = iget_locked(sb, ino);
    if (!inode)
        return ERR_PTR(-ENOMEM);
    if (!(inode->i_state & I_NEW))
        return inode;

    // Compute which block contains the inode in question.
    block_number  = 1 + super_sup->gsb->inodefreemap_blocks +
                        super_sup->gsb->blockfreemap_blocks;
    block_number += 
        (sizeof(struct gfs_inode)*(inode->i_ino)) /
            super_sup->gsb->block_size;
    block_offset  =
        (sizeof(struct gfs_inode)*(inode->i_ino)) %
            super_sup->gsb->block_size;

    // Now get the block.
    if (!(bh = sb_bread(inode->i_sb, block_number))) {
        printk(KERN_ERR "GenericFS: can't read inode block!");

        // I'm unclear if make_bad_inode or iget_failed should be used here.
        // make_bad_inode(inode);
        iget_failed(inode);
        return ERR_PTR(-EINVAL);
    }

    // It worked! Now initialize the inode.
    gin = (struct gfs_inode *)((char *)bh->b_data + block_offset);
    inode->i_mode    = le16_to_cpu(gin->mode);
    inode->i_uid.val = le16_to_cpu(gin->owner_id);
    inode->i_gid.val = le16_to_cpu(gin->group_id);
    set_nlink(inode, le16_to_cpu(gin->nlinks));
    inode->i_size  = le32_to_cpu(gin->file_size);
    inode->i_atime.tv_sec  = le32_to_cpu(gin->atime);
    inode->i_mtime.tv_sec  = le32_to_cpu(gin->mtime);
    inode->i_ctime.tv_sec  = le32_to_cpu(gin->ctime);
    inode->i_atime.tv_nsec = inode->i_mtime.tv_nsec = inode->i_ctime.tv_nsec = 0;
    inode->i_blocks  = inode->i_size >> BLOCKSIZEBITS;
    if ((inode->i_size & (BLOCKSIZE - 1)) != 0)
        inode->i_blocks++;
    inode->i_version = ++global_event;
    inode->i_generation = 1;  // ext2 uses this when accessed over the network.

    // Fill in the file system specific fields.
    gi = GFS_I(inode);
    gi->blocks[0] = le32_to_cpu(gin->blocks[0]);
    gi->blocks[1] = le32_to_cpu(gin->blocks[1]);
    gi->blocks[2] = le32_to_cpu(gin->blocks[2]);
    gi->blocks[3] = le32_to_cpu(gin->blocks[3]);
    gi->first_indirect  = le32_to_cpu(gin->first_indirect);
    gi->second_indirect = le32_to_cpu(gin->second_indirect);

    // Set up the inode operation methods.
    if (S_ISREG(inode->i_mode)) {
        inode->i_op  = &gfs_file_inode_operations;
        inode->i_fop = &gfs_file_operations;
        inode->i_mapping->a_ops = &gfs_aops;
    }
    else if (S_ISDIR(inode->i_mode)) {
        inode->i_op  = &gfs_dir_inode_operations;
        inode->i_fop = &gfs_dir_operations;
        inode->i_mapping->a_ops = &gfs_aops;
    }
    else if (S_ISLNK(inode->i_mode)) {
        inode->i_op  = &gfs_symlink_inode_operations;
        inode->i_mapping->a_ops = &gfs_aops;
    }
    else {
        // The last parameter here is the "raw" device number. GenericFS doesn't currently support
        // devices so this has to be fixed later when such support is added. (The zero is there so
        // that the code will compile.
        init_special_inode(inode, inode->i_mode, 0);
    }

    unlock_new_inode(inode);
    return inode;
}

// +++++
// Address space methods
// +++++

// This method converts a file-relative block number into a partition- relative block number. It
// then fills in certain fields of bh_result accordingly. It is supposed to allocate a block if
// necessary but for now I don't bother with that.
// 
static int gfs_get_block(
    struct inode       *inode,
           sector_t     iblock,
    struct buffer_head *bh_result,
           int          create)
{
    struct gfs_inode_info *gi;

    ENTERED

    // This is a horrible hack. Always returning the first block.
    gi = GFS_I(inode);
    map_bh(bh_result, inode->i_sb, gi->blocks[0]);

    GENERIC_DEBUG(2,
        printk(DEBUG_HEADER "iblock=%llu, b_blocknr=%llu\n",
               __FUNCTION__, (unsigned long long)iblock, (unsigned long long)bh_result->b_blocknr))

    // Zero means no error. Use -EIO for I/O errors.
    return 0;
}


// This method fills in a page in the page cache with data from the disk.
int gfs_readpage(struct file *filp, struct page *page)
{
    ENTERED

    // Use a kernel helper function.
    return mpage_readpage(page, gfs_get_block);
}
