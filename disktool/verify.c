/*!
 * \file verify.c
 * \author Peter C. Chapin <PChapin@vtc.vsc.edu>
 *
 * \brief Function to perform a file system check.
 */

#include <stdlib.h>
#include <string.h>

#include <curses.h>
#include <unistd.h>

#include "tool.h"

//! Set the block counters to appropriate initial values.
/*!
 * Most counters are initialized to zero. However the preallocated blocks used for file system
 * metadata need to have their counts initialized to one since they are, in effect, already
 * being used once.
 *
 * \param block_counters Pointer to an array of counters with one counter for each block. The
 * array is assumed to be properly sized.
 */
static void initialize_counters( uint32_t *block_counters )
{
    uint32_t i;
    uint32_t preallocated_block_count;

    memset( block_counters, 0, block_count * sizeof( uint32_t ) );
    preallocated_block_count = 1 + 2 * freemap_blocksize + inodetable_blocksize;
    for( i = 0; i < preallocated_block_count; ++i ) {
	block_counters[i] = 1;
    }
}


//! Locate and count each block accessible via a first indirection block.
/*!
 * The first zero block number found in the indirection block indicates the end of the useful
 * data. No information about the file's size is used to verify this. This function assumes that
 * all block numbers are in range. That assumption should eventually be checked.
 *
 * \param fd The handle of the partition file.
 * \param first_indirect The block number of the first indirection block.
 * \param block_counters Pointer to an array of counters with one counter for each block. The
 * array is assumed to be properly sized.
 */
static void find_first_indirection_blocks(
    int fd, uint32_t first_indirect, uint32_t *block_counters )
{
    uint8_t   workspace[BLOCKSIZE];
    uint32_t *block_numbers = ( uint32_t * )workspace;
    int i;

    // The indirect block itself is being used.
    block_counters[first_indirect]++;

    // Some error checking might be nice.
    lseek( fd, first_indirect * BLOCKSIZE, SEEK_SET );
    read( fd, workspace, BLOCKSIZE );

    for( i = 0; i < BLOCKSIZE / sizeof( uint32_t ); ++i ) {
	if( block_numbers[i] == 0 ) break;
	block_counters[block_numbers[i]]++;
    }
}


//! Locate and count each block accessible via a second indirection block.
/*!
 * The first zero block number found in the indirection block indicates the end of the useful
 * data. No information about the file's size is used to verify this. This function assumes that
 * all block numbers are in range. That assumption should eventually be checked.
 *
 * \param fd The handle of the partition file.
 * \param second_indirect The block number of the second indirection block.
 * \param block_counters Pointer to an array of counters with one counter for each block. The
 * array is assumed to be properly sized.
 */
static void find_second_indirection_blocks(
    int fd, uint32_t second_indirect, uint32_t *block_counters )
{
    uint8_t   workspace[BLOCKSIZE];
    uint32_t *block_numbers = ( uint32_t * )workspace;
    int i;

    // The indirect block itself is being used.
    block_counters[second_indirect]++;

    // Some error checking might be nice.
    lseek( fd, second_indirect * BLOCKSIZE, SEEK_SET );
    read( fd, workspace, BLOCKSIZE );

    for( i = 0; i < BLOCKSIZE / sizeof( uint32_t ); ++i ) {
	if( block_numbers[i] == 0 ) break;
	find_first_indirection_blocks( fd, block_numbers[i], block_counters );
    }
}


//! Finds and counts the blocks associated with a given inode.
/*!
 * Scans over all the blocks "attached" to the given inode and increments their counts in the
 * given counter array. This function assumes block and inode numbers are all in range. That
 * assumption should be checked.
 *
 * \param fd The handle of the partition file.
 * \param inode_number The inode number of the inode to analyze.
 * \param block_counters Pointer to an array of counters with one counter for each block. The
 * array is assumed to be properly sized.
 */
static void find_inode_blocks( int fd, uint32_t inode_number, uint32_t *block_counters )
{
    uint8_t  workspace[BLOCKSIZE];
    uint32_t relative_block = inode_number / 64;
    uint32_t inode_offset   = inode_number % 64;
    struct gfs_inode *current_inode;
    int i;

    // Some error checking might be nice.
    lseek( fd, ( 1 + 2*freemap_blocksize + relative_block ) * BLOCKSIZE, SEEK_SET );
    read( fd, workspace, BLOCKSIZE );
    current_inode = ( struct gfs_inode * )( workspace + ( 64 * inode_offset ) );

    // In what follows we assume that block numbers and indirection pointers are explicity
    // zeroed when they are not used. No information about the file's size is considered.
    
    for( i = 0; i < 4; ++i ) {
	if( current_inode->blocks[i] != 0 )
	    block_counters[current_inode->blocks[i]]++;
    }

    if( current_inode->first_indirect != 0 )
	find_first_indirection_blocks( fd, current_inode->first_indirect, block_counters );

    if( current_inode->second_indirect != 0 )
	find_second_indirection_blocks( fd, current_inode->second_indirect, block_counters );
}


