/*!
 * \file dir.c
 * \author Peter Chapin <spicacality@kelseymountain.org>
 */

#ifndef DIR_H
#define DIR_H

// This structure represents GenericFS directory entries.
struct gfs_direntry {
    unsigned int  next_offset;
    unsigned int  inode;
    unsigned char name_length;
             char name[0];     // name points off end of structure.
};

int gfs_readdir( struct file *filp, struct dir_context *ctx );

#endif
