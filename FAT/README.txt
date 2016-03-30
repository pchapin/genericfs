
README.txt
==========

The program contained in this folder is a simulation of a FAT file system driver. The executable
named "shell" allows the user to do basic operations on a file system contained in a single file
named "block.dev." Run "shell" and it will automatically open (or create if necessary) a 512 KiB
block.dev file. Use the "format" command inside shell to format that file system and "exit" to
leave the shell. See the source of shell.cpp for a list of legal commands.

This program is written in C++ with project definition files for Open Watcom. It should compile
straight forwardly with any other C++ compiler.