//! Scan over all inodes and count blocks associated with each allocated inode.
/*!
 * The unallocated inodes are skipped. Any blocks associated with them are spurious.
 *
 * \param fd The handle of the parition file.
 * \param block_counters Pointer to an array of counters with one counter for each block. The
 * array is assumed to be properly sized.
 */
static void scan_inodes( int fd, uint32_t *block_counters )
{
    uint8_t  workspace[BLOCKSIZE];
    uint32_t block_index;
    int      block_offset;
    uint32_t inode_number = 0;
    int      bit_number;

    for( block_index = 0; block_index < freemap_blocksize; ++block_index ) {
        lseek( fd, (1 + block_index) * BLOCKSIZE, SEEK_SET );
	read( fd, workspace, BLOCKSIZE );
	for( block_offset = 0; block_offset < BLOCKSIZE; ++block_offset ) {
	    for( bit_number = 0; bit_number < 8; ++bit_number ) {
		if( workspace[block_offset] & ( 1 << bit_number ) ) {
		    printw( "%u, ", inode_number );
		    refresh( );
		    find_inode_blocks( fd, inode_number, block_counters );
		}
		++inode_number;
		if( inode_number == block_count ) goto done;
	    }
	}
    }
 done: ;
}


//! Check block counters for appropriate values.
/*!
 * Verify that each block has been counted either zero or one times. Report on counts larger
 * than one as "multiple use" errors.
 *
 * \param block_counters Pointer to an array of counters. The value of each count reflects how
 * many times each block has been used in a file or in file system metadata.
 */
static void check_counters( const uint32_t *block_counters )
{
    uint32_t i;

    for( i = 0; i < block_count; ++i ) {
	if( block_counters[i] != 0 && block_counters[i] != 1 ) {
	    printw( "Block used multiple times: block=%u, count=%u\n", i, block_counters[i] );
	    refresh( );
	}
    }
}


//! Compare block counters with the block free map.
/*!
 * Verify that each allocated block has a count of exactly one and each unallocated block has a
 * count of exactly zero. Report on inconsistencies. This function assumes counts are either
 * zero or one (call check_counters() first).
 *
 * \param block_counters Point to an array of counters. The value of each count is either zero
 * or one and reflects if the block is used in a file (via the file's inode) or in file system
 * metadata.
 */
static void check_block_freemap( int fd, const uint32_t *block_counters )
{
    uint8_t  workspace[BLOCKSIZE];
    uint32_t block_index;
    int      block_offset;
    uint32_t block_number = 0;
    int      bit_number;

    lseek( fd, ( 1 + freemap_blocksize ) * BLOCKSIZE, SEEK_SET );
    for( block_index = 0; block_index < freemap_blocksize; ++block_index ) {
	read( fd, workspace, BLOCKSIZE );
	for( block_offset = 0; block_offset < BLOCKSIZE; ++block_offset ) {
	    for( bit_number = 0; bit_number < 8; ++bit_number ) {
		if( workspace[block_offset] & ( 1 << bit_number ) ) {
		    if( block_counters[block_number] != 1 ) {
			printw( "Block allocated but not used: block=%u\n", block_number );
			refresh( );
		    }
		}
		else {
		    if( block_counters[block_number] != 0 ) {
			printw( "Unallocated block in use: block=%u\n", block_number );
			refresh( );
		    }
		}
		++block_number;
		if( block_number == block_count ) goto done;
	    }
	}
    }
 done: ;
}


//! Implements the 'verify' option on the main menu.
/*!
 * Checks the give file system, assumed to be GenericFS, for internal consistency.
 *
 * \param fd The handle to the file system partition (previously opened).
 */
void verify_file_system( int fd )
{
    uint32_t *block_counters;

    clear( );
    block_counters = ( uint32_t * )malloc( block_count * sizeof( uint32_t ) );
    if( block_counters == NULL ) {
	mvaddstr( 1, 1, "Unable to allocate supporting data structures" );
    }
    else {
	move( 1, 1 );
	initialize_counters( block_counters );
	scan_inodes( fd, block_counters );
	check_counters( block_counters );
	check_block_freemap( fd, block_counters );
	free( block_counters );

	// TODO:
	//
	// Check consistency of directories and nlinks field in each inode. Also check
	// consistency between inodes mentioned in directories and the inode free map.
    }
    CONTINUE_MESSAGE;
}
