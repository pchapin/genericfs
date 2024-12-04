/*!
 * \file verify.c
 * \author Peter Chapin <spicacality@kelseymountain.org>
 *
 * \brief Function to perform a file system check.
 */

#include <stdlib.h>
#include <string.h>

#include <curses.h>
#include <unistd.h>

#include "tool.h"

// Because it looks nice...
#define PRIVATE static

//! Set the block counters to appropriate initial values.
/*!
 * Most counters are initialized to zero. However the preallocated blocks used for file system
 * metadata need to have their counts initialized to one since they are, in effect, already
 * being used once.
 *
 * \param block_counters Pointer to an array of counters with one counter for each block. The
 * array is assumed to be properly sized.
 */
PRIVATE void initialize_block_counters( uint32_t *block_counters )
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
 * all block numbers are in range.
 *
 * \todo Check that block numbers are in range.
 * \todo Add error handling to the file I/O operations.
 *
 * \param fd The handle of the partition file.
 * \param first_indirect The block number of the first indirection block.
 * \param block_counters Pointer to an array of counters with one counter for each block. The
 * array is assumed to be properly sized and initialized.
 */
PRIVATE void find_first_indirection_blocks( int fd, uint32_t first_indirect, uint32_t *block_counters )
{
    uint8_t   workspace[BLOCKSIZE];
    uint32_t *block_numbers = ( uint32_t * )workspace;  // Treat array as an array of uint32_t.
    int i;

    // The indirect block itself is being used, so count it.
    block_counters[first_indirect]++;

    lseek( fd, first_indirect * BLOCKSIZE, SEEK_SET );
    read( fd, workspace, BLOCKSIZE );

    // For every non-zero block number in the indirect block, count it.
    for( i = 0; i < BLOCKSIZE / sizeof( uint32_t ); ++i ) {
	if( block_numbers[i] == 0 ) break;
	block_counters[block_numbers[i]]++;
    }
}


//! Locate and count each block accessible via a second indirection block.
/*!
 * The first zero block number found in the indirection block indicates the end of the useful
 * data. No information about the file's size is used to verify this. This function assumes that
 * all block numbers are in range.
 *
 * \todo Check that block numbers are in range.
 * \todo Add error handling to the file I/O operations.
 *
 * \param fd The handle of the partition file.
 * \param second_indirect The block number of the second indirection block.
 * \param block_counters Pointer to an array of counters with one counter for each block. The
 * array is assumed to be properly sized and initialized.
 */
PRIVATE void find_second_indirection_blocks( int fd, uint32_t second_indirect, uint32_t *block_counters )
{
    uint8_t   workspace[BLOCKSIZE];
    uint32_t *block_numbers = ( uint32_t * )workspace; // Treat array as an array of uint32_t.
    int i;

    // The indirect block itself is being used, so count it.
    block_counters[second_indirect]++;

    lseek( fd, second_indirect * BLOCKSIZE, SEEK_SET );
    read( fd, workspace, BLOCKSIZE );

    // For every non-zero block number, process the indicated first indirection block.
    for( i = 0; i < BLOCKSIZE / sizeof( uint32_t ); ++i ) {
	if( block_numbers[i] == 0 ) break;
	find_first_indirection_blocks( fd, block_numbers[i], block_counters );
    }
}


//! Finds and counts the blocks associated with a given inode.
/*!
 * Scans over all the blocks "attached" to the given inode and increments their counts in the
 * given counter array. This function assumes block and inode numbers are all in range.
 *
 * \todo Check that block and inode numbers are in range.
 * \todo Add error handling to the file I/O operations.
 *
 * \param fd The handle of the partition file.
 * \param inode_number The inode number of the inode to analyze.
 * \param block_counters Pointer to an array of counters with one counter for each block. The
 * array is assumed to be properly sized and initialized.
 */
