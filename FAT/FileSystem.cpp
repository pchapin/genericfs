/*! \file    FileSystem.cpp
    \brief   File system support code.
    \author  Peter C. Chapin <PChapin@vtc.vsc.edu>

*/

#include <cstring>

#include "BlockDevice.hpp"
#include "FileSystem.hpp"

//========================================
//           Provided Functions
//========================================

//
// FileSystem::flush()
//
// This function updates the disk so that all cached data structures are saved.
//
void FileSystem::flush()
{
    if (formatted_flag) {
        the_disk.write(FAT_BLOCK, reinterpret_cast<char *>(FAT));
        the_disk.write(ROOT_BLOCK, reinterpret_cast<char *>(root_directory));
    }
}


//
// FileSystem::FileSystem
//
// The constructor verifies that the given block device is proper and it checks to see if the
// file system is formatted.
//
FileSystem::FileSystem(BlockDevice &disk) : the_disk(disk)
{
    // For now, let's insure that we are dealing with BLOCK_SIZE sized blocks. This version of
    // the FileSystem class will assume that size. Perhaps in a future version we can lift that
    // assumption and have the code work with whatever sized blocks it happens to find.
    // 
    if (the_disk.blk_size() != BLOCK_SIZE)
        throw "Can't manage a file system on this disk. The block size is wrong!";

    if (the_disk.blk_count() < 4)
        throw "Can't manage a file system on this disk. Not enough blocks!";

    // Is this file system formatted? Read the boot block and find out.
    char buffer[BLOCK_SIZE];
    the_disk.read(BOOT_BLOCK, buffer);

    // Assume it is not formatted.
    formatted_flag = false;

    // Now look at it. Does it look right?
    if (buffer[0] == FORMATTED) {
      
        // Looks good so far. Let's verify the checksum.
        unsigned char sum = 0;
        for (int i = 0; i < BLOCK_SIZE; i++) {
            sum += buffer[i];
        }
        if (sum == 0) formatted_flag = true;
    }

    // If the file system is formatted, get the important data structures.
    if (formatted_flag) {
        the_disk.read(FAT_BLOCK, reinterpret_cast<char *>(FAT));
        the_disk.read(ROOT_BLOCK, reinterpret_cast<char *>(root_directory));
    }

    // Finally, let's initialize the handle_table to make sure that all slots in it are
    // available for use.
    // 
    for (int i = 0; i < HANDLETABLE_SIZE; i++) {
        handle_table[i].in_use = false;
    }
}


//
// FileSystem::~FileSystem
//
// The destructor flushes the FAT and root directory out to disk. This insures that the
// information on disk agrees with what it is supposed to be.
//
FileSystem::~FileSystem()
{
    flush();
}


//
// FileSystem::format
//
// This function formats the file system by initializing the various data structures.
// 
void FileSystem::format()
{
    char buffer[BLOCK_SIZE];
    int  i;

    // Create a valid boot block.
    for (i = 0; i < BLOCK_SIZE; i++) {
        buffer[i] = 0;
    }

    buffer[0] = FORMATTED;

    // Compute a checksum.
    unsigned char sum = 0;
    for (i = 0; i < BLOCK_SIZE - 1; i++) {
        sum += buffer[i];
    }
    buffer[BLOCK_SIZE-1] = -sum;

    // Save the new boot block to disk.
    the_disk.write(BOOT_BLOCK, buffer);

    // Build a valid FAT.
    std::memset(FAT, 0, sizeof(FAT));
    for (i = 0; i < BLOCK_SIZE/sizeof(block_number); i++) {
        FAT[i] = FREE_FAT_ENTRY;
    }
    FAT[BOOT_BLOCK] = RESERVED_FAT_ENTRY;
    FAT[FAT_BLOCK]  = RESERVED_FAT_ENTRY;
    FAT[ROOT_BLOCK] = RESERVED_FAT_ENTRY;

    // Build a valid root directory.
    std::memset(root_directory, 0, sizeof(root_directory));
    for (i = 0; i < BLOCK_SIZE/sizeof(directory_entry); i++) {
        root_directory[i].in_use = 0;
    }

    formatted_flag = true;
}


//
// FileSystem::close
//
// This function closes a previously opened file. I'm assuming that all required changes to the
// directory have been made during the file I/O. This function does not attempt to update the
// directory.
//
void FileSystem::close(int handle)
{
    if (formatted_flag == false)
        throw "Attempted to close a file on an unformatted file system.";

    if (handle >= HANDLETABLE_SIZE)
        throw "Invalid handle used during close(). Handle out of range.";

    if (handle_table[handle].in_use == false)
        throw "Invalid handle used during close(). Handle not open.";

    handle_table[handle].in_use = false;
}


