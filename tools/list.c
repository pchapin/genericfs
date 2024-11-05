/*!
 * \file list.c
 * \author Peter Chapin <spicacality@kelseymountain.org>
 *
 * \brief This program is a highly simplified version of the `ls`. It does not use the `stat`
 * system call and so will work even in cases where `stat` is not implemented or is
 * malfunctioning. This program only requires the ability to read a directory to be implemented
 * by the file system. It is useful for testing GenericFS before the full functionality of the
 * file system is finished.
 */

#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

int main( void )
{
    int return_code = EXIT_SUCCESS;
    DIR *current_directory = opendir( "." );
    struct dirent *entry;

    if( current_directory == NULL ) {
        perror("Unable to open the current directory for scanning");
        return_code = EXIT_FAILURE;
    }
    else {
        errno = 0;

        // Scan the directory.
        while( (entry = readdir( current_directory ) ) != NULL ) {
            printf("name   = %s\n"
                   "inode# = %d\n"
                   "offset = %d\n"
                   "length = %u\n"
                   "type   = %u\n\n",
                   entry->d_name, entry->d_ino, entry->d_off, entry->d_reclen, entry->d_type );
        }

        // Did the above loop end because of an error?
        if( errno != 0 ) {
            perror( "Error while scanning the directory" );
            return_code = EXIT_FAILURE;
        }

        // Check to make sure nothing strange happens when closing the scan.
        if( closedir( current_directory ) == -1 ) {
            perror( "Error while closing the directory" );
            return_code = EXIT_FAILURE;
        }
    }
        
    return return_code;
}