PRIVATE void find_inode_blocks( int fd, uint32_t inode_number, uint32_t *block_counters )
{
    uint8_t  workspace[BLOCKSIZE];
    struct gfs_inode *current_inode;
    int i;

    // Where is the inode in the inode table? Here "relative" means relative to the start of the
    // inode table. The first block in the inode table is relative block zero. The inode offset
    // is in terms of inode-sized unit. For example, an inode offset of 3 means a physical
    // offset of 3 * sizeof( struct gfs_inode ).
    
    uint32_t relative_block = inode_number / ( BLOCKSIZE / sizeof( struct gfs_inode ) );
    uint32_t inode_offset   = inode_number % ( BLOCKSIZE / sizeof( struct gfs_inode ) );

    // Get the necessary block from the inode table and point at the right inode.
    lseek( fd, ( 1 + 2*freemap_blocksize + relative_block ) * BLOCKSIZE, SEEK_SET );
    read( fd, workspace, BLOCKSIZE );
    current_inode =
        ( struct gfs_inode * )( workspace + ( inode_offset * sizeof( struct gfs_inode ) ) );

    // In what follows we assume that block numbers and indirection pointers are explicity
    // zeroed when they are not used (does the GenericFS specification require this?). No
    // information about the file's size is considered.

    // The direct blocks.
    for( i = 0; i < 4; ++i ) {
	if( current_inode->blocks[i] != 0 )
	    block_counters[current_inode->blocks[i]]++;
    }

    // The first indirect blocks.
    if( current_inode->first_indirect != 0 )
	find_first_indirection_blocks( fd, current_inode->first_indirect, block_counters );

    // The second indirect blocks.
    if( current_inode->second_indirect != 0 )
	find_second_indirection_blocks( fd, current_inode->second_indirect, block_counters );
}


//! Scan over all inodes and count blocks associated with each allocated inode.
/*!
 * The unallocated inodes are skipped. Any blocks associated with them are irrelevant.
 *
 * \param fd The handle of the parition file.
 * \param block_counters Pointer to an array of counters with one counter for each block. The
 * array is assumed to be properly sized and initialized.
 */
PRIVATE void scan_inodes( int fd, uint32_t *block_counters )
{
    uint8_t  workspace[BLOCKSIZE];
    uint32_t block_index;
    int      block_offset;
    uint32_t inode_number = 0;
    int      bit_number;

    printw( "Scanning inode " );

    // Process all blocks in the block freemap.
    for( block_index = 0; block_index < freemap_blocksize; ++block_index ) {

        // Read the freemap block.
        lseek( fd, (1 + block_index) * BLOCKSIZE, SEEK_SET );
	read( fd, workspace, BLOCKSIZE );

        // Process all bytes in the block. This loop bails out after all inodes are handled.
	for( block_offset = 0; block_offset < BLOCKSIZE; ++block_offset ) {

            // Look at every bit of this byte.
	    for( bit_number = 0; bit_number < 8; ++bit_number ) {
		if( workspace[block_offset] & ( 1 << bit_number ) ) {
		    printw( "%u, ", inode_number );  // The inode we are considering.
		    refresh( );
		    find_inode_blocks( fd, inode_number, block_counters );
		}

                // Bail out once all blocks have been handled. This assumes the number of blocks
                // on a partition equals the number of inodes on the partition.
		++inode_number;
		if( inode_number == block_count ) goto done;
	    }
	}
    }
 done:
    printw( "\n" );
}


//! Check block counters for appropriate values.
/*!
 * Verify that each block has been counted either zero or one times. Report on counts larger
 * than one as "multiple use" errors.
 *
 * \param block_counters Pointer to an array of counters. The value of each count reflects how
 * many times each block has been used in a file or in file system metadata.
 */
PRIVATE void check_block_counters( const uint32_t *block_counters )
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
PRIVATE void check_block_freemap( int fd, const uint32_t *block_counters )
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


//! Set the inode counters to appropriate initial values.
/*!
 * The inode counters are initialized to zero. The root directory is pre-allocated with two
 * links to it (the `.` and `..` entries in the root directory itself). However, these links
 * will be discovered automatically during the file system scan and don't need to be accounted
 * for here.
 *
 * \param inode_counters Pointer to an array of counters with one counter for each inode. The
 * array is assumed to be properly sized.
 */
PRIVATE void initialize_inode_counters( uint32_t *inode_counters )
{
    printw( "INTERNAL ERROR: `initialize_inode_counters` is not implemented!\n" );
}


