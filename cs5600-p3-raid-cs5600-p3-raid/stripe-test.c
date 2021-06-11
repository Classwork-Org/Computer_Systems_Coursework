#include "blkdev.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Write some data to an area of memory */
void write_data(char* data, int length, int datatype){
    for (int i = 0; i < length; i++){
        data[i] = (char) (datatype);
    }
}

/* Create a new file ready to be used as an image. Every byte of the file will be zero. */
struct blkdev *  create_new_image(char * path, int blocks, int data_to_init){
    if (blocks < 1){
        printf("create_new_image: error - blocks must be at least 1: %d\n", blocks);
        return NULL;
    }
    FILE * image = fopen(path, "w");
    if( image == NULL ) {
        printf( "File <%s> failed to open!!\n", path);
    }
    char c = '\0';
    switch( data_to_init ) {
        case 0: /* init to 0 */
            /* This is a trick: instead of writing every byte from 0 to N we can instead move the file cursor
             * directly to N-1 and then write 1 byte. The filesystem will fill in the rest of the bytes with
             * zero for us.
             */
            fseek(image, blocks * BLOCK_SIZE - 1, SEEK_SET);
            char c = 0;
            fwrite(&c, 1, 1, image);
            break;
        case 1: /* init to block number */
            for( int b=0; b<blocks; b++ ) {
                for(int pos=0; pos<BLOCK_SIZE; pos++) {
                    c = (char)b;
                    fwrite(&c, sizeof(char), 1, image);
                }
            }
            break;
        case 2: /* init to alpha data  */
            for( int b=0; b<blocks; b++ ) {

                for(int pos=0; pos<BLOCK_SIZE; pos++) {
                    c = (char) ((b * BLOCK_SIZE + pos) % 26 + 'a');

                    fwrite(&c, sizeof(char), 1, image);
                }
            }
            break;
    }
    fflush(image);
    fclose(image);

    return image_create(path);
}

char image_tag[100];
struct blkdev** cur_test_drives = NULL;
struct blkdev *setup_test_stripe( struct blkdev *ref_stripe, int mode, int disks, int disk_size, int unit ) {
    printf( "\n\nsetup stripe for tests: " );
    
    struct blkdev *stripe = ref_stripe;

    if( ref_stripe != NULL )
        ref_stripe->ops->close( ref_stripe );

    /* we can't reuse stripe sets, like we could with mirrors, once it's dead it's dead, 
       so we will create a new drive set for every pass, and fail the one needed based on
       the given mode... */

    if( cur_test_drives != NULL )
        free( cur_test_drives );
    
    cur_test_drives = malloc( disks*sizeof(struct blkdev *));
    for( int i=0; i<disks; i++ ) {
            sprintf( image_tag, "stripe-create-test-normal-image-%c", (char)(i+'a') );
            cur_test_drives[i] = create_new_image(image_tag, disk_size, 1);
        }
    stripe = stripe_create(disks, cur_test_drives, unit);

    if(mode > 0) {
        printf( "failing a drive, in sequence\n");
        image_fail(cur_test_drives[mode-1]);
    }
    else
        printf( "normal operation\n");

    return stripe;
}


