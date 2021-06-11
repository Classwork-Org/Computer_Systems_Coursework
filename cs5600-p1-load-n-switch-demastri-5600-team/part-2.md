Part 2 - Program loading / syscalls
======================

Write a program which:

- reads a line of input
- splits it into words
- exit if the first word is "quit"
- load the file named by the first word into memory
- provide "system calls" for the loaded program
- call the loaded program's entry point.
- repeat

You'll find a function for splitting a line in "part-2.c". You'll need your read/write/exit system call implementations from part 1, and the `readline` and `print` functions you created. (assuming you factored that code out)

You'll need to write several more system calls

- `open`, `close`. (you can assume `open` always takes 2 arguments)
- `lseek`, to read specific parts of the executable file
- `mmap`, to allocate memory at specific addresses, and `munmap` for when you're done

I've provided prototypes for each of the system calls; writing them should be straightforward.

Loading an executable file into memory is a lot trickier. You might want to look at the file "elf-example.c" first, which prints out the information we need from the executable - the "entry point" (i.e. function to call to start execution), and the entries in the program header table which are marked `PT_LOAD`.

ELF files have two sets of headers - "section headers", which get used by the compiler and linker, and "program headers", which specify how to load it into memory; we can ignore the section headers. To find the program headers you read the ELF header (`struct elf64_ehdr`) at the beginning of the file into memory - the program headers an array of `struct elf64_phdr`; if you read the ELF header into a variable named `e_hdr`, then that array has `e_hdr.e_phnum` entries starting at offset `e_hdr.e_phoff` bytes into the file.

