/*! \file    str.hpp
    \brief   Interface to a Rexx-like string class.
    \author  Peter C. Chapin <PChapin@vtc.vsc.edu>

This file implements a simple string class. It supports a set of operations that allow clients
to use string objects in a manner similar to the way Rexx works.

The 'rep' member should never be NULL. This is because if an exception is throw during
construction (that's the only time rep might become NULL), the object under construction can
never be accessed in a well defined manner.

This version is well behaved in a multi-threaded environment provided the symbol pMULTITHREADED
is defined before compilation.

TO DO

+ The inserter and extractor operators should honor stream formatting state.
*/

#include "environ.hpp"
#include <cstring>
#include <iostream>
#include <memory>
#include "str.hpp"

#if defined(pMULTITHREADED)
#include "sem.hpp"
#endif

/*! \class spica::String
 *
 * Class String has features that are similar to those offered by the strings built into the
 * Rexx language. However, not all of the features of Rexx strings are currently implemented.
 * Furthermore, the names of these methods and their semantics are not always exactly the same
 * as the corresponding Rexx operation. Nevertheless the general behavior of these strings
 * should be very familiar to a Rexx programmer. I may or may not make these strings more
 * "Rexx-like" in the future. It will depend on how they get used.
 *
 * These strings can not hold a null character as part of the string's data. However, these
 * strings are fully dynamic and can expand (and contract) as necessary. These strings are
 * exception safe in the sense that if an exception occurs during a string operation there is no
 * resource leakage and the string being operated on is left unchanged. The only exception these
 * methods might throw would be std::bad_alloc due to an out of memory condition.
 *
 * Some of the operations in this class cause the string to be changed while many of the others
 * return a new string with the changed value. In the future I may provide both "mutating" and
 * "non-mutating" forms of most operations. For now, the decision to use one form or the other
 * was made based in part on what Rexx does and in part on what seemed most reasonable. I
 * considered making these strings fully immutable but that does not seem correct for strings
 * such as these that have value semantics.
 *
 * Notice that the methods that use an offset into a string use offsets based on one, not zero.
 * This follows Rexx conventions. Furthermore the methods all try to do sensible things when
 * given strange argument values. These strings are quite forgiving in this respect. In
 * particular, negative counts are normally treated as if they were zero and zero counts do the
 * expected thing. Counts that go beyond the end of the string are generally taken to mean "all
 * the string that is available." Offsets that are before the start of the string (&lt; 1) or
 * off the end of the string generally cause "no operation" to occur.
 *
 * This string class currently uses reference counting to improve the speed of copying strings.
 * With this implementation, passing strings to functions by value, returning them by value, or
 * copying them are all low overhead, O(1) operations. A string's representation is only copied
 * when necessary (on demand).
 *
 * These strings are thread safe in the sense that multiple threads can manipulate the same
 * string without causing undefined behavior. If a thread reads a string's value while that
 * value is being updated by a mutating operation, it is unspecified if the first thread reads
 * the string's old value or new value. However, it will read one of those two values reliably.
 * It is important that no global strings are created in a multithreaded program. This is
 * because this class uses a global lock and C++ does not define the order in which global
 * static data is initialized across translation units. If you need a global string, create a
 * global pointer to a string and then give that pointer a value using operator new after main()
 * has started.
 *
 * To compile in multithreaded support the symbol pMULTITHREADED must be defined when this file
 * (and the header file) are compiled. The module sem.cpp from the spica library must also be
 * provided. This class assumes that spica::mutex_sem supports recursive locking. If that is not
 * the case, the non-mutating operations that return a new string will deadlock.
 */

namespace spica {
    
    #if defined(pMULTITHREADED)
    // This is the "BSL" (Big String Lock).
    static mutex_sem string_lock;
    #endif

    //-------------------------------------------------
    //           Internally Linked Functions
    //-------------------------------------------------

