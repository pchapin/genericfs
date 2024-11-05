/*!
 * \file ops.h
 * \author Peter Chapin <spicacality@kelseymountain.org>
 */

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