You will be loading a *relocatable* executable, which starts at address 0 but can actually be loaded starting anywhere in memory. I would suggest loading it at 0x80000000, which will keep it far away from your program. (the rest of your program is statically linked at 0x400000, and the stack will be at an address somewhere a bit below 0x800000000000) Each program section of the executable specifies a virtual address (`phdrs[i].p_vaddr` in the ELF example code); the program will run correctly as long as you add the same constant to each virtual address to determine where to load it into memory. (remember that you'll have to add the constant to the entry point address, as well)

Additional Details
---------------

You'll use the `mmap` system call to allocate memory. Although the segments may have varying protections in the ELF header, we'll allocate them all as R/W/X to make things simpler. 

```
void *buf = mmap(address, size, PROT_READ | PROT_WRITE |
                 PROT_EXEC, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
if (buf == MAP_FAILED) {
    print(“mmap failed\n”); /* your "print" function */
    exit(1);                /* your "exit" function */
}
```

`mmap` operates in terms of 4096-byte pages, so you'll need to “round up” the section size to the next multiple of 4096 -- there's a macro in `sysdefs.h` that you can use:

```
int len = ROUND_UP(size, 4096);
```

You'll need to use `lseek` and `read` to read each program segment into the allocated memory chunk:

```
lseek(fd, offset, SEEK_SET);
read(fd, address, size_in_file);
```

Note that for some sections (e.g. data) the header gives two sizes – the size of the memory region to allocate (`p_memsz`), which we round up to a multiple of 4K, and the number of bytes to read from the file (`p_filesz`). The second number may be smaller, which means that we only read that amount of data in from the file, and leave the rest of the memory region as zero. (that's how it comes from `mmap`)

Now you'll need to make a function call to the program entry point. You'll set a function pointer to the value of `e_hdr.e_entry` (well, that value plus the offset you used with all your other sections), and then call it:

```
void (*f)();
f = hdr.e_entry + offset;
f();
```

When you load the program into memory you should keep a list (well, an array) of the memory addresses that you allocated and their lengths, so that you can `munmap` them when you're done. If you don't, you'll probably crash the second time through your command loop, when you try to allocate memory at the same location a second time.

First part - the `wait` executable
--------------------------
All that the `wait` executable does is waste time, by looping a billion times or so, and then return, which lets you tell that you loaded and ran it properly. (if it finishes immediately, you didn't)

A hint for getting this working - assuming you factored out a function to load and run an executable (you did, didn't you?), you can put off writing your command loop and just hard-code a call to load and run it, e.g.:

```
exec_file("wait");
```

(you're going to have to write a `readline` function for the next part, which you can re-use in your command loop)

Second part - system calls
---------------------
For this part you'll need to do two things:

**Write a command loop**. This will

- read a line of input,
- split it into words,
- save the 2nd, 3rd etc. word as arguments for the micro-program, and
- load and execute the program named by the first word (and preferably doesn't crash if you type the wrong thing, which you'll know if `open` returns a value less than zero)

**Write three "system calls"**

- `readline(char *buf, int n)` – this reads up to `n-1` bytes into `buf[0]`, `buf[1]`, ..., then adds a terminating zero byte at the end. It stops reading into `buf[]` when either (a) the last character read was '\n', or (b) `n-1` bytes have been written
- `print(char *buf)` – print the null-terminating string to standard output – i.e. go through `buf[0]`, `buf[1]`, ..., writing out each non-zero byte and then stopping at the zero byte that terminates the string.
- `char *getarg(int i)` - returns argument `i`, or 0 if there weren't that many arguments.

Normally you would use a standard library function to split the line into parts (`strtok` or `strsep`), so I've provided you with a function that does it for you. It's OK to handle a max of say 10 arguments, including `arg0` -– the current micro-programs don't go higher than 2.

You should be able to test your Part 2 implementation by running `hello` and `ugrep`; `hello`does what you expect it to,  and `ugrep` should behave similarly to the `grep` program, outputting lines of input which match its argument. (instead of end-of-file, it will exit when it sees a blank line)

```
pjd@pjd-VirtualBox$ ./part-2
> wait
> hello
Hello world!
> ugrep xyz
ugrep: enter blank line to quit a line without the pattern
a line without the pattern
a line with 'xyz' in it 
-- a line with 'xyz' in it
next line blank to quit:
> quit
pjd@pjd-VirtualBox$
```

Testing
------
There aren't many cases to test here:

1. Does it run `wait`, `hello`, and `ugrep` without crashing?
2. Can you run them multiple times, in different orders?
3. If you type something that isn't one of the three microprogram names, or "quit", does it crash?
4. If you type "quit", does it exit without crashing?

I suggest running these tests under `valgrind`, as that will help ensure that you don't have any "hidden" bugs that only show up when the TA is grading your assignment.

Debugging
--------
**`readelf`**. The `readelf` command prints out all the headers in an executable; you can use that to check whether your code is reading the information correctly:
```
code$ readelf -a wait
ELF Header:
  Magic:   7f 45 4c 46 02 01 01 00 00 00 00 00 00 00 00 00 
  Class:                             ELF64
  [...]
  Entry point address:               0x1000
  Start of program headers:          64 (bytes into file)
  Start of section headers:          26264 (bytes into file)
  Flags:                             0x0
  Size of this header:               64 (bytes)
  Size of program headers:           56 (bytes)
  Number of program headers:         11
  [...]
Program Headers:
  Type           Offset             VirtAddr           PhysAddr
				 FileSiz            MemSiz              Flags  Align
[...]
  LOAD           0x0000000000000000 0x0000000000000000 0x0000000000000000
				 0x0000000000000321 0x0000000000000321  R      0x1000
  LOAD           0x0000000000001000 0x0000000000001000 0x0000000000001000
				 0x00000000000000aa 0x00000000000000aa  R E    0x1000
  LOAD           0x0000000000002000 0x0000000000002000 0x0000000000002000
				 0x0000000000000050 0x0000000000000050  R      0x1000
[...]  
```

There are also a few GDB tricks which make it a lot easier to debug this assignment:

**Loading an additional symbol file**. If you crash in the middle of the microprogram, you want the debugger to know what's going on. You can load symbols for the microprogram, as long as you know the offset in memory where you loaded it -- in the example here it's 0x1000000. (Note that the address you give has to be the offset **plus 0x1000**):

```
code$ gdb part-2
GNU gdb (Ubuntu 8.3-0ubuntu1) 8.3
 ...
Reading symbols from part-2...
(gdb) b do_getarg
Breakpoint 1 at 0x401600: file part-2.c, line 153.
(gdb) run
Starting program: .../part-2 
> ugrep foo

Breakpoint 1, do_getarg (i=0) at part-2.c:153
153	{
(gdb) add-symbol-file ugrep 0x1001000
add symbol table from file "ugrep" at
	.text_addr = 0x1001000
(y or n) y
Reading symbols from ugrep...
(gdb) bt
#0  do_getarg (i=0) at part-2.c:153
#1  0x00000000010010ab in main () at ugrep.c:36
#2  0x00000000004014d1 in execfile (file=0x7fffffffe378 "ugrep", offset=16777216) at part-2.c:110
#3  0x0000000000401825 in main () at part-2.c:208
(gdb) 
```

Without that second symbol file, GDB would be totally confused:

```
(gdb) bt
#0  do_getarg (i=0) at part-2.c:153
#1  0x00000000010010ab in ?? ()
#2  0x0000000000000000 in ?? ()
(gdb) 
```

**Single-instruction stepping**. To figure out what's going wrong when you call a function pointer, you may need to single step an instruction at a time, because the same bug that's keeping your code from working is probably keeping GDB from putting line-by-line breakpoints in the right places. First we'll tell GDB to print out the next instruction after every command (`display/i $pc`), then we'll use the `stepi` command to verify that it's calling into the code of the `wait` microprogram properly:

```
(gdb) display/i $pc
1: x/i $pc
=> 0x4014b6 <execfile+801>:	mov    -0x128(%rbp),%rdx
(gdb) stepi
109	    void (*f)(void) = hdr.e_entry + offset;
1: x/i $pc
=> 0x4014bd <execfile+808>:	mov    -0x150(%rbp),%rax
(gdb) 
0x00000000004014c4	109	    void (*f)(void) = hdr.e_entry + offset;
1: x/i $pc
=> 0x4014c4 <execfile+815>:	add    %rdx,%rax
(gdb) 
109	    void (*f)(void) = hdr.e_entry + offset;
1: x/i $pc
=> 0x4014c7 <execfile+818>:	mov    %rax,-0x68(%rbp)
(gdb) 
110	    f();
1: x/i $pc
=> 0x4014cb <execfile+822>:	mov    -0x68(%rbp),%rax
(gdb) 
0x00000000004014cf	110	    f();
1: x/i $pc
=> 0x4014cf <execfile+826>:	callq  *%rax
(gdb) 
0x0000000001001000 in ?? ()
1: x/i $pc
=> 0x1001000:	endbr64 
(gdb) 
0x0000000001001004 in ?? ()
1: x/i $pc
=> 0x1001004:	push   %rbp
```

Your main program (whether part-1, part-2, or part-3.c) is going to live somewhere around 0x401000 in memory; here we see code computing the entry point value `f`, then making a function call to 0x1001000. (even without loading symbols, you can use `objdump -d wait` on the command line to verify that the first two instructions are `endbr64` and `push %rbp`.)

Oh, and I don't know what `endbr64` means, either, or at least I didn't until I got curious after writing up this example. If you're looking at assembly language code while you're debugging, you usually don't have to really understand it for it to be helpful - for example in this case we really only need to make sure that the addresses are correct.

Submission
---------
Your submission for this part will be the contents of "part-2.c" and any code you wrote for testing if it in a different file.