/****************************************************************************
FILE   : dir.c
SUBJECT: Directory operation methods for the generic file system.
AUTHOR : (C) Copyright 2011 by Peter C. Chapin

This file contains the directory operation methods for the generic file system.

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

// Need for general file system stuff.
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/slab.h>

// Project specific.
#include "global.h"
#include "dir.h"


// Returns the size of the directory file in pages.
static inline unsigned long dir_pages(struct inode *inode)
{
  return (inode->i_size + PAGE_CACHE_SIZE-1) >> PAGE_CACHE_SHIFT;
}

// Unmaps a page and marks it as free in the page cache. Use this whenever you no longer need to
// look at a page.
// 
static inline void gfs_put_page(struct page *page)
{
  ENTERED

  kunmap(page);
  page_cache_release(page);
}


// "Borrowed" more or less as-is from ext2. This function gets page n of the directory specified
// by dir. (The 'n' is relative to the beginning of the directory).
//
static struct page *gfs_get_page(struct inode *dir, unsigned long n)
{
    struct address_space *mapping = dir->i_mapping;
    struct page          *page;

    ENTERED
    GENERIC_DEBUG(2,
        printk(DEBUG_HEADER "dir = %p, n = %lu\n", __FUNCTION__, dir, n));

    page = read_mapping_page(mapping, n, NULL);
    if (!IS_ERR(page)) {
        kmap(page);
        if (!PageChecked(page))
            // For now, don't bother checking pages here. Assume they are okay.
            // gfs_check_page(page);
        if (PageError(page))
            goto fail;
    }
    return page;

fail:
    GENERIC_DEBUG(3, printk(DEBUG_HEADER "Failed\n", __FUNCTION__));
    gfs_put_page(page);
    return ERR_PTR(-EIO);
}


// +++++
// Directory methods
// +++++

//
// This method is used to read directory entries out of a directory. This version isn't very
// efficient because it only returns one directory entry each time it is called. It should use
// filldir() in a loop to fill up as many directory entries as possible. However, this version
// does return the correct data eventually.
//
// Note that this version works only if the entire directory fits into a single block
// (page). Obviously this restriction needs to be lifted eventually.
//
int gfs_readdir(struct file *filp, struct dir_context *ctx)
{
           loff_t pos    = ctx->pos;                // Current position in dir.
  struct   inode *inode  = file_inode(filp);        // Dir's inode.
  unsigned long   n      = pos >> PAGE_CACHE_SHIFT; // Page # in dir.
  unsigned long   offset = pos & ~PAGE_CACHE_MASK;  // Offset on page.
  unsigned long   npages = dir_pages(inode);        // Size of dir in pages.

  char *kaddr;    // Address of dir page in kernel address space.
  char *limit;
  struct gfs_direntry *de;  // Points at dir entry on a page.
  struct page *page;

  ENTERED

  // Verify that the current position is in reasonable bounds. This allows for an extra byte for
  // a name consisting of one character. I will have to check for sufficient space for longer
  // names later.
  // 
  if (pos > (inode->i_size - sizeof(struct gfs_direntry) - 1)) goto done;

  // This gets a single directory entry.

  page = gfs_get_page(inode, n);
  if (IS_ERR(page)) goto done;

  kaddr = page_address(page);
  de = (struct gfs_direntry *)(kaddr + offset);

  GENERIC_DEBUG(3, printk(DEBUG_HEADER "n=%lu, offset=%lu\n", __FUNCTION__, n, offset))
  GENERIC_DEBUG(3, printk(DEBUG_HEADER "next=%d, inode=%d, len=%d\n", __FUNCTION__, le32_to_cpu(de->next_offset), le32_to_cpu(de->inode), le32_to_cpu(de->name_length)))

  limit = kaddr + (PAGE_CACHE_SIZE - sizeof(struct gfs_direntry) - 1);

  // If this directory is (could be) in bounds...
  if ((char *)de <= limit) {
    dir_emit(ctx, de->name, de->name_length, le32_to_cpu(de->inode), DT_UNKNOWN);

    // If there are no more entires, set the file position to EOF.
    if (de->next_offset == 0) ctx->pos = inode->i_size;
    else {
      ctx->pos = le32_to_cpu(de->next_offset);
    }
  }
  gfs_put_page(page);

 done:
  // UPDATE_ATIME(inode);
  return 0;
}
