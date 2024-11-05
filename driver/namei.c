/*!
 * \file namei.c
 * \author Peter Chapin <spicacality@kelseymountain.org>
 */

#include <generated/autoconf.h>

// Pretty much required for a kernel module.
#define __NO_VERSION__
#include <linux/kernel.h>
#include <linux/module.h>

// Need for general file system stuff.
#include <linux/fs.h>

// Project specific.
#include "global.h"
#include "namei.h"

// +++++
// Inode methods
// +++++

//
// This method is used to locate a particular name in a directory.
//
struct dentry *gfs_lookup( struct inode *dir, struct dentry *dentry, unsigned int flags )
{
    ENTERED
    return NULL;
}
