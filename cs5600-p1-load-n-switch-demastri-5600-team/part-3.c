/*
 * file:        part-3.c
 * description: part 3, CS5600 load-and-switch assignment, Fall 2020
 */

/* NO OTHER INCLUDE FILES */
#include "elf64.h"
#include "sysdefs.h"

extern void *vector[];
extern void switch_to(void **location_for_old_sp, void *new_value);
extern void *setup_stack0(void *_stack, void *func);

/* ---------- */
/* ------------- 
    Helpful definitions
   ------------- */
#define STDIN 0
#define STDOUT 1

#define TRUE 1
#define FALSE 0

/* 
    low level system call wrapper functions to be defined here - implementations from part 1 & 2
*/
int read(int fd, void *ptr, int len);   
int write(int fd, void *ptr, int len);  
void exit(int err);                     
int open(char *path, int flags);        
int close(int fd);                      
int lseek(int fd, int offset, int flag);
void *mmap(void *addr, int len, int prot, int flags, int fd, int offset);
int munmap(void *addr, int len);

void do_print(char *buf);   /* copied from part 2 */

/* round A up to the next multiple of B */
#define ROUND_UP(a,b) (((a+b-1)/b)*b)

/* ---------- */
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

/*
    do_print - put null-terminated string characters to STDOUT
*/
void do_print(char *buf) {
    while( *buf ) {
        write(STDOUT, buf++, 1);
    }
}

/* ---------- */

/* new functions for part 3 */
void do_yield12(void);
void do_yield21(void);
void do_uexit(void);
/* ---------- */

char stack1[4096];      /* stack for process1 */
char stack2[4096];      /* stack for process2 */
void *curStack0 = 0;    /* pointer to home process stack */
void *curStack1 = 0;    /* pointer to  process1 stack */
void *curStack2 = 0;    /* pointer to  process1 stack */

typedef void(*entry_t)();   /* type to make entry point definition easier to read */
entry_t p1Entry = 0;      /* entry point for process1 */
entry_t p2Entry = 0;      /* entry point for process2 */


void do_yield12(void) {
    switch_to( (void **)&curStack1, curStack2 );    /* store stack 1 ptr, and switch to stack 2 ptr */

}
void do_yield21(void) {
    switch_to( (void **)&curStack2, curStack1 );    /* store stack 2 ptr, and switch to stack 1 ptr */
}
void do_uexit(void) {
    switch_to( NULL, curStack0 );    /* exiting - no need to store, switch to home process stack ptr */
}

/* 
    very similar to exec_file from part 2, but instead of running the file, we create and return 
    the entry point.
 */
entry_t load_file( char *fn, unsigned long load_offset ) {
    int nbr_exectutable_sections = 0;
    void *mmapped_locations[100];

    /* open the file */
    int fd=open(fn, O_RDONLY);
    if( fd < 0) {
        do_print( "Error opening file <" );
        do_print( fn );
        do_print( ">\n" );
        return 0;
    }

    /* read the full header */
    struct elf64_ehdr hdr;
    read(fd, &hdr, sizeof(hdr));

    /* get the entry point */
    void *entry_point = hdr.e_entry;

    int i, n = hdr.e_phnum;
    struct elf64_phdr phdrs[n];
    lseek(fd, hdr.e_phoff, SEEK_SET);
    read(fd, phdrs, sizeof(phdrs));

    /* iterate over the program headers */
    for (i = 0; i < hdr.e_phnum; i++) {
        if (phdrs[i].p_type == PT_LOAD) {
            int len = ROUND_UP(phdrs[i].p_memsz, 4096); /* nbr of bytes to allocate */
            void *virtual_address = phdrs[i].p_vaddr;   /* allocate target address...*/
            int section_size = (int)phdrs[i].p_filesz;  /* bytes to read */
            int read_offset =  (int)phdrs[i].p_offset;  /* offset to read from */

            void *target = mmapped_locations[ nbr_exectutable_sections++ ] = virtual_address + load_offset;

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
    /* close the disk file */
    close(fd);
 
    /* create and return the entry point for this file */
    entry_t f = entry_point + load_offset;
    return f;
}

/* main entry point

    load and generate the entry points for two processes
    set up the stacks for these values
    switch from the current process to process 1
    the processes will switch back and forth using the do_yieldxx system calls
    at the end, do_uexit will run, returning control to this process
 */
void main(void)
{
    vector[1] = do_print;

    vector[3] = do_yield12;
    vector[4] = do_yield21;
    vector[5] = do_uexit;
    
    /* your code here */
    p1Entry = load_file( "process1", 0x80000000 );
    p2Entry = load_file( "process2", 0xa0000000 );

    curStack2 = setup_stack0( stack2+4096, p2Entry );
    curStack1 = setup_stack0( stack1+4096, p1Entry );
    switch_to( (void **)&curStack0, curStack1 );

    do_print("done\n");
    exit(0);
}
