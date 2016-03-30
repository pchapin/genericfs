/*! \file    FileSystem.hpp
    \brief   File system support code.
    \author  Peter C. Chapin <PChapin@vtc.vsc.edu>

This file is part of a file system simulator program for use at Vermont Technical College. It
calls on the services of the BlockDevice class to read and write "raw" blocks from a
hypothetical mass storage device. It then manipulates those bytes the way a real file system
module might and provides services like opening files, reading files, scanning directories, and
so forth.

This code throws (char *) exceptions when it encounters errors.
*/

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include "BlockDevice.hpp"

class FileSystem {

public:

    // +++++
    // Public types.
    // +++++

    // This structure is used by clients when they try to scan the root directory. Information
    // about each file is returned into an object of this type.
    // 
    struct directory_info {
        char name[24];
        long size;
    };

    enum open_mode { READ, WRITE };
      // A real file system will support more modes than just this.

  private:

    // +++++
    // Private types.
    // +++++

    typedef unsigned short block_number;
      // This type used to hold block numbers.

    // The following structure defines a directory entry. It's size is precisely 32 bytes. At
    // least that is our hope! The hard coded value for the length of the name is bad. It will
    // be okay for now.
    // 
    struct directory_entry {
        char         name[24];        // The name of the file.
        long         size;            // The exact size of the file.
        block_number starting_block;  // Where the file is on disk.
        char         in_use;          // =1 if this directory entry is used.
        char         pad;             // To make this structure 32 bytes.

        // In general it would be nice to support various date/times and file attributes as
        // well.
    };

    // This structure is used to keep track of an open file. When a file is opened, one of these
    // structures is filled in. It is maintained for as long as the file is open.
    // 
    struct handletable_entry {
        long         offset;          // The current file pointer position.
        int          directory_index; // Index into the root directory.
        block_number current_block;   // The file pointer points into this block.
        bool         in_use;          // =true if this entry is used.
        open_mode    mode;            // File open for reading or writing?
    };

    // +++++
    // Class specific global data.
    // +++++

    static const int BLOCK_SIZE = 1024;

    static const block_number BOOT_BLOCK = 0;
    static const block_number FAT_BLOCK = 1;
    static const block_number ROOT_BLOCK = 2;
      // These values are the block numbers of the fixed data structures. In this version, these
      // structures are all exactly one block in size.

    static const block_number FREE_FAT_ENTRY = 0;
    static const block_number RESERVED_FAT_ENTRY = 1;
    static const block_number EOF_FAT_ENTRY = 2;
      // These special values are used in the FAT for special purposes. Their values are small,
      // positive integers that are not being used for any valid data block.

    static const unsigned char FORMATTED = 0x6E;
      // This "status byte" value means the disk is formatted for our wonderful file system. The
      // status byte is the first byte of the boot block. All eight bits are significant. This
      // reduces the chance that random data on an unformatted disk will cause us to think that
      // the file system is formatted.

    static const int HANDLETABLE_SIZE = 16;
      // The maximum number of files that can be open at once.

    // +++++
    // Private data members.
    // +++++

    BlockDevice &the_disk;
      // Technically, it need not be a disk. Any block device will do.

    bool formatted_flag;
      // =true if the file system appears to be formatted. This is set by the constructor and
      // updated by the format() member function.

    block_number FAT[BLOCK_SIZE/sizeof(block_number)];
    directory_entry root_directory[BLOCK_SIZE/sizeof(directory_entry)];
      // The two critical data structures will be held in memory all the time. This would be
      // impractical for any realistically sized file system.

    handletable_entry handle_table[HANDLETABLE_SIZE];
      // This array holds information about all open files.

    int scan_index;
      // Used during a directory scan. This object holds the index into the root directory of
      // the next directory entry to consider when next_dir() is called. Because there is only
      // one of these scan_index objects per filesystem, only a single scan can be going on at
      // any one time. This is not realistic, but it will do for now.

    // +++++
    // Private member functions.
    // +++++

    void flush();
      // Write cached data to disk.


  public:

    // +++++
    // Public member functions.
    // +++++

    FileSystem(BlockDevice &);
      // A file system needs a block device to put itself into. This BlockDevice object must be
      // constructed for the entire time the file system object is constructed. (Of course!)

   ~FileSystem();
      // Do what must be done to shut down the file system.

    bool is_formatted()
      { return formatted_flag; }
      // Returns true if the file system appears to be properly formatted.

    void format();
      // Formats the file system. This is called "making a file system" in some cultures.

    int open(const char *name, open_mode mode);
      // Open a file with the given name. Returns the file's handle. If a file is opened for
      // writing, it should be opened in "append" mode (new material goes on the end). It should
      // be created if it does not exist. Only sequential access is supported. This version of
      // the FileSystem class does not support random access files. This function will throw an
      // exception if an error occurs.

    void truncate(const char *name);
      // Truncates an existing file to zero size. If the file does not exist, this function does
      // nothing. If this function is applied to a file that is already open, the effect is
      // undefined.

    void close(int handle);
      // Close a previously opened file.

    int read(int handle, char *buffer, int count);
      // Reads a previously opened file. Looks a lot like the Unix system call, doesn't it!
      // Returns the number of bytes actually read. Returns zero if we are at the end of the
      // file.

    int write(int handle, const char *buffer, int count);
      // Writes a previously opened file. Another Unix-like operation here. Returns the number
      // of bytes actually written. Returns zero if the disk has filled up.

    void remove(const char *name);
      // Probably should support deleting files too. If this function is applied to a file that
      // is open, the effect is undefined.

    void open_dir()
      { scan_index = 0; }
      // Prepares the root directory for a scan.

    bool next_dir(directory_info *);
      // Returns information about the "next" directory entry in a directory scan. If open_dir()
      // was *not* called before this function is called (or after a scan has ended) the effects
      // are undefined. This function returns true if it got a directory entry. It returns false
      // if there are no directory entries left to get.

    long free_space();
      // Returns the number of free bytes on the disk.

    void check();
      // This function checks the file system for consistency. It throws an exception if it
      // finds a problem (is that really a good idea?), otherwise it just returns without
      // comment.
};

#endif
