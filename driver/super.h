/*!
 * \file super.h
 * \author Peter Chapin <spicacality@kelseymountain.org>
 *
 * \brief Declarations of various super block operations.
 */

#ifndef SUPER_H
#define SUPER_H

struct inode *gfs_alloc_inode( struct super_block *sb );
void gfs_destroy_inode( struct inode *inode );
int  gfs_write_inode( struct inode *inode, struct writeback_control *wbc );
void gfs_put_super( struct super_block *sb );
int  gfs_statfs( struct dentry *de, struct kstatfs *buf );

#endif
