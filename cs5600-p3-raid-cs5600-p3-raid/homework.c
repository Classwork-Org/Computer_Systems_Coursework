/*
 * file:        homework.c
 * description: skeleton code for CS 5600 Homework 2
 *
 * Peter Desnoyers, Northeastern Computer Science, 2011
 * $Id: homework.c 410 2011-11-07 18:42:45Z pjd $
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "blkdev.h"

/********** MIRRORING ***************/

/* example state for mirror device. See mirror_create for how to
 * initialize a struct blkdev with this.
 */
struct mirror_dev {
    struct blkdev *disks[2];    /* flag bad disk by setting to NULL */
    int nblks;
};
    
static int mirror_num_blocks(struct blkdev *dev) {
    if( dev == NULL )
        return E_UNAVAIL;
    struct mirror_dev * mirror = (struct mirror_dev*) dev->private;
    /* TODO: your code here */
    return ( mirror == NULL ) ? E_UNAVAIL : mirror->nblks;
}

/* read from one of the sides of the mirror. (if one side has failed,
 * it had better be the other one...) If both sides have failed,
 * return an error.
 * Note that a read operation may return an error to indicate that the
 * underlying device has failed, in which case you should close the
 * device and flag it (e.g. as a null pointer) so you won't try to use
 * it again. 
 */
static int mirror_read(struct blkdev * dev, int first_blk,
                       int num_blks, void *buf)
{
    /* TODO: your code here*/
    if( dev == NULL )
        return E_UNAVAIL;

    struct mirror_dev * mirror = (struct mirror_dev*) dev->private;
    int st = E_UNAVAIL;

    if( num_blks <= 0 || first_blk < 0 || first_blk+num_blks > mirror->nblks )
        return E_SIZE;


    if( mirror->disks[0] != NULL ) {
        st = mirror->disks[0]->ops->read(mirror->disks[0], first_blk, num_blks,buf);
        if( st == E_UNAVAIL ) {
            //printf( "mirror_read: first disk was unavailable\n");
            mirror->disks[0] = NULL;
        }
    }
    else {
        ;//printf( "mirror_read: first disk skipped - unavailable\n");
    }

    if( st == SUCCESS ) {
        ;//printf( "mirror_read: second disk skipped - 1st drive read successful\n");
    }
    else {
        if( mirror->disks[1] != NULL ) {
            st = mirror->disks[1]->ops->read(mirror->disks[1], first_blk, num_blks,buf);
            if( st == E_UNAVAIL ) {
                //printf( "mirror_read: second disk was unavailable\n");
                mirror->disks[1] = NULL;
            }
        }
        else {
            ;//printf( "mirror_read: second disk skipped - unavailable\n");
        }
            
    }


    return st;
}

/* write to both sides of the mirror, or the remaining side if one has
 * failed. If both sides have failed, return an error.
 * Note that a write operation may indicate that the underlying device
 * has failed, in which case you should close the device and flag it
 * (e.g. as a null pointer) so you won't try to use it again.
 */
static int mirror_write(struct blkdev * dev, int first_blk,
                        int num_blks, void *buf)
{
    /* TODO: your code here */
    if( dev == NULL )
        return E_UNAVAIL;

    struct mirror_dev * mirror = (struct mirror_dev*) dev->private;

    if( num_blks <= 0 || first_blk < 0 || first_blk+num_blks > mirror->nblks )
        return E_SIZE;

    int st1 = E_UNAVAIL, st2 = E_UNAVAIL;

    if( mirror->disks[0] != NULL ) {
        st1 = mirror->disks[0]->ops->write(mirror->disks[0], first_blk, num_blks,buf);
        if( st1 == E_UNAVAIL ) {
            ;//printf( "mirror_write: first disk was unavailable\n");
            mirror->disks[0] = NULL;
        }
    }
    else {
        ;//printf( "mirror_write: first disk skipped - unavailable\n");
    }

    if( mirror->disks[1] != NULL ) {
        st2 = mirror->disks[1]->ops->write(mirror->disks[1], first_blk, num_blks,buf);
        if( st2 == E_UNAVAIL ) {
            //printf( "mirror_write: second disk was unavailable\n");
            mirror->disks[1] = NULL;
        }
    }
    else {
        ;//printf( "mirror_write: second disk skipped - unavailable\n");
    }

    return (st1 == SUCCESS || st2 == SUCCESS) ? SUCCESS : E_UNAVAIL;

}

