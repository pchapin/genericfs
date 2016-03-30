/****************************************************************************
FILE   : namei.h
SUBJECT: 
AUTHOR : (C) Copyright 2011 by Peter C. Chapin

Please send comments or bug reports to

     Peter C. Chapin
     Computer Information Systems
     Vermont Technical College
     Randolph Center, VT 05061
     PChapin@vtc.vsc.edu
****************************************************************************/

#ifndef NAMEI_H
#define NAMEI_H

struct dentry *gfs_lookup( struct inode *dir, struct dentry *dentry, unsigned int flags );

#endif