void run_create_tests() {
    printf( "stripe_create_tests: \n\n");
    // a - pass different size drives in - fail expected
    struct blkdev* stripe_drives[3];
    stripe_drives[0] = create_new_image("stripe-create-test-badsize-image-a", 10, 0);
    stripe_drives[1] = create_new_image("stripe-create-test-badsize-image-b", 8, 0);
    stripe_drives[2] = create_new_image("stripe-create-test-badsize-image-c", 10, 0);
    struct blkdev *stripe = stripe_create(3, stripe_drives, 2);
    if( stripe == NULL )
        printf( "stripe_create_tests: bad drive test - PASSED\n");
    else
        printf( "stripe_create_tests: bad drive test - FAILED\n");

    stripe_drives[0] = NULL;
    stripe = stripe_create(3, stripe_drives, 2);
    if( stripe == NULL )
        printf( "stripe_create_tests: NULL drive a test - PASSED\n");
    else
        printf( "stripe_create_tests: NULL drive a test - FAILED\n");
    
    // b - pass one null drives in - fail expected
    stripe_drives[0] = stripe_drives[1];
    stripe_drives[1] = NULL;
    stripe = stripe_create(3, stripe_drives, 2);
    if( stripe == NULL )
        printf( "stripe_create_tests: NULL drive b test - PASSED\n");
    else
        printf( "stripe_create_tests: NULL drive b test - FAILED\n");

    // c - pass all null drives in - fail expected
    stripe_drives[0] = stripe_drives[1] = NULL;    
    stripe = stripe_create(3, stripe_drives, 2);
    if( stripe == NULL )
        printf( "stripe_create_tests: all NULL drives test - PASSED\n");
    else
        printf( "stripe_create_tests: all NULL drives test - FAILED\n");

    // d - pass same size drives in a:block, b:alpha - pass expected stripe reports correct size
    stripe_drives[0] = create_new_image("stripe-create-test-block-image-a", 10, 1);
    stripe_drives[1] = create_new_image("stripe-create-test-alpha-image-b", 10, 2);
    stripe_drives[2] = create_new_image("stripe-create-test-block-image-c", 10, 1);
    stripe = stripe_create(3, stripe_drives, 2);
    if( stripe != NULL && stripe->ops->num_blocks(stripe) == 30)
        printf( "stripe_create_tests: good create test - PASSED\n");
    else
        printf( "stripe_create_tests: good create test - FAILED\n");

    stripe = stripe_create(3, stripe_drives, 3);
    if( stripe != NULL && stripe->ops->num_blocks(stripe) == 27)
        printf( "stripe_create_tests: good create test some wasted space - PASSED\n");
    else
        printf( "stripe_create_tests: good create test some wasted space - FAILED\n");

    // ### if there's time, rewrite the volume integrity check tests
}

char read_test_buffer[ 50*BLOCK_SIZE ];
char write_test_buffer[ 15*BLOCK_SIZE ];
int validate_buffer( char *b, int start_blk, int nbr_blk) {
    /* should have these as params... */
    int disks = 4;
    int unit = 3;

    for( int i=0; i<nbr_blk*BLOCK_SIZE; i++ ) {

        int offset_in_blk = i % BLOCK_SIZE;
        int this_blk = start_blk + i/BLOCK_SIZE;
        int str_width = disks*unit;
        int act_stripe = this_blk / str_width;
        int offset = this_blk % unit;
        int this_disk = (nbr_blk % str_width) / unit;
        int disk_block = act_stripe*unit + offset;

        if( b[i] != disk_block ) {
            printf( " item logical blk %d/%d => phys %d/%d/%d, %d <> %d \n", 
                this_blk, offset_in_blk,
                this_disk, disk_block, offset_in_blk,
                b[i], disk_block);
            return 0;
        }

    }
    return 1;
}
int validate_full_buffer( char *b, int *bvals) {
    for( int bk=0; bk<36; bk++ )
        for( int i=0; i<BLOCK_SIZE; i++ ) {
            if( b[bk*BLOCK_SIZE+i] != bvals[ bk ] ) {
                printf( " item %d/%d %d <> %d \n", bk,i, (int)b[bk*BLOCK_SIZE+i], bvals[bk]);

                return 0;
            }
        }
    return 1;
}

void run_read_tests( struct blkdev *stripe, int mode ) {
    printf( "stripe_read_tests: \n");

    /* try invalid tests first */
    if( 
            stripe->ops->read(stripe, -1, 1, read_test_buffer) == SUCCESS ||
            stripe->ops->read(stripe, 0, 0, read_test_buffer) == SUCCESS ||
            stripe->ops->read(stripe, 2, 0, read_test_buffer) == SUCCESS ||
            stripe->ops->read(stripe, 2, -2, read_test_buffer) == SUCCESS ||
            stripe->ops->read(stripe, 0, 150, read_test_buffer) == SUCCESS ||
            stripe->ops->read(stripe, 2, 150, read_test_buffer) == SUCCESS )
        printf( "stripe_read_tests: invalid index or length test - FAILED\n");
    else        
        printf( "stripe_read_tests: invalid index or length test - PASSED\n");
    int total_fails = 0;

    if( !(stripe->ops->read(stripe, 0, 1, read_test_buffer) == SUCCESS && validate_buffer( read_test_buffer, 0, 1)) ) {
        if( mode == 0 ) {
            total_fails++;
            printf( "stripe_read_tests: read 1 block from base 0 - FAILED\n");
        }
    } else 
        if( mode != 0 ) {
            total_fails++;
            printf( "stripe_read_tests: read 1 block from base 0 did not fail... - FAILED\n");
        }

    if( !(stripe->ops->read(stripe, 1, 1, read_test_buffer) == SUCCESS && validate_buffer( read_test_buffer, 1, 1)) ) {
        if( mode == 0 ) {
            total_fails++;
            printf( "stripe_read_tests: read 1 block from base 1 - FAILED\n");
        }
    } else       
        if( mode != 0 ) {
            total_fails++;
            printf( "stripe_read_tests: read 1 block from base 1 did not fail... - FAILED\n");
        }


    if( !(stripe->ops->read(stripe, 0, 4, read_test_buffer) == SUCCESS && validate_buffer( read_test_buffer, 0, 4)) ) {
        if( mode == 0 ) {
            total_fails++;
            printf( "stripe_read_tests: read 4 block from base 0 - FAILED\n");
        }
    } else       
        if( mode != 0 ) {
            total_fails++;
            printf( "stripe_read_tests: read 4 block from base 0 did not fail... - FAILED\n");
        }


    if( !(stripe->ops->read(stripe, 18, 10, read_test_buffer) == SUCCESS && validate_buffer( read_test_buffer, 18, 10)) ) {
        if( mode == 0 ) {
            total_fails++;
            printf( "stripe_read_tests: read 10 block from base 18 - FAILED\n");
        }
    } else
        if( mode != 0 ) {
            total_fails++;
            printf( "stripe_read_tests: read 10 block from base 18 did not fail... - FAILED\n");
        }

    if( total_fails == 0 )
        printf( "stripe_read_tests: actual read tests - PASSED\n");
    else
        printf( "stripe_read_tests: actual read tests - FAILED\n");

}
struct stripe_dev {
    struct blkdev **disks;    /* flag bad disk by setting to NULL */
    int nbr_disks;
    int stripe_size;
    int nbr_failed;
    int nblks;
};


