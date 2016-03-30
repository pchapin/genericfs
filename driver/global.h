/****************************************************************************
FILE   : global.h
SUBJECT: Global data for the genericfs.
AUTHOR : (C) Copyright 2011 by Peter C. Chapin

This file contains the declarations of the global data used by the generic file system. See
global.c for more information.

Please send comments or bug reports to

     Peter C. Chapin
     Computer Information Systems
     Vermont Technical College
     Randolph Center, VT 05061
     PChapin@vtc.vsc.edu
****************************************************************************/

#ifndef GLOBAL_H
#define GLOBAL_H

#include "genericfs.h"

#define DEBUG_LEVEL 3
// Level 0: No debugging messages.
// Level 1: Trace into "significant" GenericFS method functions. Complete.
// Level 2: Show interesting "major" events in a function. Incomplete.
// Level 3: Show specific details and trouble spots. Incomplete.
//
// "Complete" means that every appropriate place for this type of message produces a
// message. For example, debug level 1 provides a complete trace of all calls to significant
// GenericFS functions.  "Incomplete" means that there are places where a message might be
// appropriate but none is produced. In general level 2 and level 3 messages are added on an
// as-needed basis. Note that the levels are cumulative. Setting DEBUG_LEVEL to 3, for example,
// also produces level 1 and level 2 messages. A lot of output might be produced at the higher
// debug levels so keep tests as brief as possible when debugging deeply.

// Use this header in printk messages to make debug messages stand out.
#define DEBUG_HEADER KERN_INFO "GenericFS DEBUG: %s: "

// A DEBUG_LEVEL of zero removes debugging code.
#if DEBUG_LEVEL == 0
  #define GENERIC_DEBUG(level, statement)
#else
  #define GENERIC_DEBUG(level, statement) \
    if (DEBUG_LEVEL >= level) \
      { statement; }
#endif

// Use this macro at the start of a function to implement level 1 tracing.
#define ENTERED \
  GENERIC_DEBUG(1, printk(DEBUG_HEADER "Entered\n", __FUNCTION__))


// This structure contains "supplementary" information required on a per mount basis. Each
// superblock contains a pointer to one of these objects where information specific to that
// partition is stored.
//
struct gfs_ssupplementary {

  // Points at the buffer in the buffer cache containing superblock.
  struct buffer_head *super_buffer;

  // Points at the gfs_super_block in the above buffer.
  struct gfs_super_block *gsb;

};

// Slab cache for inode data.
extern struct kmem_cache *gfs_icache;

// Used for inode version numbers.
extern int global_event;

// Horrible hack.
static inline struct gfs_inode_info *GFS_I(struct inode *inode)
{
  return container_of(inode, struct gfs_inode_info, vfs_inode);
}

#endif
