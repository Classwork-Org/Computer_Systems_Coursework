# Part 2 -- Stripping

In this part, you are asked to implement a single RAID0 stripping device. To do this you need to write the following functions in "homework.c":

* `struct blkdev *raid0_create(int N, struct blkdev *disks, int unit);`

This creates a striped volume across N disks, with stripes of `unit` sectors. If the disks are not the same size, print a message and return `NULL`. If the disks are not a multiple of `unit` blocks, the last few sectors on each disk will not be used.  For example, given two disks of 5 sectors each and a unit size of 2, you will create a striped volume holding 8 sectors, and the last sector of each disk will not be touched.

Note that there is no `raid0_replace()` function, as striped volumes cannot recover from errors.

Additionally the "homework.c" file contains several additional function signatures that begin with `strip_` that you should implement.


## Testing Deliverables

In this assignment, you are also responsible for writing tests to show that your implementation works correctly. **You will likely spend more time writing tests than implementing the functions in "homework.c".**

For this part of the assignment, you write tests that test that your code.  You should test that your implementation

* passes all other tests with different strip sizes (e.g. 2, 4, 7, and 32 sectors) and different numbers of disks
* reports the correct size
* reads data from the right disks and locations. (prepare disks with known data at various locations and make sure you can read it back)
* overwrites the correct locations -- write to your prepared disks and check the results – using something other than your stripe code – to check that the write sections got modified
* fail a disk and verify that the volume fails
* large (> 1 stripe set), small, unaligned reads and writes (i.e. starting, ending in the middle of a stripe), as well as small writes wrapping around the end of a stripe.

You should put put these tests in "strip-test.c" that creates and tests strip volumes and in "strip-test.sh" which creates any needed images files and passes them as arguments to the "strip-test" executable.