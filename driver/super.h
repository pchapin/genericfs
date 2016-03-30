/****************************************************************************
FILE   : super.h
SUBJECT: Declaration of various super block operations.
AUTHOR : (C) Copyright 2011 by Peter C. Chapin


Please send comments or bug reports to

     Peter C. Chapin
     Computer Information Systems
     Vermont Technical College
     Randolph Center, VT 05061
     PChapin@vtc.vsc.edu
****************************************************************************/

#ifndef SUPER_H
#define SUPER_H

struct inode *gfs_alloc_inode(struct super_block *sb);
void gfs_destroy_inode(struct inode *inode);
int  gfs_write_inode(struct inode *inode, struct writeback_control *wbc);
void gfs_put_super(struct super_block *sb);
int  gfs_statfs(struct dentry *de, struct kstatfs *buf);

#endif
