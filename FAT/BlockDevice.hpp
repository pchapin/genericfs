/*! \file    BlockDevice.hpp
    \brief   Simulated block device.
    \author  Peter C. Chapin <PChapin@vtc.vsc.edu>

This software is part of a file system simulation package for use at Vermont Technical College.
This module simulates a raw block device by creating a file in the hosting file system.

This class should really be an abstract base class with derived classes supporting different
backing methods. Doing that would be too much like work, so I'm not going to bother right now.
Maybe later.
*/

#ifndef BLOCKDEVICE_HPP
#define BLOCKDEVICE_HPP

#include <fstream>

class BlockDevice {
private:
    std::fstream backing_file; // We will simulate our block device in a file.
    const int block_size;      // How large are the blocks?
    const int block_count;     // How many blocks?

    BlockDevice &operator=(const BlockDevice &);
    BlockDevice(const BlockDevice &);
      // Make these members private so that we disable copying.

public:
    BlockDevice(const char *name, int size, int count);
      // The name will be used as the file name for the backing file. If the file already
      // exists, it will be used. If its size does not agree with the requested size (size *
      // count bytes), the block_ device object will put itself into an error state. In that
      // case, read() and write() will always fail.

    int  blk_size() { return block_size; }
    int  blk_count() { return block_count; }
      // These two functions allow clients to find out our dimensions.

    void read(int block_number, char *block_buffer);
    void write(int block_number, const char *block_buffer);
      // These two operations are basically the only ones a BlockDevice needs to worry about. It
      // is not concerned with the meaning of the data in the blocks that it is manipulating. It
      // never reads or writes data in units with a size different than one block.
};

#endif
