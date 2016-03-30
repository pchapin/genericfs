/*! \file    BlockDevice.cpp
    \brief   Simulated block device.
    \author  Peter C. Chapin <PChapin@vtc.vsc.edu>

This software is part of a file system simulation package for use at Vermont Technical College.
This module simulates a raw block device by creating a file in the hosting file system.
*/

#include "environ.hpp"

#include <cstdlib>
#include <iostream>

#if eOPSYS == eOS2
#define INCL_DOSERRORS
#define INCL_DOSFILEMGR
#include <os2.h>
#endif

#if eOPSYS == eWIN32
#include <windows.h>
#endif

#include "BlockDevice.hpp"

#if eOPSYS == eOS2

//
// Check_Size
//
// This helper function finds out if the given file exists. If so, it returns the file's size.
// It will return -1 if the file does not exist.
//
static long check_file(const char *name)
{
    // We need to figure out how big the file is. This is the way it's done using OS/2.

    FILESTATUS3 file_info;   // Structure to hold the result.
    APIRET      return_code; // To hold the result of the API function.

    return_code = DosQueryPathInfo(name, 1, &file_info, sizeof(FILESTATUS3));
    if (return_code == ERROR_FILE_NOT_FOUND) return -1;
    if (return_code != 0)
        throw "Unexpected error occured when searching for the backing file";
 
    return file_info.cbFile;
}

#endif

#if eOPSYS == eWIN32

//
// Check_Size
//
// This helper function finds out if the given file exists. If so, it returns the file's size.
// It will return -1 if the file does not exist.
//
static long check_file(const char *name)
{
    // We need to figure out how big the file is. This is the way it's done using Win32.

    WIN32_FIND_DATA file_information;
    HANDLE          search_handle;

    search_handle = FindFirstFile(name, &file_information);
    if (search_handle == INVALID_HANDLE_VALUE) return -1;

    // Don't worry about the possibility of very large backing files.
    return file_information.nFileSizeLow;
}

#endif


//
// BlockDevice::BlockDevice
//
// The constructor verifies that the given backing file is there. It creates it if necessary. It
// also initializes the various other members to sensible values.
//
BlockDevice::BlockDevice(const char *name, int size, int count) :
    block_size(size), block_count(count)
{
    long existing_size;

    // Is the file already there?
    if ((existing_size = check_file(name)) != -1) {

        if (existing_size != static_cast<long>(size) * static_cast<long>(count))
            throw "Bad backing file selected. Size of file is wrong";

        backing_file.open(name, std::ios::in | std::ios::out | std::ios::binary);
        if (!backing_file)
            throw "Unable to open the backing file. Cause unknown";
    }

    else {
        // ... the file is not there already. Let's create it and be sure it has the right size.
        // This operation is sort of like doing a low level format on a real hard disk.

        backing_file.open(name, std::ios::in | std::ios::out | std::ios::trunc | std::ios::binary);
        if (!backing_file)
            throw "Unable to create the backing file. Cause unknown";

        // Write an appropriate number of zeros into the file.
        for (long i = 0; i < static_cast<long>(block_size) * static_cast<long>(block_count); i++)
            backing_file.put((unsigned char)(0));

        if (!backing_file)
            throw "Unable to create the backing file. Insufficient disk space?";
    }
}


//
// BlockDevice::read
//
// This does the obvious thing.
//
void BlockDevice::read(int block_number, char *block_buffer)
{
    // Is this block on the disk?
    if (block_number < 0 || block_number >= block_count)
        throw "Attempt to read an invalid block by a block device";

    backing_file.seekg(block_number * block_size);
    backing_file.read(block_buffer, block_size);
}


//
// BlockDevice::write
//
// This does the obvious thing
//
void BlockDevice::write(int block_number, const char *block_buffer)
{
    // Is this block on the disk?
    if (block_number < 0 || block_number >= block_count)
        throw "Attempt to write an invalid block by a block device";

    backing_file.seekp(block_number * block_size);
    backing_file.write(block_buffer, block_size);
}
