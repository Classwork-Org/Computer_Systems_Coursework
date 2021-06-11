# Part 3 -- Discrete Event Simulation

For this part, you will compile your code using "make part-3", which will build it with a framework (from "misc.c" using the GNU Pth library) for discrete-event simulation. What this means is that your code will run in simulated time -- basically the thread library "skips" time forward whenever threads are sleeping, and stops the clock when a thread is running. For simulations of small, slow systems (like ours) this results in a simulation running much faster than real time; for fast, complex systems (e.g., simulating operation of an integrated circuit) the simulation might run thousands of times slower than real time.

The simulation library is designed so that it is compatible with pthreads operations, so you should be able to compile and run the same monitor code you used in Part 2. In addition, several functions have been provided for gathering statistics:

```
void *counter = stat_counter();
stat_counter_incr(counter);
stat_counter_decr(counter);
double val = stat_count_mean(counter);

void *timer = stat_timer();
stat_timer_start(timer);
stat_timer_stop(timer);
double val = stat_timer_mean(timer);
```

A counter tracks an integer variable (e.g., the number of customers waiting in the shop) and provides its average value over time. A timer tracks the interval between a single thread calling `start()` and `stop()`, and provides the average value of these measured intervals.

Run your code for at least 1000 seconds (10000 would be better) of simulated time and measure average value for the following:

- fraction of customer visits resulting in turning away due to a full shop (you'll have to keep your own counter for this one).
- average time spent in the ship (including haircut) by a customer who does not find a full shop
- average number of customer in the shop (including the barber's chair)
- fraction of time someone is sitting in the barber's chair (hint -- use a `stat_counter` with a value 0 for empty and 1 for full)

You simply need to demonstrate that your recorded statistics are sensible.

To compile part 3, you will need the GNU Pth library (gnu portable threads). Pth 2.0. is provided in the repository, as well as a script to compile the library. If you are on your own machine running linux, you can install pth to the system with `sudo apt install pth-dev`; although you will have to modify the Makefile to use the system provided pth library.

To build the provided pth library, run "build-pth.sh". It will untar the "pth-2.0.7.tar.gz", cd to the new directory, and run "./configure; make; make install". The pth library is known to build on Ubuntu 20.04.

### Submission

Your submission for this part of this assignment is the code that you write in "homework.c".