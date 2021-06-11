# Overview


In this project, we will be implementing mirroring, striping, and RAID in a block-device layer.  The abstraction you will use is the following structure:

```
#define BLOCK_SIZE 512   /* 512-byte unit for all blkdev addressing in HW3 */

struct blkdev {
    struct blkdev_ops *ops;
    void *private;
};

struct blkdev_ops {
    int (*num_blocks)(struct blkdev *dev);
    int (*read)(struct blkdev * dev, int first_blk, int num_blks, void *buf);
    int  (*write)(struct blkdev * dev, int first_blk, int num_blks, void *buf);
    void (*close)(struct blkdev *dev);
};
```

This is a common style of operating system structure, which provides the equivalent of a C++ abstract class by using a structure of function pointers for the virtual method table and a `void*` pointer for any subclass-specific data.  Interfaces like this are used so that independently compiled drivers (e.g., network and graphics drivers) to be loaded into the kernel in an OS such as Windows or Linux and then invoked by direct function calls from within the OS.

The methods provided in the `blkdev_ops` structure are:

* `size` -- the total size of this block device, in 512-byte sectors

* `read` -- read `len` sectors into the buffer.  The caller guarantees that `buf` points to a buffer large enough to hold the amount of data being requested, and that `len > 0`.  Legal return values are `SUCCESS`, `E_BADADDR`, and `E_UNAVAIL` (defined in "blkdev.h").

* `write` -- write `len` sectors.  The caller guarantees that `buf` points to a buffer holding the amount of data being written, and that `len > 0`.  Legal return values are `SUCCESS`, `E_BADADDR`, and `E_UNAVAIL`.

* `close` -- the destructor method, this closes all `blkdev`s "underneath" this one and frees any memory allocated.  Note that once this method is called, the `blkdev` object must not be access (not even to call `size`).

The `E_BADADDR` error is returned if any address in the requested range is illegal -- i.e., less than zero or greater than `blkdev->ops->size(blkdev) - 1`.

The `E_UNAVAIL` error is returned if a device fails.  If your code receives this error, it should call `close()` on the corresponding `blkdev`; after this it cannot access the device anymore.  Your code should only return this error if it is unable to read (or write) the requested data due to a single disk failure (for striping) or multiple disk failures (for mirroring and RAID 4).

We will be working with disk image files, rather than actual devices, for ease of running and debugging your code.  You may be familiar with image files in the form of .ISO files, which are byte-for-byte copies of a CD-ROM or DVD, and can be read by the same file system code which interacts with a physical disk; in our case we will be writing to the files as well as reading them.

You will be writing what are termed "filter drives", which site between the actual disk driver (or in our case, the image file access routines) and the file structure above, so your code will read and write via a `struct blkdev`, and will export a `struct blkdev` to higher layers.

For reference consider the following example:

![Example](example.png)

If we have 3 disks -- * 1 (A1, B1, C1), * 2, and * 3 in a striped or RAID configuration, then we call each of the single units (e.g., A1 or B3) a **stripe**, and the entire row across (e.g., A1, A2, A3) a **strip set**.

## [Part 1 -- RAID 1 Mirroring](part-1.md)

In this part, you will be asked to implement and test a RAID 1 mirroring device.

## [Part 2 -- RAID 0 Stripping](part-2.md)

In this part, you will be asked to implement and test a RAID 0 stripping device.

## [Part 3 -- RAID 4](part-3.md)

In this part, you will be asked to implement and test a RAID 4 device.



# Testing Deliverables

Once you finished implementing the above functions, it's time to start writing tests.  **You will likely spend more time writing tests than implementing the functions in "homework.c".**

C programs should handle test failures by using `assert()` statements or printing "ERROR" and calling exit(1) if a test failure is detected. Shell scripts should use the echo command to print "ERROR" and then call `exit 1`.

You will need image files for your tests – you can create a zero-filled file with the `dd` command like this:

`dd if=/dev/zero of=test.img bs=512 count=200`

(the abbreviations stand for input file, output file, and block size) This creates a file named "test.img" containing 200 512-byte blocks (i.e. 102400 bytes) of zeroes (or you can just create the file in a C program, write the correct number of zeroes to it, and close it).

You may find it useful to create files containing easy-to-distinguish values. To create a 1024-byte file containing only the character 'C', you can use the command:

`dd if=/dev/zero bs=512 count=2 | tr '\0' 'C' > test.img`

For RAID 0 and 4 testing you may want to create multiple small (e.g. stripe-sized) files with different contents and concatenate them into one.

