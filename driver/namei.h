/*!
 * \file namei.h
 * \author Peter Chapin <spicacality@kelseymountain.org>
 */

#ifndef NAMEI_H
#define NAMEI_H

struct dentry *gfs_lookup( struct inode *dir, struct dentry *dentry, unsigned int flags );

#endif