    static bool is_white( int ch, const char *white )
    {
        // If the user is trying to use a special kind of whitespace...
        if( white != 0 ) {
            if( std::strchr( white, ch ) != 0 ) return true;
            return false;
        }

        // Otherwise use the default.
        if( ch == ' '  || ch == '\t' || ch == '\v' ||
            ch == '\r' || ch == '\n' || ch == '\f' )
            return true;
        
        return false;
    }


    //--------------------------------------
    //           Friend Functions
    //--------------------------------------
    
    /*!
     * This funtion returns true if the strings have the same contents. The comparison is done in
     * a case sensitive manner.
     */
    bool operator==( const String &left, const String &right )
    {
        #if defined(pMULTITHREADED)
        mutex_sem::grabber lock( string_lock );
        #endif

        // Is this first comparison worthwhile?
        if( left.rep == right.rep ) return true;
        return ( std::strcmp( left.rep->workspace, right.rep->workspace ) == 0 );
    }


    /*!
     * This function returns true if the first string comes before the second. [Elaborate on
     * what 'comes before' means for strings]
     */
    bool operator<( const String &left, const String &right )
    {
        #if defined(pMULTITHREADED)
        mutex_sem::grabber lock( string_lock );
        #endif

        return ( std::strcmp( left.rep->workspace, right.rep->workspace ) < 0 );
    }


    /*!
     * This function writes the characters of the given string into the given output stream. A
     * newline character is <em>not</em> added to the output automatically.
     */
    std::ostream &operator<<( std::ostream &os, const String &right )
    {
        #if defined(pMULTITHREADED)
        mutex_sem::grabber lock( string_lock );
        #endif

        os << right.rep->workspace;
        return os;
    }


    /*!
     * This function reads characaters from the given input stream into the given string.
     * Characters are read until a newline or EOF is reached. The string is expanded as
     * necessary. Note that this funtion does not add the newline to the string although it does
     * remove the newline from the input.
     */
    std::istream &operator>>( std::istream &is, String &right )
    {
        char   ch;
        String temp;

        // This is sort of inefficient. Do I care? Not right now.
        while( is.get( ch ) ) {
            if( ch == '\n' ) break;
            temp.append( ch );
        }
        
        right = temp;
        return is;
    }

    //----------------------------
    //           Methods
    //----------------------------

    String::String( )
    {
        std::auto_ptr< string_node > new_node( new string_node );
        new_node->workspace = new char[1];
        *new_node->workspace = '\0';
        rep = new_node.get( );
        new_node.release( );
    }


    String::String( const String &existing )
    {
        #if defined(pMULTITHREADED)
        mutex_sem::grabber lock(string_lock);
        #endif
  
        rep = existing.rep;
        rep->count++;
    }


    String::String( const char *existing )
    {
        std::auto_ptr< string_node > new_node( new string_node );
        new_node->workspace = new char[std::strlen( existing ) + 1];
        std::strcpy( new_node->workspace, existing );
        rep = new_node.get( );
        new_node.release( );
    }


    String::String( char existing )
    {
        std::auto_ptr< string_node > new_node( new string_node );
        new_node->workspace = new char[2];
        new_node->workspace[0] = existing;
        new_node->workspace[1] = '\0';
        rep = new_node.get( );
        new_node.release( );
    }


    /*!
     * This method releases the memory owned by the string provided that this string's
     * representation is not being shared.
     */
    String::~String( )
    {
        #if defined(pMULTITHREADED)
        mutex_sem::grabber lock( string_lock );
        #endif
  
        if( rep->count > 1 ) rep->count--;
        else {
            delete [] rep->workspace;
            delete    rep;
        }
    }


    String &String::operator=( const String &other )
    {
        // Check for assignment to self.
        if( &other == this ) return *this;

        #if defined(pMULTITHREADED)
        mutex_sem::grabber lock( string_lock );
        #endif

        if( rep->count > 1 ) rep->count--;
        else {
            delete [] rep->workspace;
            delete    rep;
        }

        rep = other.rep;
        rep->count++;

        return *this;
    }


