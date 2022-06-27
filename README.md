
GenericFS
=========

This repository contains GenericFS, an educational file system for Linux. The purpose of
GenericFS is to serve as a vehicle for learning about file systems and about Linux kernel
development. GenericFS does not support any special or unusual features (hence "generic"),
although there is some information in the `doc` folder about how additional features might be
implemented as future exercises.

GenericFS is currently unfinished and is not usable as a real file system.

The contents of this folder are as follows.

+ disktool: A user mode application that allows unmounted GenericFS partitions to be inspected
  and manipulated. This tool includes formatting and file system checking functionality.

+ doc: The documentation for the GenericFS system. This folder contains detailed documentation
  for the file system as well as information about implementing file systems in general. Since
  the purpose of GenericFS is educational the documentation set also includes general
  information about Linux kernel development.

+ driver: The GenericFS file system driver. This is a Linux kernel module.

+ FAT: A program that allows one to manipulate a simple FAT file system as implemented in an
  ordinary file in the host file system. This is an educational sample only. It is not directly
  related to GenericFS.

+ shared: This folder contains source files that are shared between the disktool and the driver
  components.
  
+ tools: Various programs of use during the development and testing of GenericFS.

Although GenericFS currently only supports Linux, it would be possible (and interesting) to
create a GenericFS driver for some other operating systems as well. That is a possible area for
future work.

Peter Chapin  
pchapin@vtc.edu
