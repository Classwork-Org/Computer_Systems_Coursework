Part 3 - Context Switching
======================
For this last part we'll throw away the command loop, and we'll load and run two processes which switch back and forth between each other:

```
pjd@pjd-vm10:$ ./part-3
program 1
program 2
program 1
program 2
program 1
program 2
program 1
program 2
done
pjd@pjd-vm10:$ 
```

Program loading
---------------
We'll load two different programs, `process1` and `process2`, at two different locations in memory. I would suggest modifying your load-and-execute function from Part 2 to (a) take an offset as a parameter, and (b) return the entry point rather than calling it. Don't worry about unmapping the memory when you're done - we're just going to run things once and then exit, so Linux can clean everything up for us when we're done.

Context switching
---------------
We'll need to allocate stacks for the two processes - 4096 bytes should be more than enough. You can either use global arrays:

```
char stack1[4096];  
char stack2[4096];  
```

or you can allocate them with `mmap`:

```
void *stack1 = mmap(0, len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
```

(the zero address tells `mmap` to pick an unused address)

We'll use the `setup_stack_0` from `stack.c`, and the `switch_to` function defined in `switch.S`:

```
extern void switch_to(void **location_for_old_sp, void *new_value);
extern void *setup_stack0(void *_stack, void *func);
```

`setup_stack0` takes a pointer to the **top** of a region of memory (not the bottom - in our case we would pass `stack1+4096`), and returns a stack pointer which will cause `switch_to` to invoke `func` with no arguments. 

We need to implement the following system calls:
- `yield12` - this switches from process 1 to 2 
- `yield21` - this switches from process 2 to 1
- `uexit` - this switches back to the original process stack (see below)

There are three different stacks - the original process stack that the operating system gave us, and the stacks we allocated for processes 1 and 2 - and we'll need locations (global variables) for storing pointers to each of them. We're going to switch from the OS stack to the process 1 stack; it will then switch to process 2, and they'll switch back and forth a few times, then one of them will call `uexit` which will switch back to the main OS stack and we can call the real `exit` system call to finish up.

Testing
------
There isn't much to test here - there aren't any parameters or variable inputs to the assignment, so it either runs to completion without crashing, generating the output shown above, or it doesn't.

Debugging
---------
There's a lot to debug here. Unfortunately, `valgrind` won't be of much use for this part of the assignment, as the moment you switch the stack pointer it thinks you have a horrible bug.

You'll want to use the same tricks for loading additional symbols - in this case you can load them for both the "process1" and "process2" executables - and for single-stepping by instruction.

Finally, you may find yourself crashing with this error:

```
Program received signal SIGSEGV, Segmentation fault.
switch_to () at switch.S:34
34		mov  0(%rax),%rax	
```

That's an assert in the `switch_to` function, telling you that you've tried to switch to a bogus stack. Both `setup_stack` and `switch_to` leave a flag value on the top of the stack; if you try to switch to a stack that doesn't have this flag, it crashes **before** the switch happens. (Once you switch stack pointers, there's no way to tell where you called `switch_to` from, or to look at the variables in that function.)

Submission
---------
Your submission for this part will be the contents of "part-3.c" and any code you wrote for testing if it in a different file.