    String &String::operator=( const char *other )
    {
        if( other == 0 ) return *this;

        std::auto_ptr< string_node > new_node( new string_node );
        new_node->workspace = new char[std::strlen( other ) + 1];
        std::strcpy( new_node->workspace, other );
  
        #if defined(pMULTITHREADED)
        mutex_sem::grabber lock( string_lock );
        #endif

        if( rep->count > 1 ) rep->count--;
        else {
            delete [] rep->workspace;
            delete    rep;
        }
        rep = new_node.get( );
        new_node.release( );

        return *this;
    }


    /*!
     * The length does not include the terminating null character. Note that currently this is
     * an O(n) operation.
     */
    int String::length( ) const
    {
        #if defined(pMULTITHREADED)
        mutex_sem::grabber lock( string_lock );
        #endif

        return std::strlen( rep->workspace );
    }


    String &String::append( const String &other )
    {
        #if defined(pMULTITHREADED)
        mutex_sem::grabber lock( string_lock );
        #endif

        int count = std::strlen( rep->workspace ) + std::strlen( other.rep->workspace );
        std::auto_ptr< string_node > new_node( new string_node );
        new_node->workspace = new char[count + 1];

        std::strcpy( new_node->workspace, rep->workspace );
        std::strcat( new_node->workspace, other.rep->workspace );

        if( rep->count > 1 ) rep->count--;
        else {
            delete [] rep->workspace;
            delete    rep;
        }
        rep = new_node.get( );
        new_node.release( );
        return *this;
    }


    String &String::append( const char *other )
    {
        #if defined(pMULTITHREADED)
        mutex_sem::grabber lock( string_lock );
        #endif

        int count = std::strlen( rep->workspace ) + std::strlen( other );
        std::auto_ptr< string_node > new_node( new string_node );
        new_node->workspace = new char[count + 1];

        std::strcpy( new_node->workspace, rep->workspace );
        std::strcat( new_node->workspace, other );

        if( rep->count > 1 ) rep->count--;
        else {
            delete [] rep->workspace;
            delete    rep;
        }
        rep = new_node.get( );
        new_node.release( );
        return *this;
    }


    String &String::append( char other )
    {
        #if defined(pMULTITHREADED)
        mutex_sem::grabber lock( string_lock );
        #endif

        int count = std::strlen( rep->workspace ) + 1;
        std::auto_ptr< string_node > new_node( new string_node );
        new_node->workspace = new char[count + 1];

        std::strcpy( new_node->workspace, rep->workspace );
        new_node->workspace[count - 1] = other;
        new_node->workspace[count    ] = '\0';
        
        if( rep->count > 1 ) rep->count--;
        else {
            delete [] rep->workspace;
            delete    rep;
        }
        rep = new_node.get( );
        new_node.release( );
        return *this;
    }


    /*!
     * After this method returns, this string is empty. This is a mutating operation. In that
     * respect it differs from erase(int, int)
     */
    void String::erase( )
    {
        std::auto_ptr< string_node > new_node( new string_node );
        new_node->workspace = new char[1];
        *new_node->workspace = '\0';

        #if defined(pMULTITHREADED)
        mutex_sem::grabber lock( string_lock );
        #endif

        if( rep->count > 1 ) rep->count--;
        else {
            delete [] rep->workspace;
            delete rep;
        }
        rep = new_node.get( );
        new_node.release( );
    }


    /*!
     * If the requested number of characters is larger than the length of the string this method
     * pads the result (on the left) with multiple copies of the pad character. Consequently
     * this method always returns a string with the requested length.
     *
     * \param length The number of characters to return.
     * \param pad The pad character to use if length is too large.
     * \return A new string containing the result. The original string is unchanged.
     */
    String String::right( int length, char pad ) const
    {
        #if defined(pMULTITHREADED)
        mutex_sem::grabber lock( string_lock );
        #endif

        // A place to put the answer.
        String result;

        // Ignore attempts to use a negative count.
        if (length <= 0) return result;

        int current_length = std::strlen( rep->workspace );

        // If we need to make the string shorter...
        if( length < current_length ) {
            char *temp = new char[length + 1];
            std::strcpy( temp, &rep->workspace[current_length - length] );
            delete [] result.rep->workspace;
            result.rep->workspace = temp;
        }
        
        // otherwise we need to make the string longer or the same size...
        else {
            char *temp = new char[length + 1];
            std::memset( temp, pad, length - current_length );
            std::strcpy( &temp[length - current_length], rep->workspace );
            delete [] result.rep->workspace;
            result.rep->workspace = temp;
        }

        return result;
    }