/* clean up, including: close any open (i.e. non-failed) devices, and
 * free any data structures you allocated in mirror_create.
 */
static void mirror_close(struct blkdev *dev)
{
    /* TODO: your code here */
    if( dev == NULL )
        return;

    //printf( "In mirror_close...\n");
    /* ### free allocated structures!! */
    struct mirror_dev * mirror = (struct mirror_dev*) dev->private;
    if( mirror->disks[0] != NULL ) {
        mirror->disks[0]->ops->close(mirror->disks[0]);
    }
    if( mirror->disks[1] != NULL ) {
        mirror->disks[1]->ops->close(mirror->disks[1]);
    }
}

struct blkdev_ops mirror_ops = {
    .num_blocks = mirror_num_blocks,
    .read = mirror_read,
    .write = mirror_write,
    .close = mirror_close
};

/* create a mirrored volume from two disks. Do not write to the disks
 * in this function - you should assume that they contain identical
 * contents. 
 */
struct blkdev *mirror_create(struct blkdev *disks[2])
{
    /* TODO: your code here */
    if( disks == NULL || disks[0] == NULL || disks[1] == NULL)  {
        printf( "mirror_create: Neither provided disk image can be NULL\n" );
        return NULL;
    }

    int blk0 = disks[0]->ops->num_blocks(disks[0]);
    int blk1 = disks[0]->ops->num_blocks(disks[1]);
    if( blk0 != blk1 ) {
        printf( "mirror_create: Provided disk images must be the same size\n" );
        return NULL;
    }

    // the blkdev structures passed into this function wrap "image_dev" structs with details about the actual files acting as drives.
    struct blkdev *dev = malloc(sizeof(*dev));
    struct mirror_dev *mdev = malloc(sizeof(*mdev));

    mdev->disks[0] = disks[0];
    mdev->disks[1] = disks[1];
    mdev->nblks = blk0;

    dev->private = mdev;
    dev->ops = &mirror_ops;

    return dev;
}

/* replace failed device 'i' (0 or 1) in a mirror. Note that we assume
 * the upper layer knows which device failed. You will need to
 * replicate content from the other underlying device before returning
 * from this call.
 */
int mirror_replace(struct blkdev *volume, int i, struct blkdev *newdisk)
{
    /* TODO: your code here */
    if( volume == NULL )
        return E_UNAVAIL;

    struct mirror_dev * mirror = (struct mirror_dev*) volume->private;

    int blks = volume->ops->num_blocks(volume);

    if( blks != newdisk->ops->num_blocks(newdisk) ) {
        printf( "mirror_replace: New image must be the same size as the image it replaces\n" );
        return E_SIZE;
    }
    char bfr[BLOCK_SIZE];
    for( int x=0; x<blks; x++) {
        volume->ops->read(volume, x, 1, bfr);
        newdisk->ops->write( newdisk, x, 1, bfr);
    }
    mirror->disks[i] = newdisk;

    return SUCCESS;
}

/**********  STRIPPING ***************/

struct stripe_dev {
    struct blkdev **disks;    /* flag bad disk by setting to NULL */
    int nbr_disks;
    int stripe_size;
    int nbr_failed;
    int nblks;
};

int image_test(struct blkdev *dev);

int get_failed_count( struct stripe_dev *dev ) {
    int nbr_failed = 0;
    for( int i=0; i<dev->nbr_disks; i++ )
        if( image_test( dev->disks[i] ) == E_UNAVAIL )
            nbr_failed++;
    return nbr_failed;
}
    