//
// free_space
//
// This function computes the amount of free space on the disk and returns that number (of
// bytes).
//
long FileSystem::free_space()
{
    int  i;
    long count = 0;

    if (formatted_flag == false)
        throw "Attempted to ask for free space on an unformatted file system.";

    // Scan the FAT looking for free blocks.
    for (i = 0; i < sizeof(FAT)/sizeof(block_number); i++) {
        if (FAT[i] == FREE_FAT_ENTRY) count++;
    }

    return count*BLOCK_SIZE;
}


//================================================
//           Write one of the following.
//================================================

int FileSystem::read(int handle, char *buffer, int count)
{
    const int han_size = sizeof(handle_table)/sizeof(handletable_entry);
    const int dir_size = sizeof(root_directory)/sizeof(directory_entry);
    const int FAT_size = sizeof(FAT)/sizeof(block_number);

    // Validate the handle.
    if (handle < 0 || handle >= han_size)
        throw "FileSystem::read() -- Invalid handle";

    if (!handle_table[handle].in_use || handle_table[handle].mode != READ)
        throw "FileSystem::read() -- Handle not opened for reading";

    // Adjust the count.
    int file_size = root_directory[handle_table[handle].directory_index].size;
    if (file_size - handle_table[handle].offset < count)
        count = file_size - handle_table[handle].offset;
    
    // Are we already at the EOF?
    if (count == 0) return 0;

    // Now let's loop to get 'count' bytes. We know they have to be there.
    char block_buffer[BLOCK_SIZE];
    int  block_offset = handle_table[handle].offset % BLOCK_SIZE;
    int  block_count  = BLOCK_SIZE - block_offset;
    the_disk.read(handle_table[handle].current_block, block_buffer);
    for (int i = 0; i < count; i++) {
        *buffer++ = block_buffer[block_offset++];
        handle_table[handle].offset++;

        // If that's the last byte in this block, fetch the next one.
        if (--block_count == 0) {
            handle_table[handle].current_block =
                FAT[handle_table[handle].current_block];
            the_disk.read(handle_table[handle].current_block, block_buffer);
            block_offset = 0;
            block_count  = BLOCK_SIZE;
        }
    }

    return count;
}


int FileSystem::write(int handle, const char *buffer, int count)
{
    const int han_size = sizeof(handle_table)/sizeof(handletable_entry);
    const int dir_size = sizeof(root_directory)/sizeof(directory_entry);
    const int FAT_size = sizeof(FAT)/sizeof(block_number);
    
    // Validate the handle.
    if (handle < 0 || handle >= han_size)
        throw "FileSystem::write() -- Invalid handle";

    if (!handle_table[handle].in_use || handle_table[handle].mode != WRITE)
        throw "FileSystem::write() -- Handle not opened for writing";

    // Adjust the count.
    int file_size   = root_directory[handle_table[handle].directory_index].size;
    int slack_space = BLOCK_SIZE - (file_size % BLOCK_SIZE);
    int open_space  = free_space() + slack_space - 1;
    if (open_space < count) count = open_space;
    
    // Is there any more space?
    if (count == 0) return 0;

    // Now let's loop to put 'count' bytes. We know there is space.
    char block_buffer[BLOCK_SIZE];
    int  block_offset = handle_table[handle].offset % BLOCK_SIZE;
    int  block_count  = BLOCK_SIZE - block_offset;
    the_disk.read(handle_table[handle].current_block, block_buffer);
    for (int i = 0; i < count; i++) {
        block_buffer[block_offset++] = *buffer++;
        handle_table[handle].offset++;
        root_directory[handle_table[handle].directory_index].size++;

        // If that's the last byte in this block, get a new one.
        if (--block_count == 0) {
            the_disk.write(handle_table[handle].current_block, block_buffer);
            
            // Find a free block. There must be one.
            int j;
            for (j = 0; j < FAT_size; j++) {
                if (FAT[j] == FREE_FAT_ENTRY) break;
            }
            if (j == FAT_size)
                throw "FileSystem::write() -- Can't locate a free block, but one expected";

            FAT[handle_table[handle].current_block] = j;
            FAT[j] = EOF_FAT_ENTRY;
            handle_table[handle].current_block = j;
            block_offset = 0;
            block_count  = BLOCK_SIZE;
        }
    }

    // Put the last, partially filled block back on the disk.
    the_disk.write(handle_table[handle].current_block, block_buffer);
    return count;
}


//================================================
//           Write one of the following.
//================================================

