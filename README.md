
GenericFS
=========

This repository contains GenericFS, an educational file system for Linux. The contents of this
folder are as follows.

+ disktool: A user mode application that allows unmounted GenericFS partitions to be inspected
  and manipulated. This tool includes formatting and file system checking functionality.

+ doc: The documentation for the GenericFS system. This folder contains detailed documentation
  for the file system as well as information about implementing file systems in general. Since
  the purpose of GenericFS is educational the documentation set includes significant information
  about Linux kernel development as well.

+ driver: The GenericFS file system driver. This is a Linux kernel module.

+ FAT: A program that allows one to manipulate a simple FAT file system as implemented in an
  ordinary file in the host file system. This is an educational sample only. It is not directly
  related to GenericFS.

+ shared: This folder contains source files that are shared between the disktool and the driver
  components.
  
+ tools: Various programs of use during the development and testing of GenericFS.

Peter C. Chapin  
PChapin@vtc.vsc.edu