    /*!
     * If the requested number of characters is larger than the length of this string this
     * method pads the result (on the right) with multiple copies of the pad character.
     * Consequently this method always returns a string with the requested length.
     *
     * \param length The number of characters to return.
     * \param pad The pad character to use if length is too large.
     * \return A new string containing the result. The original string is unchanged.
     */
    String String::left( int length, char pad ) const
    {
        #if defined(pMULTITHREADED)
        mutex_sem::grabber lock( string_lock );
        #endif

        // A place to put the answer.
        String result;

        // Ignore attempts to use a negative count.
        if( length <= 0 ) return result;

        int current_length = std::strlen(rep->workspace);

        // If we need to make the string shorter...
        if( length < current_length ) {
            char *temp = new char[length + 1];
            std::strncpy( temp, rep->workspace, length );
            temp[length] = '\0';
            delete [] result.rep->workspace;
            result.rep->workspace = temp;
        }

        // otherwise we need to make the string longer...
        else {
            char *temp = new char[length + 1];
            std::strcpy( temp, rep->workspace );
            std::memset( &temp[current_length], pad, length - current_length );
            temp[length] = '\0';
            delete [] result.rep->workspace;
            result.rep->workspace = temp;
        }

        return result;
    }


    /*!
     * If the requested length is larger than the length of this string (and this would be the
     * usual case), this string is centered between a suitable number of pad characters. If the
     * requested length is less than the length of this string, this string is truncated so that
     * the returned string contains only the first length characters of this string.
     *
     * \param length The length of the returned string.
     * \param pad The pad character to use on either side of the centered string.
     * \return A new string containing the result. The original string is not changed.
     */
    String String::center( int length, char pad ) const
    {
        #if defined(pMULTITHREADED)
        mutex_sem::grabber lock( string_lock );
        #endif

        // A place to put the answer.
        String result;

        // Ignore attempts to use a negative length.
        if( length <= 0 ) return result;

        int current_length = std::strlen( rep->workspace );

        // If the current string is too large or the same size, it's just a left() operation.
        //
        if( length <= current_length ) {
            return left( length, pad );
        }

        // Otherwise I have to do real work.
        else {
            int left_side  = ( length - current_length ) / 2;
            int right_side = length - current_length - left_side;

            char *temp = new char[length + 1];
            std::memset( temp, pad, left_side );
            std::strcpy( &temp[left_side], rep->workspace );
            std::memset( &temp[left_side + current_length], pad, right_side );
            temp[length] = '\0';
            delete [] result.rep->workspace;
            result.rep->workspace = temp;
        }

        return result;
    }


    /*!
     * For example if this string was 'xyz' and count was 3, this method would return
     * 'xyzxyzxyz'.
     *
     * \param count The number of times this string should be copied.
     *
     * \return A new string containing count copies of this string concatenated onto one
     * another. The original string is unchanged.
     */
    String String::copy( int count ) const
    {
        #if defined(pMULTITHREADED)
        mutex_sem::grabber lock( string_lock );
        #endif

        // A place to put the answer.
        String result;

        // Ignore attempts to use a negative count.
        if( count < 0 ) return result;

        char *temp = new char[count * std::strlen( rep->workspace ) + 1];

        temp[0] = '\0';
        for( int i = 0; i < count; i++ ) std::strcat( temp, rep->workspace );
        delete [] result.rep->workspace;
        result.rep->workspace = temp;

        return result;
    }


