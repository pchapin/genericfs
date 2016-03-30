/*!
 * \file blocks.c
 * \author Peter C. Chapin <PChapin@vtc.vsc.edu>
 *
 * \brief This file provides the implementation of a block read/write abstraction. This version
 * uses the host file system as any user mode application might.
 */

#include <unistd.h>
#include "blocks.h"

//! Get a block from the indicated partition.
/*!
 * Use the underlying file system to get a block of data.
 *
 * \param part A pointer to a structure that specifies the partition from which data will be
 * read.
 * \param block_number The block number to read. If the block is out of range an error
 * indication is returned.
 * \param block A pointer to an array to receive the block data. The array is assumed to be
 * large enough.
 * \return Zero if successful; non-zero if an error occurs.
 */
int get_block( const struct Partition *part, uint32_t block_number, uint8_t *block )
{
    // TODO: Add error handling.
    lseek( part->fd, block_number * BLOCKSIZE, SEEK_SET );
    read ( part->fd, block, BLOCKSIZE );
    return 0;
}


//! Puts a block to the indicated partition.
/*!
 * \param part A pointer to a structure that specifies the partition to which data will be
 * written.
 * \param block_number The block number to write. If the block is out of range an error
 * indication is returned.
 * \param block A pointer to an array from which data will be written. The array is assumed to
 * be large enough.
 * \return Zero if successful; non-zero if an error occurs.
 */
int put_block( const struct Partition *part, uint32_t block_number, const uint8_t *block )
{
    // TODO: Add error handling.
    lseek( part->fd, block_number * BLOCKSIZE, SEEK_SET );
    write( part->fd, block, BLOCKSIZE );
    return 0;
}