//! Scans a single directory and counts the number of times each inode is mentioned.
/*!
 * This function walks down the linked list of directory entries for a single directory
 * specified by the given inode number. It increments the inode counter for every file and
 * directory mentioned in the list. For directories, this function calls itself recursively.
 *
 * \param fd The handle of the file system partition (already opened).
 * \param inode_number The inode number corresponding to the directory to be scanned.
 * \param inode_counters A pointer to an array of counters with one counter for each inode. The
 * array is assumed to be properly sized and initialized.
 */
PRIVATE void scan_directory( int fd, uint32_t inode_number, uint32_t *inode_counters )
{
    printw( "INTERNAL ERROR: `scan_directory` is not implemented!\n" );
}


//! Scans all directories and counts the number of times each inode is mentioned.
/*!
 * This function crawls the enitre file system, directory by directory. For each directory, it
 * walks the linked list of directory entries noting which inode is referenced by the directory
 * entries. The counter for an inode is incremented whenever that inode is encountered.
 *
 * \param fd The handle to the file system partition (already opened).
 * \param inode_counters A pointer to an array of counters with one counter for each inode. The
 * array is assumed to be properly sized and initialized.
 */
PRIVATE void scan_filesystem( int fd, uint32_t *inode_counters )
{
    // Start the scan at the root directory, which is always inode number zero.
    scan_directory( fd, 0, inode_counters );
}


//! Verifies that the inode counters are consistent with the nlinks field in each inode.
/*!
 * This Function ensures the inode counter for every inode agrees with the nlinks field inside
 * the inode.
 *
 * \param fd The handle to the file system paritition (already opened).
 * \param inode_counters A pointer to an array of counters with one counter for each inode. The
 * array is assumed to be properly sized and with count values that reflect the number of times
 * each inode is mentioned in the directory lists.
 */
PRIVATE void check_inode_counters( int fd, uint32_t *inode_counters )
{
    printw( "INTERNAL ERROR: `check_inode_counters` is not implemented!\n" );
}


//! Verifies that the inode counters are consistent with the inode freemap.
/*!
 * This function compares the inode counters with the inode freemap to ensure that every free
 * inode has a count of zero (is not mentioned in any directory), and that ever allocated inode
 * has a positive count (is mentioned in at least one directory).
 
 * \param fd The handle to the file system paritition (already opened).
 * \param inode_counters A pointer to an array of counters with one counter for each inode. The
 * array is assumed to be properly sized and with count values that reflect the number of times
 * each inode is mentioned in the directory lists.
 */
PRIVATE void check_inode_freemap( int fd, uint32_t *inode_counters )
{
    printw( "INTERNAL ERROR: `check_inode_freemap` is not implemented!\n" );
}


//! Implements the 'verify' option on the main menu.
/*!
 * Checks the give file system, assumed to be GenericFS, for internal consistency. This function
 * assumes the number of blocks and the number of inodes on the parition are the same. This is
 * (currently) required for GenericFS, but might change in the future.
 *
 * \param fd The handle to the file system partition (previously opened).
 */
void verify_file_system( int fd )
{
    uint32_t *counters;

    // Prepare the screen.
    clear( );
    move( 1, 1 );

    // Allocate an array of counters for both block and inode checking.
    counters = ( uint32_t * )malloc( block_count * sizeof( uint32_t ) );
    if( counters == NULL ) {
        printw( "FATAL ERROR: Unable to allocate counters!\n" );
        CONTINUE_MESSAGE;
        return;
    }

    printw( "\nBLOCK CHECKING\n" );
    initialize_block_counters( counters );
    scan_inodes( fd, counters );          // Count block usage.
    check_block_counters( counters );     // Verify every block is used exactly zero or one times.
    check_block_freemap( fd, counters );  // Verify consistency of the block freemap.

    // TODO:
    printw( "\nINODE CHECKING\n" );
    initialize_inode_counters( counters );
    scan_filesystem( fd, counters );      // Count inode usage.
    check_inode_counters( fd, counters ); // Verify inode counts agree with nlinks field.
    check_inode_freemap( fd, counters );  // Verify consistency of the inode freemap.

    free( counters );
    
    // Bump up the content of the screen so the continue message doesn't overwrite the last
    // line output by the checking process.
    printw( "\n" );    
    CONTINUE_MESSAGE;
}