void run_write_tests( struct blkdev *stripe, int mode ) {
    printf( "stripe_write_tests:\n");
        /* try invalid tests first */
    if( 
            stripe->ops->write(stripe, -1, 1, read_test_buffer) == SUCCESS ||
            stripe->ops->write(stripe, 0, 0, read_test_buffer) == SUCCESS ||
            stripe->ops->write(stripe, 2, 0, read_test_buffer) == SUCCESS ||
            stripe->ops->write(stripe, 2, -2, read_test_buffer) == SUCCESS ||
            stripe->ops->write(stripe, 0, 150, read_test_buffer) == SUCCESS ||
            stripe->ops->write(stripe, 2, 150, read_test_buffer) == SUCCESS )
        printf( "stripe_write_tests: invalid index or length test - FAILED\n");
    else        
        printf( "stripe_write_tests: invalid index or length test - PASSED\n");
    int total_fails = 0;

    write_data( write_test_buffer, BLOCK_SIZE, 77 );
    if( !(stripe->ops->write(stripe, 0, 1, write_test_buffer) == SUCCESS &&
            stripe->ops->read(stripe, 0, 36, read_test_buffer) == SUCCESS &&
            validate_full_buffer( read_test_buffer, (int[])
                { 77,1,2, 0,1,2, 0,1,2, 0,1,2, 3,4,5, 3,4,5, 3,4,5, 3,4,5, 6,7,8, 6,7,8, 6,7,8, 6,7,8, 9,9,9,9  } ) ) ) {
        if( mode == 0 ) {
            total_fails++;
            printf( "stripe_write_tests: write 1 block from base 0 - FAILED\n");
        }
    } else if( mode != 0 ) {
        total_fails++;
        printf( "stripe_write_tests: write 1 block from base 0 did not fail... - FAILED\n");
    }

    write_data( write_test_buffer, BLOCK_SIZE, 88 );
    if( !(stripe->ops->write(stripe, 1, 1, write_test_buffer) == SUCCESS &&
            stripe->ops->read(stripe, 0, 36, read_test_buffer) == SUCCESS &&
            validate_full_buffer( read_test_buffer, (int[])
                { 77,88,2, 0,1,2, 0,1,2, 0,1,2, 3,4,5, 3,4,5, 3,4,5, 3,4,5, 6,7,8, 6,7,8, 6,7,8, 6,7,8, 9,9,9,9  } ) ) ) {
        if( mode == 0 ) {
            total_fails++;
            printf( "stripe_write_tests: write 1 block from base 1 - FAILED\n");
        }
    } else if( mode != 0 ) {
        total_fails++;
        printf( "stripe_write_tests: write 1 block from base 1 did not fail... - FAILED\n");
    }

    write_data( write_test_buffer, BLOCK_SIZE, 44 );
    write_data( write_test_buffer+BLOCK_SIZE, BLOCK_SIZE, 55 );
    write_data( write_test_buffer+2*BLOCK_SIZE, BLOCK_SIZE, 66 );
    write_data( write_test_buffer+3*BLOCK_SIZE, BLOCK_SIZE, 77 );
    if( !(stripe->ops->write(stripe, 0, 4, write_test_buffer) == SUCCESS &&
            stripe->ops->read(stripe, 0, 36, read_test_buffer) == SUCCESS &&
            validate_full_buffer( read_test_buffer, (int[])
                { 44,55,66, 77,1,2, 0,1,2, 0,1,2, 3,4,5, 3,4,5, 3,4,5, 3,4,5, 6,7,8, 6,7,8, 6,7,8, 6,7,8, 9,9,9,9  } ) ) ) {
        if( mode == 0 ) {
            total_fails++;
            printf( "stripe_write_tests: write 4 block from base 0 - FAILED\n");
        }
    } else if( mode != 0 ) {
        total_fails++;
        printf( "stripe_write_tests: write 4 block from base 0 did not fail... - FAILED\n");
    }
    
    write_data( write_test_buffer, BLOCK_SIZE, 23 );
    write_data( write_test_buffer+BLOCK_SIZE, BLOCK_SIZE, 34 );
    write_data( write_test_buffer+2*BLOCK_SIZE, BLOCK_SIZE, 45 );
    write_data( write_test_buffer+3*BLOCK_SIZE, BLOCK_SIZE, 56 );
    write_data( write_test_buffer+4*BLOCK_SIZE, BLOCK_SIZE, 67 );
    write_data( write_test_buffer+5*BLOCK_SIZE, BLOCK_SIZE, 78 );
    write_data( write_test_buffer+6*BLOCK_SIZE, BLOCK_SIZE, 88 );
    write_data( write_test_buffer+7*BLOCK_SIZE, BLOCK_SIZE, 99 );
    write_data( write_test_buffer+8*BLOCK_SIZE, BLOCK_SIZE, 28 );
    write_data( write_test_buffer+9*BLOCK_SIZE, BLOCK_SIZE, 82 );
    if( !(stripe->ops->write(stripe, 10, 10, write_test_buffer) == SUCCESS &&
            stripe->ops->read(stripe, 0, 36, read_test_buffer) == SUCCESS &&
            validate_full_buffer( read_test_buffer, (int[])
                { 44,55,66, 77,1,2, 0,1,2, 0,23,34, 45,56,67, 78,88,99, 28,82,5, 3,4,5, 6,7,8, 6,7,8, 6,7,8, 6,7,8, 9,9,9,9  } ) ) ) {
        if( mode == 0 ) {
            total_fails++;
            printf( "stripe_write_tests: write 10 block from base 10 - FAILED\n");
        }
    } else if( mode != 0 ) {
        total_fails++;
        printf( "stripe_write_tests: write 10 block from base 10 did not fail... - FAILED\n");
    }

    // reset data for next test
    for( int i=0; i<4; i++ ) {
    for( int b=0; b<10; b++ ) {
        struct stripe_dev *act_stripe = stripe->private;
        write_data( write_test_buffer, BLOCK_SIZE, b );
        act_stripe->disks[i]->ops->write(act_stripe->disks[i], b, 1, write_test_buffer);
    }
    }

    if( total_fails == 0 )
        printf( "stripe_write_tests: actual write tests - PASSED\n");
    else
        printf( "stripe_write_tests: actual write tests - FAILED\n");

}


void run_close_tests( struct blkdev *stripe, int mode ) {
    printf( "stripe_close_tests: ");
    stripe->ops->close( stripe );
    // if we're here, we passed
    printf( " - PASSED \n");
}

int main(){
    printf( "\n\nStripe set functional tests:\n\n");

    int stripe_set_nbr_drives = 4;
    int disk_size = 10;
    int stripe_unit = 3;

    run_create_tests();
    struct blkdev *test_stripe = NULL;
    /* 6 modes for mirror were normal, deg a, repl a, deg b, repl b, deg both
       there is no redundancy/replacement, so the 5 modes for a 4 disk stripe set are:
       normal, deg a, deg b, deg c, deg d */
    for( int mode=0; mode<stripe_set_nbr_drives+1; mode++ ) {
        test_stripe = setup_test_stripe( test_stripe, mode, stripe_set_nbr_drives, disk_size, stripe_unit );
        run_read_tests( test_stripe, mode );
        run_write_tests( test_stripe, mode );
        run_close_tests( test_stripe, mode );
        // since it's closed, we should set it to NULL
        test_stripe = NULL;
    }
}
