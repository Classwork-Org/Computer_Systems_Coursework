/*
 * file:        part-1.c
 * description: Part 1, CS5600 load-and-switch assignment, Fall 2020
 */

/* THE ONLY INCLUDE FILE */
#include "sysdefs.h"

/* ------------- 
    Helpful definitions
   ------------- */
#define BUF_MAX 200

#define STDIN 0
#define STDOUT 1

#define TRUE 1
#define FALSE 0

#define TERMINATE_STRING "quit"

/* ------------- 
    Use this define to replace the 'read' syscall by a series of characters, returned one at a time from readtest()
    Helpful to decouple read / write, and debug read/write issues.
   ------------- */
#undef USE_READTEST

#ifdef USE_READTEST
char readtest() {
    static int totalChars = 0;
    /* verify string length             1         2          3         4          5          6          7         8 */
    /* verify string length   1234567 8901234567890123456 789012345678901 23456789012 34567890123456789012345678 901234567890 */
    const char *testString = "abcdefg\nsome other strings\ndon't quit yet\nquitString\ndone - shouldn't get here\n";
    return testString[totalChars++ % 80 ];  /* walk through the string, one character at a time including term */
}
#endif

/* 
    'read' syscall wrapper.

    This method assumes (not a good thing for general use) that you are reading strings.
    As a result, it will terminate before 'len' characters if it sees a '\n' as input.
    It will turn the '\n' into a '\0', allowing the client to use intput as readline() if needed.

    int   @fd   pass file descriptor to syscall
    void* @ptr  buffer of characters to be read into and returned
    int   @len  max characters to read
    int return the number of characters read
*/
int read(int fd, void* ptr, int len) {
    int i = 0;
    char c = '\0'; /* seed to enter loop */

    while( i < len && c != '\n' ) {
#ifdef USE_READTEST
        c = readtest();
#else
        syscall( __NR_read, fd, &c, 1); /* do i/o one char at a time */
#endif
        ((char *)ptr)[i++] = (c == '\n' ? '\0' : c); /* store the character */
    }
    return i;
}

/* 
    'write' syscall wrapper.

    This method assumes (not a good thing for general use) that you are writing strings.
    As a result, it will terminate before 'len' characters if it sees a '\0' in the buffer.


    int   @fd   pass file descriptor to syscall
    void* @ptr  buffer of characters to be written
    int   @len  max characters to write
    int return the number of characters written
*/
int write(int fd, void* ptr, int len) {
    
    for ( int i = 0; i < len; i++) {
        if (*(char*)(ptr+i) != '\0') {
            syscall( __NR_write, fd, ptr+i, 1); // write the character
        } else {
            return i;
        }
    }
    return len;
}

/* 
    'exit' syscall wrapper.
    @err int status value to pass to syscall
*/
void exit(int err) {
    syscall(__NR_exit, err);
}

/* 
    Helper function to determine if one string starts with the contents of another.
*/
int str_starts_with( const char *testStr, const char *refStr ) {
    while( *refStr ) {
        if( *refStr != *testStr || *testStr == '\0') {
            return FALSE;
        }
        refStr++, testStr++;
    }
    return TRUE;
} 

char loop_bfr[BUF_MAX] = ""; /* define here so it is compiler allocated and not put on stack */
/* 
    Helper function to implement the loopback test.
    Prints instructions, and prompt, then reads back the line and prints back.
    Terminates if the read line starts with whatever's in TERMINATE_STRING
*/
int loopback_test() {
    write(STDOUT, "this program echos input a line at a time, and quits if the line starts with 'quit'\n", 100);

    do {
        write(STDOUT, "> ", 3);
        if( read(STDIN, loop_bfr, BUF_MAX) < 0 )
            return -1;

        write(STDOUT, "Got: ", 6);
        write(STDOUT, loop_bfr, BUF_MAX);
        write(STDOUT, "\n", 2);
    } while( !str_starts_with( loop_bfr, TERMINATE_STRING) );
    return 0;
}

/* 
    Main entry point.
    Calls loopback test, and exits with whatever status it returns.   
*/
void main(void) {
    int status = loopback_test();

    exit(status);
}
