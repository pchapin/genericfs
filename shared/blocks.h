/*!
 * \file blocks.h
 * \author Peter C. Chapin <PChapin@vtc.vsc.edu>
 *
 * \brief This file provides the interface to a block read/write abstraction. This abstraction
 * is implemented differently in the disktool and the driver. However, the higher level code
 * that is shared by both can use this abstraction in a uniform manner.
 */

#ifndef BLOCKS_H
#define BLOCKS_H

#include "genericfs.h"

#if defined(__KERNEL__)
struct Partition {
    // Kernel specific information for identifying partitions.
};
#else
struct Partition {
    int fd;
};
#endif

int get_block( const struct Partition *part, uint32_t block_number, uint8_t *block );
int put_block( const struct Partition *part, uint32_t block_number, const uint8_t *block );

#endif