static int stripe_num_blocks(struct blkdev *dev) {
    if( dev == NULL )
        return E_UNAVAIL;
    struct stripe_dev * stripe = (struct stripe_dev*) dev->private;
    /* TODO: your code here */
    return ( stripe == NULL ) ? E_UNAVAIL : stripe->nblks;
}

static int stripe_rw_oper(struct blkdev * dev, int first_blk,
                           int num_blks, void *buf, int is_read)
{
    if( dev == NULL )
        return E_UNAVAIL;
    struct stripe_dev * stripe = (struct stripe_dev*) dev->private;
    if( stripe->nbr_failed )
        return E_UNAVAIL;

    if( num_blks <= 0 || first_blk < 0 || first_blk+num_blks > stripe->nblks )
        return E_SIZE;

    if( get_failed_count( stripe ) >= 1 )
        return E_UNAVAIL;


    if( num_blks > 1 ) {
        for( int i=0; i<num_blks; i++ ) {
            int st = stripe_rw_oper( dev, first_blk+i, 1, buf + BLOCK_SIZE*i, is_read);
            if( st == E_UNAVAIL ) {
                stripe->nbr_failed = 1;
                return E_UNAVAIL;
            }
        }
    }
    else {  /* say, 5 disks, 3 sectors/stripe request block 53 */
        int total_stripe_width = stripe->nbr_disks * stripe->stripe_size;       /* 5*3   = 15 */
        int stripe_nbr = first_blk / total_stripe_width;                        /* 53/15 = 3  */
        int stripe_offset = first_blk % total_stripe_width;                     /* 53%15 = 8  */
        int which_disk = stripe_offset / stripe->stripe_size;                   /* 8/3   = 2  */
        int stripe_disk_offset = stripe_offset % stripe->stripe_size;           /* 8%3   = 2  */
        int actual_block = stripe_nbr*stripe->stripe_size + stripe_disk_offset; /* 3*3+2 = 11 */

        int st;
        if( is_read )
            st = stripe->disks[which_disk]->ops->read(stripe->disks[which_disk], actual_block, 1, buf);
        else {
            st = stripe->disks[which_disk]->ops->write(stripe->disks[which_disk], actual_block, 1, buf);
        }

        if( st == E_UNAVAIL ) {
            stripe->nbr_failed = 1;
            return E_UNAVAIL;
        }
    }

    return SUCCESS;
}

/* read blocks from a striped volume. 
 * Note that a read operation may return an error to indicate that the
 * underlying device has failed, in which case you should (a) close the
 * device and (b) return an error on this and all subsequent read or
 * write operations. 
 */
static int stripe_read(struct blkdev * dev, int first_blk,
                          int num_blks, void *buf)
{
    return stripe_rw_oper( dev, first_blk, num_blks, buf, 1 );

}

/* write blocks to a striped volume.
 * Again if an underlying device fails you should close it and return
 * an error for this and all subsequent read or write operations.
 */
static int stripe_write(struct blkdev * dev, int first_blk,
                           int num_blks, void *buf)
{
    return stripe_rw_oper( dev, first_blk, num_blks, buf, 0 );
}

/* clean up, including: close all devices and free any data structures
 * you allocated in stripe_create. 
 */
static void stripe_close(struct blkdev *dev)
{
    /* ### free allocated structures!! */
    struct stripe_dev * stripe = (struct stripe_dev*) dev->private;
    for( int i=0; i< stripe->nbr_disks; i++ )
        if( stripe->disks[i] != NULL ) {
            stripe->disks[i]->ops->close(stripe->disks[i]);
        }
}

struct blkdev_ops stripe_ops = {
    .num_blocks = stripe_num_blocks,
    .read = stripe_read,
    .write = stripe_write,
    .close = stripe_close
};


/* create a striped volume across N disks, with a stripe size of
 * 'unit'. (i.e. if 'unit' is 4, then blocks 0..3 will be on disks[0],
 * 4..7 on disks[1], etc.)
 * Check the size of the disks to compute the final volume size, and
 * fail (return NULL) if they aren't all the same.
 * Do not write to the disks in this function.
 */
