# Part 3 -- RAID 4

In this part, you are asked to implement a RAID 4 device. To do this you need to write the following functions in "homework.c":

* `struct blkdev *raid4_create(int N, struct blkdev *disks, int unit);`

This creates a RAID4 volume across N disks, striped in chunks of `unit` sectors, where `disks[N - 1]` is the parity drive. Again, return `NULL` for a size mis-match, and do not use any sectors beyond the last multiple of the stripe size.

* `int raid4_replace(int i, struct blkdev *newdisk);`

Again, replace disk `i` where `0 <= i < N` with `newdisk`, returning `SUCCESS`, or return `E_SIZE`. As in the mirrored case you will need to reconstruct and write data to the new drive before returning.

Additionally the "homework.c" file contains several additional function signatures that begin with `raid4_` that you should implement.


## Testing Deliverables

Once you finished implementing the above functions, it's time to start writing tests.  **You will likely spend more time writing tests than implementing the functions in "homework.c".**

For this part of the assignment, you should write tests that test your code. RAID 4 tests are basically the same as the RAID 0 stripping tests, combined with the failure and recovery tests from RAID 1 mirroring.

You should put put these tests in "raid4-test.c" that creates and tests strip volumes and in "raid4-test.sh" which creates any needed images files and passes them as arguments to the "raid4-test" executable.