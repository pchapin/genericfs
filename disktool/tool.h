/*!
 * \file tool.h
 * \author Peter Chapin <spicacality@kelseymountain.org>
 *
 * \brief This header declares all the global data and implementation functions used by the
 * disktool program. The other .c files in the disktool project should only have to include this
 * header. Notice that this header includes the genericfs.h header used by the file system
 * driver. This is done so the on-disk data structures will be used consistently by both the
 * disktool and the driver.
 */

#ifndef TOOL_H
#define TOOL_H

#include <stdint.h>

#include "genericfs.h"

// ==========================
//           Macros
// ==========================

// Used at the end of the processing of each menu option.
#define CONTINUE_MESSAGE { \
                           mvaddstr( LINES - 1, 1, "Hit RETURN to continue..." ); \
                           refresh( ); \
                           while( getch( ) != '\r' ) ; \
                         }

// ===============================
//           Global Data
// ===============================

extern uint32_t block_count;           // Size of partition in blocks.
extern uint32_t freemap_bytesize;      // Size of the freemap in bytes.
extern uint32_t freemap_blocksize;     // Size of the freemap in blocks.
extern uint32_t inodetable_bytesize;   // Size of the inode table in bytes.
extern uint32_t inodetable_blocksize;  // Size of the inode table in blocks.

// ============================================
//           Implementation Functions
// ============================================

void create_dir( int fd );
void create_file( int fd );
void initialize( int fd );
void show_inode_freemap( int fd );
void show_block_freemap( int fd );
void show_inode( int fd );
void show_block( int fd );
void show_file( int fd );
void show_root_dir( int fd );
void verify_file_system( int fd );

// =====================================
//           Utility Functions
// =====================================

// Free map utilities.
uint32_t allocate_block( int fd );
uint32_t allocate_inode( int fd );

// File name checking.
int valid_filename( const char *name );

// Endianness management.
uint32_t htod32( uint32_t value );
uint32_t dtoh32( uint32_t value );

// Directory management.
int name_exists( int fd, uint32_t dir_inode, const char *name );
int add_entry( int fd, uint32_t dir_inode, const char *name, uint32_t inode );
int check_consistency( int fd, uint32_t dir_inode );

uint8_t *get_directory( int fd, struct gfs_inode *dir_inode );

#endif