struct blkdev *stripe_create(int N, struct blkdev *disks[], int unit)
{
    int image_size = -1;
    /* TODO: your code here */
    for( int i=0; i<N; i++ ) {
        if( disks[i] == NULL )  {
            printf( "stripe_create: None of the provided disk image can be NULL\n" );
            return NULL;
        }
        int this_size = disks[i]->ops->num_blocks(disks[i]);
        if( image_size < 0 )
            image_size = this_size;
        else {
            if( image_size != this_size ) {
                printf( "stripe_create: All of the provided disk images must be the same size\n" );
                return NULL;
            }
        }
    }

    struct blkdev *dev = malloc(sizeof(*dev));
    struct stripe_dev *sdev = malloc(sizeof(*sdev));
    sdev->disks = malloc(N*sizeof(struct stripe_dev *));

    for( int i=0; i<N; i++ ) {
        sdev->disks[i] = disks[i];
    }
    sdev->nbr_disks = N;
    sdev->stripe_size = unit;
    sdev->nblks = (image_size/unit)*unit*N;
    sdev->nbr_failed = 0;

    dev->private = sdev;
    dev->ops = &stripe_ops;

    //printf( "I think we have %d blocks\n", dev->ops->num_blocks(dev) );

    return dev;
}

/**********   RAID 4  ***************/

/* helper function - compute parity function across two blocks of
 * 'len' bytes and put it in a third block. Note that 'dst' can be the
 * same as either 'src1' or 'src2', so to compute parity across N
 * blocks you can do: 
 *
 *     void **block[i] - array of pointers to blocks
 *     dst = <zeros[len]>
 *     for (i = 0; i < N; i++)
 *        parity(block[i], dst, dst);
 *
 * Yes, it could be faster. Don't worry about it.
 */
void parity(int len, void *src1, void *src2, void *dst)
{
    unsigned char *s1 = src1, *s2 = src2, *d = dst;
    int i;
    for (i = 0; i < len; i++)
        d[i] = s1[i] ^ s2[i];
}

