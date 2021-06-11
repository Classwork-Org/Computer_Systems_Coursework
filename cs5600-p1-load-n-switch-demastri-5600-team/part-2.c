/*
 * file:        part-2.c
 * description: Part 2, CS5600 load-and-switch assignment, Fall 2020
 */

/* NO OTHER INCLUDE FILES */
#include "elf64.h"
#include "sysdefs.h"

/* round A up to the next multiple of B */
#define ROUND_UP(a,b) (((a+b-1)/b)*b)

#undef STDIO_TEST        /* loopback test - checks kbd io */
#undef FILEIO_TEST       /* basic file io test - reads from 'test.txt', echoes to console */
#define EXEC_COMMANDS    /* actual process mapping and execution - normal mode */
#undef VERBOSE_EXEC_TEST /* prints helpful messages when opening / mapping program sections */

/* ------------- 
    Helpful definitions
   ------------- */
#define STDIN 0
#define STDOUT 1

#define TRUE 1
#define FALSE 0

#define TERMINATE_STRING "quit"

extern void *vector[];

/* 
    low level system call wrapper functions to be defined here 
*/
int read(int fd, void *ptr, int len);   
int write(int fd, void *ptr, int len);  
void exit(int err);                     
int open(char *path, int flags);        
int close(int fd);                      
int lseek(int fd, int offset, int flag);
void *mmap(void *addr, int len, int prot, int flags, int fd, int offset);
int munmap(void *addr, int len);

