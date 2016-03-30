/*! \file    shell.cpp
    \brief   Program to do basic file management in a simulated file system.
    \author  Peter C. Chapin <PChapin@vtc.vsc.edu>

This program supports a simple user interface complete with commands that allow a user to
manipulate files in a FileSystem object. This program is part of a demonstration at Vermont
Technical College. The total program illustrates how file systems are implemented (in a general
sort of way).
*/

#include "environ.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <cstdlib>

#include "BlockDevice.hpp"
#include "FileSystem.hpp"
#include "str.hpp"


//=======================================
//           Support Functions
//=======================================

static void error(const char *message)
{
    std::cout << "ERROR: " << message << std::endl;
}


//=======================================
//           Command Functions
//=======================================

//
// The following stuff defines functions for each supported command and sets up a jump table to
// make it easier to run them.
//
typedef bool (*operation)(const spica::String &, FileSystem &);

struct command_definition {
    spica::String command_name;
    operation   command_function;

    command_definition(const spica::String &s, operation o) :
        command_name(s), command_function(o) { }
};

bool dir_op     (const spica::String &, FileSystem &);
bool quit_op    (const spica::String &, FileSystem &);
bool format_op  (const spica::String &, FileSystem &);
bool vcopy_op   (const spica::String &, FileSystem &);
bool vcopyin_op (const spica::String &, FileSystem &);
bool vcopyout_op(const spica::String &, FileSystem &);
bool vdel_op    (const spica::String &, FileSystem &);
bool vdir_op    (const spica::String &, FileSystem &);

command_definition jump_table[] = {
    command_definition(spica::String("dir"),      dir_op     ),
    command_definition(spica::String("exit"),     quit_op    ),
    command_definition(spica::String("format"),   format_op  ),
    command_definition(spica::String("vcopy"),    vcopy_op   ),
    command_definition(spica::String("vcopyin"),  vcopyin_op ),
    command_definition(spica::String("vcopyout"), vcopyout_op),
    command_definition(spica::String("vdel"),     vdel_op    ),
    command_definition(spica::String("vdir"),     vdir_op    )
};
const int command_count = sizeof(jump_table)/sizeof(command_definition);


//
// dir_op
//
bool dir_op(const spica::String &, FileSystem &)
{
    std::system("dir");
    return false;
}


//
// quit_op
//
bool quit_op(const spica::String &, FileSystem &)
{
    return true;
}


//
// format_op
//
bool format_op(const spica::String &, FileSystem &files)
{
    files.format();
    return false;
}


//
// vcopy_op
//
bool vcopy_op(const spica::String &command_line, FileSystem &files)
{
    if (command_line.words() != 3) error("usage: vcopy source destination");
    else {
        int in, out;
        in  = files.open(static_cast<const char *>(command_line.word(2)), FileSystem::READ);
        files.truncate(command_line.word(3));
        out = files.open(static_cast<const char *>(command_line.word(3)), FileSystem::WRITE);

        // Odd size chosen on purpose. This is supposed to be a test!
        char buffer[1000];
        int  count;
        while ((count = files.read(in, buffer, 1000)) != 0) {
            if (files.write(out, buffer, count) != count)
                error("problem writing destination");
        }
        files.close(in);
        files.close(out);
    }
    return false;
}


//
// vcopyin_op
//
bool vcopyin_op(const spica::String &command_line, FileSystem &files)
{
    if (command_line.words() != 3) error("usage: vcopyin source destination");
    else {
        std::ifstream in;
        int           out;

        in.open(static_cast<const char *>(command_line.word(2)), std::ios::in|std::ios::binary);
        if (!in) error("can't open input file in host file system");
        else {
            files.truncate(command_line.word(3));
            out = files.open(static_cast<const char *>(command_line.word(3)), FileSystem::WRITE);

            // Odd size chosen on purpose. This is supposed to be a test!
            char buffer[1000];
            int  count;
            in.read(buffer, 1000);
            while ((count = in.gcount()) != 0) {
                if (files.write(out, buffer, count) != count)
                    error("problem writing destination");
                in.read(buffer, 1000);
            }
            files.close(out);
        }
    }
    return false;
}


//
// vcopyout_op
//
bool vcopyout_op(const spica::String &command_line, FileSystem &files)
{
    if (command_line.words() != 3) error("usage: vcopy source destination");
    else {
        int           in;
        std::ofstream out;

        in  = files.open(static_cast<const char *>(command_line.word(2)), FileSystem::READ);
        out.open(static_cast<const char *>(command_line.word(3)), std::ios::out|std::ios::binary);
        if (!out) {
            error("can't open output file in host file system");
            files.close(in);
        }
        else {

            // Odd size chosen on purpose. This is supposed to be a test!
            char buffer[1000];
            int  count;
            while ((count = files.read(in, buffer, 1000)) != 0) {
                out.write(buffer, count);
            }
            files.close(in);
        }
    }
    return false;
}


//
// vdel_op
//
bool vdel_op(const spica::String &command_line, FileSystem &files)
{
    if (command_line.words() != 2) error("usage: vdel filename");
    else {
        files.remove(command_line.word(2));
    }
    return false;
}


//
// vdir_op
//
bool vdir_op(const spica::String &, FileSystem &files)
{
    FileSystem::directory_info info;
    files.open_dir();
    while (files.next_dir(&info)) {
        std::cout
            << std::setw(24) << info.name
            << std::setw(10) << info.size << std::endl;
    }
    return false;
}

//==================================
//           Main Program
//==================================

//
// my_main
//
// This is the real main() function. It is called by main(). This method of organization puts
// the primary exception handler out of the way.
//
int my_main()
{
    using namespace spica;
    
    bool done = false;
    // Becomes true when the user wants to quit.

    BlockDevice disk("block.dev", 1024, 512);
    // We need a "raw" disk here. The constructor creates space in the hosting file system and
    // does, in effect, a low level format. If the backing file already exists it is used as is.

    FileSystem files(disk);
    // Associate a file system with the block device we created above.

    // Let's see what we've got.
    if (files.is_formatted()) {
        std::cout << "The file system appears to be formatted." << std::endl;
    }
    else {
        std::cout << "The file system does not appear to be formatted." << std::endl;
    }

    // Now interact with the user.
    while (!done) {
        String command_line;

        // Display the prompt.
        std::cout << std::endl;
        if (files.is_formatted())
            std::cout << files.free_space() << " bytes available" << std::endl;
        std::cout << "> " << std::flush;
        std::cin  >> command_line;

        // Process the command.
        spica::String command_word = command_line.word(1);

        int i;
        for (i = 0; i < command_count; i++) {
            if (jump_table[i].command_name == command_word) {
                done = jump_table[i].command_function(command_line, files);
                break;
            }
        }

        // If we didn't find it, print a "command unknown" message.
        if (i == command_count) error("command unknown");

        // Check the file system after every command.
        files.check();
    }

    return 0;
}


//
// main
//
// This function calls the real my_main() function. It encloses everything in a generic
// exception handler so that no exceptions can escape from the program. (Well, not really, but
// almost).
//
int main()
{
    // Let's try to execute the my_main() function. If it returns, the program is done.
    try {
        return my_main();
    }

    // If my_main() throws an exception, then print a message and die. Let's not attempt to
    // restart ourselves. Why bother?
    // 
    catch (const char *message) {
        std::cerr << "We gacked: An unhandled exception reached main()" << std::endl;
        std::cerr << "  MESSAGE: " << message << std::endl;
    }
    catch (...) {
        std::cerr
            << "We gacked: An unknown, unhandled exception reached main()" << std::endl;
    }
    return 1;
}
