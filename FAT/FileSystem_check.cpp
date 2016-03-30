/*! \file    FileSystem_check.cpp
    \brief   Implementation of FileSystem::check()
    \author  Peter C. Chapin <PChapin@vtc.vsc.edu>

*/

#include "FileSystem.hpp"

//
// Check
//
// Verify that the file system is consistent. This function does a number of checks on the file
// system. It throws an exception if it finds a problem. (This might not be appropriate since
// finding a problem is a "normal" thing for this function to do. It is not really exceptional).
//
// Note that this function does not attempt to repair a damaged file system. That is something
// for version 2.0, I guess.
//
void FileSystem::check()
{
    int i;

    // Don't even bother if there is no file system to check.
    if (formatted_flag == false)
        throw "file_system::check() -- Unformatted file system";
    
    // First let's verify that there are no files open for writing.
    for (i = 0; i < sizeof(handle_table)/sizeof(handletable_entry); i++) {
        if (handle_table[i].in_use && handle_table[i].mode == WRITE)
            throw "file_system::check() -- Files open for writing";
    }

    // For each file in the root directory, let's verify it's size.
    for (i = 0; i < sizeof(root_directory)/sizeof(directory_entry); i++) {

      // If we have a file, then count the number of blocks allocated to it.
        if (root_directory[i].in_use == 1) {
            long         size          = root_directory[i].size;
            long         block_count   = 0;
            block_number current_block = root_directory[i].starting_block;

            while (FAT[current_block] != EOF_FAT_ENTRY) {
                
                // The blocks are either reserved or EOF. If we come to a free block, then we
                // have a problem.
                //
                if (FAT[current_block] == FREE_FAT_ENTRY)
                    throw "file_system::check() -- Unreserved FAT block in a file's chain";

                block_count++;
                current_block = FAT[current_block];
            }

            // Does the number of blocks allocated to this file make sense?
            if (size/BLOCK_SIZE != block_count)
                throw "file_system::check() -- A file has an invalid size";
        }
    }

    // Now let's see if we can locate lost chains and cross linked files.
    bool check_off[sizeof(FAT)/sizeof(block_number)];

    // Check off the blocks that are used by files (and reserved structures).
    for (i = 0; i < sizeof(FAT)/sizeof(block_number); i++) {
        check_off[i] = false;
        if (FAT[i] == RESERVED_FAT_ENTRY) check_off[i] = true;
        if (FAT[i] == FREE_FAT_ENTRY)     check_off[i] = true;
    }

    // Scan over all the files again
    for (i = 0; i < sizeof(root_directory)/sizeof(directory_entry); i++) {

        // If we have a file, then check off all the blocks used by that file.
        if (root_directory[i].in_use == 1) {
            block_number current_block = root_directory[i].starting_block;

            while (FAT[current_block] != EOF_FAT_ENTRY) {
                if (check_off[current_block] == true)
                    throw "file_system::check() -- Cross linked files detected";
                check_off[current_block] = true;
                current_block = FAT[current_block];
            }

            // The EOF block is also being used.
            if (check_off[current_block] == true)
                throw "file_system::check() -- Cross linked files detected on a file EOF";
            check_off[current_block] = true;
        }
    }

    // Any unchecked blocks? If so, they are lost.
    for (i = 0; i < sizeof(FAT)/sizeof(block_number); i++) {
        if (check_off[i] == false)
            throw "file_system::check() -- Lost chain detected";
    }
}
