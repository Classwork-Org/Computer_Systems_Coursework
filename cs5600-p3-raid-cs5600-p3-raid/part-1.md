# Part 1 -- Mirroring

In this part, you are asked to implement a simple RAID1 mirroring device. To do this, you need to write the following functions in "homework.c":

* `struct blkdev *mirror_create(struct blkdev *disks[2]);`

This creates a mirrored volume across two devices.  If the devices are not the same size, print an error message and return `NULL`.

 * `int mirror_replace(int i, struct blkdev *newdisk);`

This replaces disk `i` where `i` = 0 or 1, which may or may not have failed, and copies the existing data onto it before returning `SUCCESS`.  If the new disk is not the same size as the old one, return `E_SIZE`.

Additionally the "homework.c" file contains several additional function signatures that begin with `mirror_` that you should implement.


## Testing Deliverables

In this assignment, you are also responsible for writing tests to show that your implementation works correctly. **You will likely spend more time writing tests than implementing the functions in "homework.c".**

For this part of the assignment, you write tests that test that your code.  The provided "mirror-test.c" shows an example of how to create a mirror, and also checks that data written to the mirror can be read back successfully. Additionally, you should test that your implementation

* creates a volume properly
* returns the correct length
* can handle reads and writes of different sizes, and returns the same data as was written
* reads data from the proper location in the images, and doesn't overwrite incorrect locations on write
* continues to read and write correctly after one of the disks fails (call `image_fail()` on the image `blkdev` to force it to fail)
* continues to read and write (correctly returning data written before the failure) after the disk is replaced
* reads and writes (returning data written before the first failure) after the other disk fails

You should also write a "mirror-test.sh" file that creates any needed image files and passes them as arguments to "mirror-test" executable.