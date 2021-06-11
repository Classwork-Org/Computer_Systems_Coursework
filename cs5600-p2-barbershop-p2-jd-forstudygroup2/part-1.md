# Part 1 -- Synchronization

In this part, you are asked to design a monitor that has two methods:  `barber()` and `customer()`, modeling a barbershop with 4 waiting chairs and 1 barber's chair.

Remember the following characteristics of the monitor definition used in this class:

* Only one thread can be *in* the monitor at a time, threads enter the monitor at the beginning of a method or when returning from `wait()` and leave the monitor by returning from a method or entering `wait()`
* This means there is no preemption -- a thread in a method executes without interruption until it returns or waits.
* When thread A calls `signal()` to release thread B from waiting on a condition variable, you don't know whether B will run before or after some thread C that tries to enter the method at the same time (that's why they are called *race conditions*).
* You don't need a separate mutex -- that is a monitor, not a pthreads translation of a monitor

### Submission

In addition to *pseudocode*, you should also submit a *graphical representation* for the monitor depicting the interaction between the two methods. Commit and push this as a file called "part-1.pdf" in your repo for this assignment.