/****************************************************************************
FILE   : dir.h
SUBJECT: Directory operation methods for the generic file system.
AUTHOR : (C) Copyright 2011 by Peter C. Chapin


Please send comments or bug reports to

     Peter C. Chapin
     Computer Information Systems
     Vermont Technical College
     Randolph Center, VT 05061
     PChapin@vtc.vsc.edu
****************************************************************************/

#ifndef DIR_H
#define DIR_H

// This structure represents GenericFS directory entries.
struct gfs_direntry {
  unsigned int  next_offset;
  unsigned int  inode;
  unsigned char name_length;
           char name[0];     // name points off end of structure.
};

int gfs_readdir(struct file *filp, struct dir_context *ctx);

#endif
