# Part 2 -- POSIX Threads

There is a straightforward translation from monitor pseudocode to POSIX thread primitives:

1. Create a per-object mutex, *m* of type `pthread_mutex_t` which is locked on entry to each method and unlocked on exit (be careful when using multiple exits). Actually, since there is only one object -- the barbershop -- there should only be one mutex.
2. Condition variables translate directly to objects of type `pthread_cond_t`. `C.signal()` and `C.broadcast()` become `pthread_cond_signal(C)` and `pthread_cond_broadcast(C)`.
3. The monitor mutex must be passed to wait calls; thus `C.wait()` becomes `pthread_cond_wait(C, m)`.

For this exercise we will create a singleton monitor, using global variables instead of object variables and functions rather than object methods.

You will use a single file, "homework.c" for both part  2 and part 3, using conditional compilation to separate the code for the two -- code for part 2 will be compiled with "make part-2" creating an executable named "part-2". The startup code for this question will go in the function `part2()` and will do the following:

- Initialize the monitor objects. Note that mutexes and condition variables may be initialized either statically or dynamically:

  ```
  pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER; // static init
  pthread_cond_t C = PTHREAD_COND_INITIALIZER;
  
  pthread_mutex_t m; // dynamic
  pthread_mutex_init(&m, NULL); // NULL = default params
  
  pthread_cond_t C;
  pthread_cond_init(&C, NULL);
  ```

- Create *N = 10* customer threads; each thread will loop doing the following:

  ```
  ... sleep for random(T seconds) ...
  customer()
  ```

  You are provided with a sleep function, `sleep_exp(T)`, where `T` is a floating point number giving the mean sleep time in seconds. Each thread will need to know its thread number, from 1 to 10; there is a comment in the code describing how to pass this value when starting a thread.

- Call `wait_until_done()` which will sleep until a command-line-provided timeout or until user types "ctrl-C"

### **Running Part 2:**

The "part-2" command is used as follows:

```
./part-2 [-speedup <speedup>] [<time>]
```

where `<time>` is a total number of seconds the homework should run for, and `<speedup>` is a speedup factor (e.g., "./part-2 100" would run for 100 seconds, "./part-2 2.5 100" would do the same amount of work, but run 2.5 times faster completing 100 simulated seconds in 40 real seconds).

To use the debug script provided for this question, you will need to print the following lines as your code executes:

```
DEBUG: TTT customer # enters shop
DEBUG: TTT customer # starts haircut
DEBUG: TTT customer # leaves shop
DEBUG: TTT barber wakes up
DEBUG: TTT barber goes to sleep
```

where "TTT" is a floating point time stamp returned by the `timestamp()` function. Use the provided `print_*` functions to produce these debug lines.

If you redirect the output of the command into a file, you should be able to use the "q2test.py" script to determine whether or not your implementation obeyed all the rules:

```
./part-2 ... > q2.out
./q2test.py q2.out
SUCCESS
```

You can also pipe the output of "part-2" directly to "q2test.py":

```
./part-2 ... | python2 q2test.py
SUCCESS
```

Note that if you run your program for a short period of time, it is less likely to break any rules even if it is incorrect.

I suggest running part-2 for a while with a high speedup, and possibly performing some work in another terminal window on the same machine (to disturb the thread scheduling order) in order to determine whether it operates correctly and whether it deadlocks.

### Submission

Your submission for this part of this assignment is the code that you write in "homework.c".