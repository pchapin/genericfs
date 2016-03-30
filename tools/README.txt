
README.txt
==========

This folder contains some small programs that might be useful for testing and debugging
GenericFS. They interact with the file system in well defined, limited ways.

list
    This program generates a directory listing. It does not attempt to stat any of the files
    in the directory but it does display the entire struct dirent for each directory entry.
    It also has comprehensive error detection.