/* should only return E_UNAVAIL if the block couldn't be r/w AND it couldn't be proxied */
static int raid4_rw_oper(struct blkdev * dev, int first_blk,
                           int num_blks, void *buf, int is_read)
{
    if( dev == NULL )
        return E_UNAVAIL;
    struct stripe_dev * stripe = (struct stripe_dev*) dev->private;
    if( stripe->nbr_failed )
        return E_UNAVAIL;

    if( num_blks <= 0 || first_blk < 0 || first_blk+num_blks > stripe->nblks )
        return E_SIZE;

    if( get_failed_count( stripe ) >= 2 )
        return E_UNAVAIL;

    if( num_blks > 1 ) {
        for( int i=0; i<num_blks; i++ ) {
            int st = raid4_rw_oper( dev, first_blk+i, 1, buf + BLOCK_SIZE*i, is_read);
            if( st == E_UNAVAIL ) {
                stripe->nbr_failed = 1;
                return E_UNAVAIL;
            }
        }
        
    }
    else {  /* say, 5 disks, 3 sectors/stripe request block 53 -> disk 1, blk 17 (in stripe 5) */
        char old_data[BLOCK_SIZE];
        char parity_data[BLOCK_SIZE];
        int parity_disk = stripe->nbr_disks-1;
        int total_stripe_width = (stripe->nbr_disks-1) * stripe->stripe_size;   /* 4*3   = 12 */
        int stripe_nbr = first_blk / total_stripe_width;                        /* 53/12 = 4  */
        int stripe_offset = first_blk % total_stripe_width;                     /* 53%12 = 5  */
        int which_disk = stripe_offset / stripe->stripe_size;                   /* 5/3   = 1  */
        int stripe_disk_offset = stripe_offset % stripe->stripe_size;           /* 5%3   = 2  */
        int actual_block = stripe_nbr*stripe->stripe_size + stripe_disk_offset; /* 5*3+2 = 17 */

        //printf( " reading disk/blk %d/%d\n", which_disk, actual_block );

        int st;
        int cur_errors = 0;
        if( is_read ) {
            st = stripe->disks[which_disk]->ops->read(stripe->disks[which_disk], actual_block, 1, buf);
            if( st == E_UNAVAIL ) {
                cur_errors = 1;

                /* this is where the fun is - read the rest of the stripe and try to infer this data fail on another error */
                st = stripe->disks[parity_disk]->ops->read(stripe->disks[parity_disk], actual_block, 1, parity_data);
                if( st == E_UNAVAIL )
                    cur_errors++;

                for( int x=0; x<stripe->nbr_disks-1 && cur_errors<2; x++ ) {
                    if( x == which_disk )
                        continue;
                    st = stripe->disks[x]->ops->read(stripe->disks[x], actual_block, 1, old_data);
                    if( st == E_UNAVAIL )
                        cur_errors++;
                    else {
                        parity( BLOCK_SIZE, old_data, parity_data, parity_data );
                    }
                }
                if( cur_errors >= 2) {
                    stripe->nbr_failed = 2;
                    return E_UNAVAIL;
                }
                else{
                    memcpy( buf, parity_data, BLOCK_SIZE );
                }
            }
        }
        else {
       
            int stR = stripe->disks[which_disk]->ops->read(stripe->disks[which_disk], actual_block, 1, old_data);
            int stW = stripe->disks[which_disk]->ops->write(stripe->disks[which_disk], actual_block, 1, buf);
            memset( parity_data, 0, BLOCK_SIZE);
            if( stR == E_UNAVAIL || stW == E_UNAVAIL ) {
                cur_errors = 1;
                /* failed to write the block - read the rest of the stripe and try to infer the new parity - fail on another error */
                for( int x=0; x<stripe->nbr_disks-1 && cur_errors<2; x++ ) {
                    if( x == which_disk ) {
                        parity( BLOCK_SIZE, buf, parity_data, parity_data );
                        continue;
                    }
                    st = stripe->disks[x]->ops->read(stripe->disks[x], actual_block, 1, old_data);
                    if( st == E_UNAVAIL )
                        cur_errors++;
                    else {
                        parity( BLOCK_SIZE, old_data, parity_data, parity_data );
                    }
                }

            } else
            {
                /* calculate the new parity - fail on two errors */
                st = stripe->disks[parity_disk]->ops->read(stripe->disks[parity_disk], actual_block, 1, parity_data);
                if( st == SUCCESS ) {
                    parity( BLOCK_SIZE, old_data, parity_data, parity_data );
                    parity( BLOCK_SIZE, buf, parity_data, parity_data );
                }
            }
            /* write the calculated parity if there hasn't been another error write if possible */
            stripe->disks[parity_disk]->ops->write(stripe->disks[parity_disk], actual_block, 1, parity_data);
        }

    }

    return SUCCESS;
}

/* read blocks from a RAID 4 volume.
 * If the volume is in a degraded state you may need to reconstruct
 * data from the other stripes of the stripe set plus parity.
 * If a drive fails during a read and all other drives are
 * operational, close that drive and continue in degraded state.
 * If a drive fails and the volume is already in a degraded state,
 * close the drive and return an error.
 */
static int raid4_read(struct blkdev * dev, int first_blk,
                      int num_blks, void *buf) 
{
    return raid4_rw_oper( dev, first_blk, num_blks, buf, 1 );
}

/* write blocks to a RAID 4 volume.
 * Note that you must handle short writes - i.e. less than a full
 * stripe set. You may either use the optimized algorithm (for N>3
 * read old data, parity, write new data, new parity) or you can read
 * the entire stripe set, modify it, and re-write it. Your code will
 * be graded on correctness, not speed.
 * If an underlying device fails you should close it and complete the
 * write in the degraded state. If a drive fails in the degraded
 * state, close it and return an error.
 * In the degraded state perform all writes to non-failed drives, and
 * forget about the failed one. (parity will handle it)
 */
