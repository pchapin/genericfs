/****************************************************************************
FILE   : ops.h
SUBJECT: Declaration of various operations structures.
AUTHOR : (C) Copyright 2011 by Peter C. Chapin


Please send comments or bug reports to

     Peter C. Chapin
     Computer Information Systems
     Vermont Technical College
     Randolph Center, VT 05061
     PChapin@vtc.vsc.edu
****************************************************************************/

#ifndef OPS_H
#define OPS_H

#include <linux/fs.h>

// +++++
// Super Operations
// +++++
extern struct super_operations gfs_super_operations;

// +++++
// File Operations
// ++++++
extern struct file_operations gfs_file_operations;
extern struct file_operations gfs_dir_operations;

// +++++
// Inode Operations
// +++++
extern struct inode_operations gfs_file_inode_operations;
extern struct inode_operations gfs_dir_inode_operations;
extern struct inode_operations gfs_symlink_inode_operations;

// +++++
// Address Space Operations
// +++++
extern struct address_space_operations gfs_aops;

#endif