    /*!
     * \param offset The index to the starting character of the substring to be erased.
     * \param count The number of characters to be erased.
     * \return A new string containing the result after erasure. The original string is
     * unchanged.
     */
    String String::erase( int offset, int count ) const
    {
        #if defined(pMULTITHREADED)
        mutex_sem::grabber lock( string_lock );
        #endif

        // A place to put the answer.
        String result;

        // The client uses one based offsets. We'll used zero based offsets.
        offset--;

        // Ignore negative parameters.
        if( offset < 0 || count < 0 )
            { result = *this; return result; }

        int current_length = std::strlen( rep->workspace );

        // Verify that there is actual work to do.
        if( offset >= current_length || count == 0 )
            { result = *this; return result; }

        // Trim the count so that it fits into the bounds on the string. This has to be done
        // carefully considering that count might be INT_MAX in many cases.
        //
        int max_count = current_length - offset;
        if( count > max_count ) count = max_count;

        // Now do the work.
        char *temp = new char[current_length - count + 1];
        std::memcpy( temp, rep->workspace, offset );
        std::strcpy( &temp[offset], &rep->workspace[offset + count] );
        delete [] result.rep->workspace;
        result.rep->workspace = temp;

        return result;
    }


    /*!
     * If the index is just past the end of this string, the inserted material is appended to
     * this string. If the index is farther past the end than that, the result string is the
     * same as this string (the insertion request is ignored).
     *
     * \param incoming The string to insert.
     *
     * \param offset The index into this string before which the new material will be inserted.
     * Thus an index of one implies that the new material is prepended to the string.
     *
     * \param count The number of characters to insert. If this parameter is greater than the
     * length of the incoming string, all the incoming characters are used.
     *
     * \return A new string containing the result after insertion. The original string is
     * unchanged.
     */
    String String::insert( const String &incoming, int offset, int count ) const
    {
        #if defined(pMULTITHREADED)
        mutex_sem::grabber lock( string_lock );
        #endif

        // A place to put the answer.
        String result;

        offset--;

        if( offset < 0 || count < 0 )
            { result = *this; return result; }

        int current_length = std::strlen( rep->workspace );

        // Verify that there is actual work to do.
        if( offset > current_length || count == 0 )
            { result = *this; return result; }
        
        // Trim the count.
        int incoming_length = std::strlen( incoming.rep->workspace );
        if( count > incoming_length ) count = incoming_length;

        // Now do the work.
        char *temp = new char [current_length + count + 1];
        std::memcpy( temp, rep->workspace, offset );
        // Does memcpy() freak if you copy 0 bytes?
        std::memcpy( &temp[offset], incoming.rep->workspace, count );
        std::strcpy( &temp[offset + count], &rep->workspace[offset] );
        delete [] result.rep->workspace;
        result.rep->workspace = temp;

        return result;
    }


    /*!
     * You can <em>not</em> locate the null character at the end of the string using this
     * method.
     *
     * \param needle The character to find.
     * \param offset The starting index for the search.
     * \return The index of the first occurance of the needle or 0 if it is not found.
     */
    int String::pos( char needle, int offset ) const
    {
        #if defined(pMULTITHREADED)
        mutex_sem::grabber lock( string_lock );
        #endif

        offset--;

        // If we are starting off the end of the string, then obviously we didn't find anything.
        // Note that this function *does* allow the caller to locate the null character at the
        // end of the string.
        //
        if( offset < 0 || static_cast< std::size_t >( offset ) > std::strlen( rep->workspace ) )
            return 0;

        // Locate the character.
        const char *p = rep->workspace + offset;
        p = std::strchr( p, needle );

        // If we didn't find it, return error.
        if( p == 0 ) return 0;

        // Otherwise return the offset to the character.
        return static_cast< int >( p - rep->workspace ) + 1;
    }


    /*!
     * \param needle Pointer to the string to find.
     * \param offset The starting index for the search.
     * \return The index to the beginning of the needle string's first occurance or 0 if the
     * needle string is not found.
     */
    int String::pos( const char *needle, int offset ) const
    {
        #if defined(pMULTITHREADED)
        mutex_sem::grabber lock( string_lock );
        #endif

        offset--;

        // If we are starting off the end of the string, then obviously we didn't find anything.
        if( offset < 0 || static_cast< std::size_t >( offset ) > std::strlen( rep->workspace ) )
            return 0;

        // Locate the substring.
        const char *p = rep->workspace + offset;
        p = std::strstr( p, needle );

        // If we didn't find it, return error.
        if( p == 0 ) return 0;

        // Otherwise return the offset to the first character in the substring.
        return static_cast< int >( p - rep->workspace ) + 1;
    }


