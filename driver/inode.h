/****************************************************************************
FILE   : inode.h
SUBJECT: Inode operation methods for the generic file system.
AUTHOR : (C) Copyright 2011 by Peter C. Chapin


Please send comments or bug reports to

     Peter C. Chapin
     Computer Information Systems
     Vermont Technical College
     Randolph Center, VT 05061
     PChapin@vtc.vsc.edu
****************************************************************************/

#ifndef INODE_H
#define INODE_H

struct inode *gfs_iget(struct super_block *, unsigned long);
int gfs_readpage(struct file *filp, struct page *page);

#endif
