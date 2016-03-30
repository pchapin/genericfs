/****************************************************************************
FILE   : list.c
SUBJECT: Program to list the files in the current folder.
AUTHOR : (C) Copyright 2011 by Peter C. Chapin

This program does what ls does but without the frills.

Please send comments or bug reports to

     Peter C. Chapin
     Computer Information Systems
     Vermont Technical College
     Randolph Center, VT 05061
     PChapin@vtc.vsc.edu
****************************************************************************/

#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

int main( void )
{
    int return_code = EXIT_SUCCESS;
    DIR *current_directory = opendir(".");
    struct dirent *entry;

    if( current_directory == NULL ) {
        perror("Unable to open the current directory for scanning");
        return_code = EXIT_FAILURE;
    }
    else {
        errno = 0;

        // Scan the directory.
        while( (entry = readdir( current_directory )) != NULL ) {
            printf("name   = %s\n"
                   "inode# = %d\n"
                   "offset = %d\n"
                   "length = %u\n"
                   "type   = %u\n\n",
                   entry->d_name, entry->d_ino, entry->d_off, entry->d_reclen, entry->d_type);
        }

        // Did the above loop end because of an error?
        if( errno != 0 ) {
            perror("Error while scanning the directory");
            return_code = EXIT_FAILURE;
        }

        // Check to make sure nothing strange happens when closing the scan.
        if( closedir( current_directory ) == -1 ) {
            perror("Error while closing the directory");
            return_code = EXIT_FAILURE;
        }
    }
        
    return return_code;
}