    /*!
     * \param needle The character to find.
     *
     * \param offset The starting index for the search. Any index that is off the end of the
     * string implies that the search starts at the string's end.
     *
     * \return The index of the last occurance of the needle character (last relative to the
     * starting index) or 0 if the character was not found.
     */
    int String::last_pos( char needle, int offset ) const
    {
        #if defined(pMULTITHREADED)
        mutex_sem::grabber lock( string_lock );
        #endif

        offset--;

        int current_length = std::strlen( rep->workspace );

        // Handle the case of offset being off the end of the string.
        if( offset < 0 ) return 0;
        if( offset > current_length ) offset = current_length;

        const char *p = rep->workspace + offset;
    
        // Now back up. If we find the character, return the offset to it.
        while( p >= rep->workspace ) {
            if( *p == needle ) return static_cast< int >( p - rep->workspace ) + 1;
            p--;
            // Is it technically ok to step a pointer one off the beginning of an array? (NO!)
        }
        
        // If we got here, then we didn't find the character.
        return 0;
    }


    /*!
     * \param mode Use 'L' to strip leading characters, 'T' to strip trailing characters, or 'B'
     * to strip both leading and trailing characters.
     *
     * \param kill_char The character to strip. All leading (or trailing or both) copies of such
     * character are removed.
     *
     * \return A new string containing the result. The original string is unchanged.
     */
    String String::strip( char mode, char kill_char ) const
    {
        #if defined(pMULTITHREADED)
        mutex_sem::grabber lock( string_lock );
        #endif

        // A place to put the answer.
        String result;

        const char *start = rep->workspace;
        const char *end   = std::strchr( rep->workspace, '\0' );

        // Handle the empty string as a special case.
        if( start == end ) return result;

        // Back up end so that it points just before the terminating null character.
        //
        end--;

        // Move start to the desired spot.
        if( mode == 'L' || mode == 'B' ) {
            while( *start ) {
                if( *start != kill_char ) break;
                start++;
            }
        }

        // Move end to the desired spot. Note that there is a portability problem here. If the
        // string is entirely kill_char and just 'T' mode is requested, end will be backed up
        // all the way before rep->workspace. This means that end will point off the *front* of
        // an array and that is a bad thing. This should be fixed someday.
        //
        if( mode == 'T' || mode == 'B' ) {
            while( end >= rep->workspace ) {
                if( *end != kill_char ) break;
                end--;
            }
        }

        // There is nothing to do if the two pointers passed by each other.
        if( start > end ) {
            return result;
        }

        // Otherwise there is something to do.
        else {
            int length = static_cast< int >( end - start ) + 1;
            char *temp = new char[length + 1];
            std::memcpy( temp, start, length );
            temp[length] = '\0';
            delete [] result.rep->workspace;
            result.rep->workspace = temp;
        }
        
        return result;
    }


    /*!
     * \param offset The starting index for the substring.
     * \param count The length of the substring
     * \return The specified substring.
     */
    String String::substr( int offset, int count ) const
    {
        #if defined(pMULTITHREADED)
        mutex_sem::grabber lock( string_lock );
        #endif

        // A place to put the answer.
        String result;

        offset--;

        if( offset < 0 || count < 0 ) return result;

        int current_length = std::strlen( rep->workspace );

        // If the offset is off the end of the string, then return an empty string.
        //
        if( offset >= current_length ) {
            return result;
        }

        // Adjust the count if necessary.
        if( count > current_length - offset ) count = current_length - offset;

        // Create the new string.
        char *temp = new char[count + 1];
        std::memcpy( temp, &rep->workspace[offset], count );
        temp[count] = '\0';

        delete [] result.rep->workspace;
        result.rep->workspace = temp;

        return result;
    }