If you want to compare two binary files in a shell script you can use the cmp command, which returns true if the files are identical:

```
if ! cmp file1 file2 ; then
   echo ERROR: files do not match
   exit 1
fi
```

If you are trying to figure out what exactly is in your disk image, you may find the `od` ("octal dump") program useful; e.g. here is a dump of a file with 3 512-byte blocks initialized to 'C', 'D', and 'E' respectively, with addresses (on left) in decimal (-A d) and values printed as characters (-c):

```
$ od -c -A d x
0000000    C   C   C   C   C   C   C   C   C   C   C   C   C   C   C   C
*
0000512    D   D   D   D   D   D   D   D   D   D   D   D   D   D   D   D
*
0001024    E   E   E   E   E   E   E   E   E   E   E   E   E   E   E   E
*
0001536
```

Another tool, `xxd`, is also useful.

```
$ xxd file1
00000000: 0001 0203 0405 0607 0809 0a0b 0c0d 0e0f ................
00000010: 1011 1213 1415 1617 1819 1a1b 1c1d 1e1f ................
00000020: 2021 2223 2425 2627 2829 2a2b 2c2d 2e2f !"#$%&'()*+,-./
00000030: 3031 3233 3435 3637 3839 3a3b 3c3d 3e3f 0123456789:;<=>?
00000040: 4041 4243 4445 4647 4849 4a4b 4c4d 4e4f @ABCDEFGHIJKLMNO
00000050: 5051 5253 5455 5657 5859 5a5b 5c5d 5e5f PQRSTUVWXYZ[\]^_
00000060: 6061 6263 6465 6667 6869 6a6b 6c6d 6e6f `abcdefghijklmno
00000070: 7071 7273 7475 7677 7879 7a7b 7c7d 7e7f pqrstuvwxyz{|}~.
```

In a test script the following snippet of code may be handy:

```
$ perl -e 'while (!eof(STDIN)) {$a = getc(STDIN);\
                                     printf “%s\n”, $a;}' < x | uniq -c
512 C
512 D
512 E
```

You may need to replace `%s` with `%x` or `%d` if you are working with RAID 4.



CS5600 Project Rules
================

I expect you to work in teams of two students; you will submit one copy of the homework and receive the same grade. Feel free to discuss the ideas in the homework with other groups, but absolutely no sharing of code across groups is allowed. All your code should come from your own fingers typing on the keyboard - if you're copy-and-pasting it from somewhere (from another team or from the Internet) it's likely to be academic dishonesty.

Submitting your work
------------------

You do not need to explicitly submit your assignment --- your grade will be based on the code in your repository at the time it is due.

Commit and push your work frequently
---------------------------------

Commit your changes frequently ­ at least once every two days while you are working on the project, and preferably much more often. These check-ins serve to document that you and your teammate wrote the code you are submitting, rather than copying it from someone else. Failure to check in frequently will result in a lower grade.

Progress report
-------------

You will be required to submit a progress report partway through the assignment ­ this is a text file in your repository (named progress.txt) describing the work you have done so far and your plans for finishing the project. It can include things like "discussed design of part X", or "implemented Y function", etc.  Remember to use `git add`, `git commit`, and `git push` so I can see your report.  It's a good idea to create this file at the very beginning and use it to make notes on the work you've done.

Testing
------

You are expected to implement tests for your code and to check them in as part of the assignment. (As a general rule, code that hasn't been tested doesn't work. Therefore if you didn't test your code but it passes my tests, then it's a lucky accident and doesn't deserve as good a grade as if you actually tested it and know it works)  Run your tests under Valgrind, as well - it will catch a lot of bugs before they actually happen.

Code Quality
-----------

Unlike most workplaces or open-source projects, the coding standards for this class are minimal. They are:

1. Initialize every variable when you declare it.
2. Get rid of all compiler warnings. There's almost always a good reason the compiler is complaining - fix it. 
3. Indent your code consistently and reasonably.
4. Use reasonable variable and function names. If you change what a function does, **change the name of the function to reflect that**.
5. Comments are for comments, not for code. Your editor has a perfectly good "delete line" function, and Git can keep track of things that you deleted.

Other than #1 and #2, these can all be described as **Don't write ugly code**. Take some pride in your work; don't turn in something that makes you look bad.

Grading
------

Things you can lose points for:

- Failure to achieve any progress by the progress report date, or failure to commit your code frequently.
- Really ugly code, compiler warnings, uninitialized variables
- Failure to implement all of the requested functionality. 
- Test failures (on the grading tests) 
- Not enough testing.