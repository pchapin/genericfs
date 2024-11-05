/*!
 * \file inode.c
 * \author Peter Chapin <spicacality@kelseymountain.org>
 */

#ifndef INODE_H
#define INODE_H

struct inode *gfs_iget( struct super_block *, unsigned long );
int gfs_readpage( struct file *filp, struct page *page );

#endif