    /*!
     * Embeded delimiter characters are retained exactly as they exist in the string, but
     * leading and trailing delimiter characters are removed. By default delimiters are the
     * white space characters: space, tab, vertical tab, carriage return, newline, and form
     * feed. However, if white is non-null it is taken to point at a string that defines the
     * delimiter characters. For example, if white points at "xyz" then the characters 'x', 'y',
     * and 'z' would be the delimiters. In that case, the second word of the string
     * "HixThereyYouz" would be "There".
     *
     * \param offset The starting index in this string. For this method, indicies are word
     * counts. The first word is at index 1.
     * \param count The number of words in the desired substring.
     * \param white Pointer to a string containing word delimiter characters.
     * \return The specified substring.
     */
    String String::subword( int offset, int count, const char *white ) const
    {
        #if defined(pMULTITHREADED)
        mutex_sem::grabber lock( string_lock );
        #endif

        // A place to put the answer.
        String result;

        offset--;

        if( offset < 0 || count < 0 ) return result;

        int current_length = words( white );

        // If the offset is off the end of the string, then return an empty string.
        if( offset >= current_length ) {
            return result;
        }

        // Adjust the count if necessary.
        if( count > current_length - offset ) count = current_length - offset;

        // Handle the count of zero as a special case.
        if( count == 0 ) return result;

        // Find the beginning of the the offsetth word.
        const char *start = rep->workspace;
        while( 1 ) {

            // Skip leading whitespace.
            while( is_white( *start, white ) ) start++;

            if( offset == 0 ) break;

            // Find the end of this word.
            while( !is_white( *start, white ) ) start++;
            offset--;
        }

        // Now find the end of the countth word from start.
        const char *end = start;
        while( 1 ) {

            // Find the end of this word.
            while( !( is_white( *end, white ) || *end == '\0' ) ) end++;
            count--;

            if( count == 0 ) break;

            // Find the beginning of the next word.
            while( is_white( *end, white ) ) end++;
        }

        // Now create the new character string.
        int length = static_cast< int >( end - start );
        char *temp = new char[length + 1];
        std::memcpy( temp, start, length );
        temp[length] = '\0';

        delete [] result.rep->workspace;
        result.rep->workspace = temp;
        
        return result;
    }


    /*!
     * \param white Points at a string of word delimiter characters.
     * \return The number of words in this string.
     * \sa subword
     */
    int String::words( const char *white ) const
    {
        #if defined(pMULTITHREADED)
        mutex_sem::grabber lock( string_lock );
        #endif

        int  word_count = 0;   // The number of words found.
        int  in_word    = 0;   // =1 When we are scanning a word.
        
        // Scan down the string...
        for( const char *p = rep->workspace; *p; p++ ) {

            // If this is the start of a word...
            if( !is_white( *p, white ) && !in_word ) {
                word_count++;
                in_word = 1;
            }

            // If we just finished scanning a word...
            if( is_white( *p, white ) && in_word ) {
                in_word = 0;
            }
        }

        return word_count;
    }


    /*!
     * This function concatenates right onto the end of left and returns the result. Neither
     * right nor left are modified.
     */
    String operator+( const String &left, const String &right )
        { String temp( left ); temp.append( right ); return temp; }

    /*!
     * This function concatenates right onto the end of left and returns the result. Neither
     * right nor left are modified.
     */
    String operator+( const String &left, const char *right )
        { String temp( left ); temp.append( right ); return temp; }

    /*!
     * This function concatenates right onto the end of left and returns the result. Neither
     * right nor left are modified.
     */
    String operator+( const char *left, const String &right )
        { String temp( left ); temp.append( right ); return temp; }

    /*!
     * This function concatenates right onto the end of left and returns the result. Neither
     * right nor left are modified.
     */
    String operator+( const String &left, char right )
        { String temp( left ); temp.append( right ); return temp; }

    /*!
     * This function concatenates right onto the end of left and returns the result. Neither
     * right nor left are modified.
     */
    String operator+( char left, const String &right )
        { String temp( left ); temp.append( right ); return temp; }

}