static int raid4_write(struct blkdev * dev, int first_blk,
                       int num_blks, void *buf)
{
    return raid4_rw_oper( dev, first_blk, num_blks, buf, 0 );
}

/* clean up, including: close all devices and free any data structures
 * you allocated in raid4_create. 
 */
static void raid4_close(struct blkdev *dev)
{
        /* ### free allocated structures!! */
}

struct blkdev_ops raid_ops = {
    .num_blocks = stripe_num_blocks,
    .read = raid4_read,
    .write = raid4_write,
    .close = raid4_close
};

/* Initialize a RAID 4 volume with strip size 'unit', using
 * disks[N-1] as the parity drive. Do not write to the disks - assume
 * that they are properly initialized with correct parity. (warning -
 * some of the grading scripts may fail if you modify data on the
 * drives in this function)
 */
char set_parity_bfr[BLOCK_SIZE];
char create_blk_bfr[BLOCK_SIZE];
struct blkdev *raid4_create(int N, struct blkdev *disks[], int unit)
{
    struct blkdev *volume = stripe_create( N, disks, unit );
    if( volume != NULL ) {
        struct stripe_dev * raid = (struct stripe_dev*) volume->private;
        raid->nblks = raid->nblks / raid->nbr_disks * (raid->nbr_disks-1);
        volume->ops = &raid_ops;

        // at this point the drives are mounted, but parity could be anything...
        int blks_per_drive = raid->nblks / (raid->nbr_disks-1);
        for( int i=0; i<blks_per_drive; i++) {
            bzero( set_parity_bfr, BLOCK_SIZE);
            for( int d=0; d<raid->nbr_disks-1; d++) {
                raid->disks[d]->ops->read(raid->disks[d], i, 1, create_blk_bfr);
                parity( BLOCK_SIZE, create_blk_bfr, set_parity_bfr, set_parity_bfr);
            }
            raid->disks[raid->nbr_disks-1]->ops->write(raid->disks[raid->nbr_disks-1], i, 1, set_parity_bfr);
        }
    }
    return volume;
}

/* replace failed device 'i' in a RAID 4. Note that we assume
 * the upper layer knows which device failed. You will need to
 * reconstruct content from data and parity before returning
 * from this call.
 */
int raid4_replace(struct blkdev *volume, int i, struct blkdev *newdisk)
{
    if( volume == NULL )
        return E_UNAVAIL;

    struct stripe_dev * raid = (struct stripe_dev*) volume->private;

    int vol_blks = volume->ops->num_blocks(volume);
    int drives = raid->nbr_disks-1;
    int unit = raid->stripe_size;
    int blks = vol_blks/drives;
    int newblks = newdisk->ops->num_blocks(newdisk);

    if( blks > newblks ) {
        printf( "raid_replace: New image must be at least the same size as the image it replaces - %d <> %d\n", blks, newblks );
        return E_SIZE;
    }
    char bfr[BLOCK_SIZE];
    char parity_data[BLOCK_SIZE];
    /* so, populating the drive is a little more involved than for a mirror
        for a data drive you have to calculate which logical blocks belong to this drive
        then you can copy them one at a time to their right physical location.
        for a parity drive, you have to read the rest of the stripe it belongs to,
        calculate the parity, then write it to the proper physical location.
    */

    if( i < drives ) {  // data disk
        for( int x=0; x<blks; x++) {
            int stripe = x / unit;
            int offset = x % unit;
            int lba = stripe*drives*unit + offset;

            volume->ops->read(volume, lba, 1, bfr);
            newdisk->ops->write( newdisk, x, 1, bfr);
        }
    } else {    // parity disk
        for( int x=0; x<blks; x++) {
            bzero( parity_data, BLOCK_SIZE );
            for( int d=0; d<drives; d++ ) {
                raid->disks[d]->ops->read( raid->disks[d], x, 1, bfr );
                parity(BLOCK_SIZE, bfr, parity_data, parity_data );
            }
            newdisk->ops->write( newdisk, x, 1, parity_data);
        }
    }
    raid->disks[i] = newdisk;

    return SUCCESS;
}

