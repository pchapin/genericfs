/*!
 * \file tool.c
 * \author Peter Chapin <spicacality@kelseymountain.org>
 *
 * \brief Definitions of global data.
 */

#include "tool.h"

uint32_t block_count;           // Size of partition in blocks.
uint32_t freemap_bytesize;      // Size of the freemap in bytes.
uint32_t freemap_blocksize;     // Size of the freemap in blocks.
uint32_t inodetable_bytesize;   // Size of the inode table in bytes.
uint32_t inodetable_blocksize;  // Size of the inode table in blocks.