int FileSystem::open(const char *name, open_mode mode)
{
    const int han_size = sizeof(handle_table)/sizeof(handletable_entry);
    const int dir_size = sizeof(root_directory)/sizeof(directory_entry);
    const int FAT_size = sizeof(FAT)/sizeof(block_number);

    int handle;
    int dir_index;
    int FAT_index;

    // Locate a free handle table entry.
    for (handle = 0; handle < han_size; handle++) {
        if (!handle_table[handle].in_use) break;
    }

    if (handle == han_size)
        throw "FileSystem::open() -- Out of available handles";

    // Now locate the proper root directory entry. Search for an existing entry.
    for (dir_index = 0; dir_index < dir_size; dir_index++) {
        if (root_directory[dir_index].in_use == 0) continue;
        
        if (std::strcmp(root_directory[dir_index].name, name) == 0) break;
    }

    // If we didn't find the name, take appropriate action.
    if (dir_index == dir_size) {
        if (mode == READ) throw "FileSystem::open() -- File does not exist";

        // mode is write. Try to create the file.
        for (dir_index = 0; dir_index < dir_size; dir_index++) {
            if (root_directory[dir_index].in_use == 0) break;
        }

        // Can't find a free slot.
        if (dir_index == dir_size)
            throw "FileSystem::open() -- Unable to create file. No space in root directory";

        // Find a free slot in the FAT for the file's first block.
        for (FAT_index = 0; FAT_index < FAT_size; FAT_index++) {
            if (FAT[FAT_index] == FREE_FAT_ENTRY) break;
        }

        // Can't find one.
        if (FAT_index == FAT_size)
            throw "FileSystem::open() -- Unable to create file. Not enough disk space";

        // Finally, we are ready to fill in the fields of the various data structures.
        // 
        FAT[FAT_index] = EOF_FAT_ENTRY;

        root_directory[dir_index].in_use         = 1;
        root_directory[dir_index].starting_block = FAT_index;
        root_directory[dir_index].size           = 0;
        std::strcpy(root_directory[dir_index].name, name);

        handle_table[handle].offset          = 0;
        handle_table[handle].directory_index = dir_index;
        handle_table[handle].current_block   = FAT_index;
        handle_table[handle].in_use          = true;
        handle_table[handle].mode            = mode;
    }
    
    // We found the name in the directory. What happens next depends on the open mode.
    // 
    else {
        if (mode == READ) {
            handle_table[handle].offset          = 0;
            handle_table[handle].directory_index = dir_index;
            handle_table[handle].current_block   = root_directory[dir_index].starting_block;
            handle_table[handle].in_use          = true;
            handle_table[handle].mode            = READ;
        }
        else {
            handle_table[handle].offset          = root_directory[dir_index].size;
            handle_table[handle].directory_index = dir_index;
            handle_table[handle].in_use          = true;
            handle_table[handle].mode            = WRITE;

            // We have to locate the last block in the file.
            block_number current = root_directory[dir_index].starting_block;
            while (FAT[current] != EOF_FAT_ENTRY) {
                current = FAT[current];
            }
            handle_table[handle].current_block   = current;
        }
    }
    return handle;
}

void FileSystem::truncate(const char *name)
{
    int index;

    // Locate the file in the root directory.
    for (index = 0; index < sizeof(root_directory)/sizeof(directory_entry); index++) {
        if (root_directory[index].in_use == 0) continue;
        
        // If we found it...
        if (std::strcmp(root_directory[index].name, name) == 0) {

            // Scan the FAT and mark all the blocks as free.
            block_number current_block = root_directory[index].starting_block;
            while (FAT[current_block] != EOF_FAT_ENTRY) {
                block_number next  = FAT[current_block];
                FAT[current_block] = FREE_FAT_ENTRY;
                current_block      = next;
            }
            FAT[current_block] = FREE_FAT_ENTRY;
            FAT[root_directory[index].starting_block] = EOF_FAT_ENTRY;
            root_directory[index].size = 0;
            break;
        }
    }
}

void FileSystem::remove(const char *name)
{
    int index;

    // Locate the file in the root directory.
    for (index = 0; index < sizeof(root_directory)/sizeof(directory_entry); index++) {
        if (root_directory[index].in_use == 0) continue;
        
        // If we found it...
        if (std::strcmp(root_directory[index].name, name) == 0) {

            // Scan the FAT and mark all the blocks as free.
            block_number current_block = root_directory[index].starting_block;
            while (FAT[current_block] != EOF_FAT_ENTRY) {
                block_number next  = FAT[current_block];
                FAT[current_block] = FREE_FAT_ENTRY;
                current_block      = next;
            }
            FAT[current_block] = FREE_FAT_ENTRY;
            root_directory[index].in_use = 0;
            break;
        }
    }
}


//=========================================
//           Write the following.
//=========================================

bool FileSystem::next_dir(directory_info *info)
{
    while (scan_index < sizeof(root_directory)/sizeof(directory_entry)) {

        // Is this directory entry actually being used?
        if (root_directory[scan_index].in_use == 0) {

            // If not, just advance to the next one.
            scan_index++;
        }
        else {

            // It was! Copy the good information out of it for the caller.
            std::strcpy(info->name, root_directory[scan_index].name);
            info->size = root_directory[scan_index].size;
            scan_index++;
            return true;
        }
    }
    
    return false;
}