/* 
    'read' syscall wrapper.

    Improved over part 1 - if it reads a '\0' (or anything) )t will put it into the buffer.
    Flow control is based on syscall status or reaching the buffer length.
    
    int   @fd   pass file descriptor to syscall
    void* @ptr  buffer of characters to be read into and returned
    int   @len  max characters to read
    int return the number of characters read
*/
int read(int fd, void *ptr, int len)  {
    int i = 0;
    char c='\0';

    while( i < len ) {
        int status = syscall( __NR_read, fd, &c, 1);
        ((char *)ptr)[i++] = c;
        if( status < 0 )
            break;
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
int write(int fd, void *ptr, int len) {
    int i = 0;
    for ( ; i < len && *(char*)ptr; i++) {
        syscall( __NR_write, fd, ptr++, 1); // write the character
    }
    return i;
}

/* 
    'exit' syscall wrapper.
    @err int status value to pass to syscall
*/
void exit(int err)  {
    syscall(__NR_exit, err);
}

/* 
    'open' syscall wrapper.
    char* @path   location of the file to be opened
    int   @flags  permissions for the file on open
    int return the fd of the opened file
*/
int open(char *path, int flags) {
    return syscall( __NR_open, path, flags );
}

/* 
    'close' syscall wrapper.
    int @fd    the file descriptor of the file to be closed
    int return the syscall status
*/
int close(int fd) {
    return syscall( __NR_close, fd );
}

/* 
    'lseek' syscall wrapper.
    int @fd     the fd being read
    int @offset the location within the file being sought
    int @flag   parameter to the syscall
    int return  the syscall status
*/
int lseek(int fd, int offset, int flag) {
    return syscall( __NR_lseek, fd, offset, flag );
}

/* 
    'mmap' syscall wrapper.

    int @addr   process logical address for the block of memory read (including offset)
    int @len    block size
    int @prot   parameter to the syscall
    int @flag   parameter to the syscall
    int @fd     the fd sourcing this block
    int @offset the logical location in fd that sourced this block
    int return  the pointer to the actual physical location for this blocl
*/
void *mmap(void *addr, int len, int prot, int flags, int fd, int offset) {
    return (void *)syscall( __NR_mmap, addr, len, prot, flags, fd, offset );
}

/* 
    'munmap' syscall wrapper.

    int @addr   address of previously mapped block
    int @len    block size to be unmapped
    int return  the syscall status
*/
int munmap(void *addr, int len) {
    return syscall( __NR_munmap, addr, len );
}


/* ---------- */

/* the three higher level 'system call' functions to be implemented here
 *    readline, print, getarg 
 * hints: 
 *  - read() or write() one byte at a time. It's OK to be slow.
 *  - stdin is file desc. 0, stdout is file descriptor 1
 *  - use global variables for getarg
 */
void do_readline(char *buf, int len);
void do_print(char *buf);            
char *do_getarg(int i);              

int split(char **argv, int max_argc, char *line); /* given */

/*
    do_readline - read characters until a '\n' is seen into the provided buffer.
*/
void do_readline(char *buf, int len) {
    int status;
    do {
        do {
            status = read(STDIN, buf, 1);
        } while( status < 0 );
    } while( len-- > 1 && *(buf++) != '\n' );
    *buf = '\0';
}

/*
    do_print - put null-terminated string characters to STDOUT
*/
void do_print(char *buf) {
    while( *buf ) {
        write(STDOUT, buf++, 1);
    }
}

/*
    do_print_nbr - recursively print an integer to the screen
*/
void do_print_nbr(int nbr) {
    if( nbr < 0 ) {
        do_print( "-" );
        nbr = -nbr;
    }

    if( nbr > 10 )
        do_print_nbr( nbr/10 );
    
    char s[2];
    s[1] = '\0';
    s[0] = nbr % 10 + '0';
    do_print( s );
}

/**
    set of helper functions to manage argument passing to child processes
    after a command is read, it is split based on whitespace
    the count is placed into argc
    the 'words' are placed into argv (up to 10)
    
    arg[0] is the command to be executed
**/

int argc = 0;
char *argv[10];
char cmd_buffer[200];

char *do_getarg(int i) { 
    return i < argc ? argv[i] : 0;
}

int do_splitargs( char *buffer ) {
    return argc = split(argv, 10, buffer);
}

/* the guts of part 2
 *   read the ELF header
 *   for each section, if b_type == PT_LOAD:
 *     create mmap region
 *     read from file into region
 *   function call to hdr.e_entry
 *   munmap each mmap'ed region so we don't crash the 2nd time
 */

void exec_file( char *fn ) {
    unsigned long load_offset = 0x80000000;
    int nbr_exectutable_sections = 0;
    void *mmapped_locations[100];
    int mmapped_sizes[100];

    /* open the file */
    int fd=open(fn, O_RDONLY);
    if( fd < 0) {
        do_print( "Error opening file <" );
        do_print( fn );
        do_print( ">\n" );
        return;
    }
#ifdef VERBOSE_EXEC_TEST
    do_print( "Opened file: <");
    do_print( fn );
    do_print( ", at fd " );
    do_print_nbr( fd );
    do_print( ">\n" );
#endif

    /* read the full header */
    struct elf64_ehdr hdr;
    read(fd, &hdr, sizeof(hdr));

    /* get the entry point */
    void *entry_point = hdr.e_entry;

    int i, n = hdr.e_phnum;
    struct elf64_phdr phdrs[n];
    lseek(fd, hdr.e_phoff, SEEK_SET);
    read(fd, phdrs, sizeof(phdrs));

#ifdef VERBOSE_EXEC_TEST
    do_print_nbr( hdr.e_phnum );
    do_print( " program headers\n");
#endif

    /* iterate over the program headers */
    for (i = 0; i < hdr.e_phnum; i++) {
        if (phdrs[i].p_type == PT_LOAD) {
            int len = ROUND_UP(phdrs[i].p_memsz, 4096); /* nbr of bytes to allocate */
            void *virtual_address = phdrs[i].p_vaddr;   /* allocate target address...*/
            int section_size = (int)phdrs[i].p_filesz;  /* bytes to read */
            int read_offset =  (int)phdrs[i].p_offset;  /* offset to read from */

            void *target = mmapped_locations[ nbr_exectutable_sections ] = virtual_address + load_offset;
            mmapped_sizes[ nbr_exectutable_sections++ ] = len;

            void *buf = mmap( target, len, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0 );
            if( buf == MAP_FAILED ) {
                do_print( "mmap failed\n" );
                exit(1);
            }
            /* copy from file to this memory area */
            lseek( fd, read_offset, SEEK_SET );
            read( fd, target, section_size );
        }
    }
    close(fd);
 
#ifdef VERBOSE_EXEC_TEST
    do_print( "Closed file after loading / mapping ");
    do_print_nbr( nbr_exectutable_sections );
    do_print( " executable sections\n");

    do_print( "Jumping to entry point\n");
#endif

    /* create and call the loaded program's entry point */
    void (*f)();
    f = entry_point + load_offset;
    f();
    
    /* unmap executable sections on return */
#ifdef VERBOSE_EXEC_TEST
    do_print( "Unmapping loaded sections\n");
#endif
    for( i=0; i<nbr_exectutable_sections; i++) {
        munmap( mmapped_locations[i], mmapped_sizes[i]);
    }
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

/* 
    command function - reads a line from the console and splits it into arguments, returning the number read
    if quitting, returns 0
*/
int get_next_command() {
    do {
        do_print( "> " );
        do_readline( cmd_buffer, 200 );
        argc=do_splitargs( cmd_buffer );
    } while( argc == 0 );
    if( str_starts_with( do_getarg(0), TERMINATE_STRING) )
        return 0;

    return argc;
}

/* ---------- */

/* simple function to split a line:
 *   char buffer[200];
 *   <read line into 'buffer'>
 *   char *argv[10];
 *   int argc = split(argv, 10, buffer);
 *   ... pointers to words are in argv[0], ... argv[argc-1]
 */
int split(char **argv, int max_argc, char *line)
{
    int i = 0;
    char *p = line;

    while (i < max_argc) {
        while (*p != 0 && (*p == ' ' || *p == '\t' || *p == '\n'))
            *p++ = 0;
        if (*p == 0)
            return i;
        argv[i++] = p;
        while (*p != 0 && *p != ' ' && *p != '\t' && *p != '\n')
            p++;
    }
    return i;
}

/* Helper / test functions */

void echotest() {
    do_print( "Running echotest, echoes kbd input line-by-line.  Start a line with 'quit' to end:\n" );

    int argc=0;
    do {
        if( (argc=get_next_command()) == 0 )
            break;

        do_print( "Received: " );
        for( int i=0; i<argc; i++ ) {
            do_print( do_getarg(i) );
            do_print( "\n" );
        }
    } while( 1 );
    do_print( "Ending echotest\n\n" );
}

void readtest() {
    do_print( "Running readtest, opens and echoes contents of 'text.txt' in same dir to console:\n" );

    char buffer[200];
    do_print( "Opening file 'text.txt'\n" );
    int fd=open("text.txt", O_RDONLY);
    if( fd < 0) {
        do_print( "Error opening file\n" );
        return;
    }
    do_print( "File opened successfully, contents:\n" );
    int offset = 0;
    buffer[1] = '\0';
    while( lseek( fd, offset++, SEEK_SET ) >= 0 ) {
        read( fd, buffer, 1);
        if( buffer[0] == '\0')
            break;
        do_print( buffer );
    }
    do_print( "\n" );
    do_print( "Closing file\n" );
    close( fd );
    do_print( "Ending readtest\n\n" );
}

void exectest(char *exe) {
    do_print( "Testing file execution, running 'lirq' - should fail\n" );
    exec_file( "lirq" );
    do_print( "Testing file execution, running '" );
    do_print( exe );
    do_print( "'\n" );
    exec_file( exe );
    do_print( "Back from file execution\n\n" );
}

/*
    main command loop - depending on flags set, executes the requested method.
*/
int command_loop() {
#ifdef STDIO_TEST
    echotest();
#endif

#ifdef FILEIO_TEST
    readtest();
#endif

#ifdef EXEC_COMMANDS
    do_print( "Main Command Loop.  Enter file to run, and actions(s) to take.  Start a line with 'quit' to end:\n" );

    while( get_next_command() )
        exec_file( do_getarg(0) );
    do_print( "Quitting\n" );
#endif

    return 0;
}

/* 
    Main entry point.
    Calls command_loop(), and exits with whatever status it returns.   
*/
void main(void) {
    vector[0] = do_readline;
    vector[1] = do_print;
    vector[2] = do_getarg;


    exit( command_loop() );
}


