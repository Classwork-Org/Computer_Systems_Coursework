Part 1 - read/write/exit
================

Write a program which:

- reads a line of input from standard input (file descriptor 0)
- if the line starts with 'q', 'u', 'i', 't' then exit
- otherwise print the line to standard output (file descriptor 1) and loop

You will need to implement the following in "part-1.c":

- system call wrappers for `read`, `write`, and `exit`
- logic to read a line of input using `read`. (I suggest reading 1 byte at a time and sticking it in a buffer; if it's '\n' then add a zero on the end and you're done)
- a function to print a string using `write`. (again I suggest doing it a byte at a time)
- a simple loop, with a check after reading a line to see if the user entered "quit".

You can assume that input lines are never more than 200 bytes long. Remember that you can't return from the `main` function -- you have to call `exit` or your program will crash after it's done.

You'll probably want to factor out `readline` and `print`, since you'll need them as separate functions in parts 2 and 3.

Remember that you can't use any functions other than the ones that you write yourself. That means no `printf` -- use GDB for your debugging (and no `strcmp` for string comparisons, or`malloc` for allocating memory).

Compiling
--------
You can compile part 1 with the command `make part-1`. You can "clean up" (i.e. delete the compiled executables and intermediate files) with the command `make clean`.

Testing
------
You won't need to do a lot of testing for this one - it either works or it doesn't. Before you submit your work I would suggest you run it under `valgrind`, which will tell you if you're reading or writing memory that you didn't intend to:

```
pjd@pjd-vm10:hw2$ valgrind ./part-1
==12338== Memcheck, a memory error detector
==12338== Copyright (C) 2002-2017, and GNU GPL'd, by Julian Seward et al.
==12338== Using Valgrind-3.15.0 and LibVEX; rerun with -h for copyright info
==12338== Command: ./part-1
==12338== 
Hello, type lines of input, or 'quit':
> this is a test
you typed: this is a test
> quit
==12338== 
==12338== HEAP SUMMARY:
==12338==     in use at exit: 0 bytes in 0 blocks
==12338==   total heap usage: 0 allocs, 0 frees, 0 bytes allocated
==12338== 
==12338== All heap blocks were freed -- no leaks are possible
==12338== 
==12338== For lists of detected and suppressed errors, rerun with: -s
==12338== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
pjd@pjd-vm10:hw2$
```

Submission
---------
Your submission for this part will be the contents of "part-1.c" and any code you wrote for testing if it in a different file.
