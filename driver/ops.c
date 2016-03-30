/****************************************************************************
FILE   : ops.c
SUBJECT: Definition of various operations structures.
AUTHOR : (C) Copyright 2011 by Peter C. Chapin


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

// Project specific.
#include "global.h"
#include "dir.h"
#include "file.h"
#include "inode.h"
#include "namei.h"
#include "ops.h"
#include "super.h"

// +++++
// Super Operations
// +++++

struct super_operations gfs_super_operations = {
  .alloc_inode   = gfs_alloc_inode,
  .destroy_inode = gfs_destroy_inode,
  .write_inode   = gfs_write_inode,
  .put_super     = gfs_put_super,
  .statfs        = gfs_statfs
};


// +++++
// File Operations
// +++++

struct file_operations gfs_file_operations = {
    .llseek = generic_file_llseek,
    .read   = do_sync_read,
    .write  = do_sync_write,
    .mmap   = generic_file_mmap,
    .open   = generic_file_open
};

struct file_operations gfs_dir_operations = {
    .read    = generic_read_dir,
    .iterate = gfs_readdir
};


// +++++
// Inode Operations
// +++++

struct inode_operations gfs_file_inode_operations = {
};

struct inode_operations gfs_dir_inode_operations = {
    .lookup = gfs_lookup
};

struct inode_operations gfs_symlink_inode_operations = {
};


// +++++
// Address Space Operations
// +++++

struct address_space_operations gfs_aops = {
    .readpage = gfs_readpage
};
