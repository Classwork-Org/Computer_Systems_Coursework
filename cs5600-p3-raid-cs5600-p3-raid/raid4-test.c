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

/* Write a buffer to a file for debugging purposes */
void dump(char* buffer, int length, char* path){
    FILE * output = fopen(path, "w");
    fwrite(buffer, 1, length, output);
    fclose(output);
}
void run_create_tests() {
    printf("Run RAID create tests:\n");

    // a - pass different size drives in - fail expected
    struct blkdev* raid_drives[5];
    raid_drives[0] = create_new_image("raid-create-test-badsize-image-a", 10, 0);
    raid_drives[1] = create_new_image("raid-create-test-badsize-image-b", 8, 0);
    raid_drives[2] = create_new_image("raid-create-test-badsize-image-c", 10, 0);
    raid_drives[3] = create_new_image("raid-create-test-badsize-image-d", 10, 0);
    raid_drives[4] = create_new_image("raid-create-test-badsize-image-e", 10, 0);
    struct blkdev *raid = raid4_create(5, raid_drives, 3);
    if( raid == NULL )
        printf( "raid_create_tests: bad drive test - PASSED\n");
    else
        printf( "raid_create_tests: bad drive test - FAILED\n");

    raid_drives[1] = create_new_image("raid-create-test-badsize-image-b", 10, 0);   // ok - same size
    raid_drives[0] = NULL;
    for( int i=0; i<5; i++ ) {
        raid = raid4_create(5, raid_drives, 3);
        if( raid == NULL )
            printf( "raid_create_tests: NULL drive %c test - PASSED\n", (char)(i+'a'));
        else
            printf( "raid_create_tests: NULL drive %c test - FAILED\n", (char)(i+'a'));
        raid_drives[i] = raid_drives[(i+1)%5];
        raid_drives[(i+1)%5] = NULL;
    }

    // c - pass all null drives in - fail expected
    raid_drives[0] = raid_drives[1] = raid_drives[2] = raid_drives[3] = raid_drives[4] = NULL;    
    raid = raid4_create(5, raid_drives, 3);
    if( raid == NULL )
        printf( "raid_create_tests: all NULL drives test - PASSED\n");
    else
        printf( "raid_create_tests: all NULL drives test - FAILED\n");

    // d - pass same size drives in a:block, b:alpha - pass expected stripe reports correct size
    raid_drives[0] = create_new_image("raid-create-test-block-image-a", 10, 1);
    raid_drives[1] = create_new_image("raid-create-test-alpha-image-b", 10, 2);
    raid_drives[2] = create_new_image("raid-create-test-block-image-c", 10, 1);
    raid_drives[3] = create_new_image("raid-create-test-alpha-image-d", 10, 0);
    raid_drives[4] = create_new_image("raid-create-test-alpha-image-e", 10, 1);
    raid = raid4_create(5, raid_drives, 2);
    if( raid != NULL && raid->ops->num_blocks(raid) == 40)
        printf( "raid_create_tests: good create test - PASSED\n");
    else
        printf( "raid_create_tests: good create test - FAILED\n");

    raid = raid4_create(5, raid_drives, 3);
    if( raid != NULL && raid->ops->num_blocks(raid) == 36)
        printf( "raid_create_tests: good create test some wasted space - PASSED\n");
    else
        printf( "raid_create_tests: good create test some wasted space - FAILED\n");
}

int get_failed_count( struct blkdev *raid );

char image_tag[100];
struct blkdev** cur_test_drives = NULL;
struct blkdev *setup_test_raid( struct blkdev *test_raid, int mode, int disks, int disk_size, int unit ) {
    printf("\nSetup RAID for this mode: ");
   
    struct blkdev *raid = test_raid;
    struct blkdev *act_raid = (raid == NULL ? NULL : raid->private);

    /* we can reuse raid sets, 12 test modes
        mode 0 -> all normal
        mode 1 -> fail a, mode 2 -> repl a
        mode 3 -> fail b, mode 4 -> repl b
        mode 5 -> fail c, mode 6 -> repl c
        mode 7 -> fail d, mode 8 -> repl d
        mode 9 -> fail e, mode 10 -> repl e
        mode 11 -> fail b, d
    */    
    switch( mode ) {
        case 0:
            if( cur_test_drives != NULL )
                free( cur_test_drives );
    
            cur_test_drives = malloc( disks*sizeof(struct blkdev *));
            for( int i=0; i<disks; i++ ) {
                sprintf( image_tag, "raid-create-test-normal-image-%c", (char)(i+'a') );
                cur_test_drives[i] = create_new_image(image_tag, disk_size, 1);
            }
            raid = raid4_create(disks, cur_test_drives, unit);
            printf( "normal operation\n");
            break;
        case 1:
        case 3:
        case 5:
        case 7:
        case 9: {
            int drive_to_fail = (mode-1) / 2;
            image_fail( cur_test_drives[drive_to_fail] );
            cur_test_drives[drive_to_fail] = NULL;
            printf( "failing drive %c\n", (char)(drive_to_fail+'a') );
            int fc = get_failed_count( act_raid );
            if( fc != 1 )
                printf( "expected 1 failed drive, got %d\n", fc );
            }
            break;
        case 2:
        case 4:
        case 6:
        case 8:
        case 10: {
            int drive_to_replace = (mode-2) / 2;
            sprintf( image_tag, "raid-create-test-replace-image-%c", (char)(drive_to_replace+'a') );
            cur_test_drives[ drive_to_replace ] = create_new_image(image_tag, disk_size, 1);
            raid4_replace( raid, drive_to_replace, cur_test_drives[ drive_to_replace ] );
            printf( "replacing drive %c\n", (char)(drive_to_replace+'a') );
            }
            break;
        case 11:
            image_fail( cur_test_drives[1] );
            image_fail( cur_test_drives[3] );
            cur_test_drives[1] = cur_test_drives[3] = NULL;
            printf( "failing drives b and d\n" );
            int fc = get_failed_count( act_raid );
            if( fc != 2 )
                printf( "expected 2 failed drive, got %d\n", fc );
            break;
    }

    return raid;
}

char read_test_buffer[ 60*BLOCK_SIZE ];
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

void run_read_tests( struct blkdev *test_raid, int mode ) {
    printf("Run RAID read tests:\n");

    /* try invalid tests first */
    if( 
            test_raid->ops->read(test_raid, -1, 1, read_test_buffer) == SUCCESS ||
            test_raid->ops->read(test_raid, 0, 0, read_test_buffer) == SUCCESS ||
            test_raid->ops->read(test_raid, 2, 0, read_test_buffer) == SUCCESS ||
            test_raid->ops->read(test_raid, 2, -2, read_test_buffer) == SUCCESS ||
            test_raid->ops->read(test_raid, 0, 150, read_test_buffer) == SUCCESS ||
            test_raid->ops->read(test_raid, 2, 150, read_test_buffer) == SUCCESS )
        printf( "raid_read_tests: invalid index or length test - FAILED\n");
    else        
        printf( "raid_read_tests: invalid index or length test - PASSED\n");
    
    int total_fails = 0;

    if( !(test_raid->ops->read(test_raid, 0, 1, read_test_buffer) == SUCCESS && validate_buffer( read_test_buffer, 0, 1)) ) {
        if( mode != 11 ) {
            total_fails++;
            printf( "raid_read_tests: read 1 block from base 0 - FAILED\n");
        }
    } else 
        if( mode == 11 ) {
            total_fails++;
            printf( "raid_read_tests: read 1 block from base 0 did not fail... - FAILED\n");
        }

    if( !(test_raid->ops->read(test_raid, 4, 1, read_test_buffer) == SUCCESS && validate_buffer( read_test_buffer, 4, 1)) ) {
        if( mode != 11 ) {
            total_fails++;
            printf( "raid_read_tests: read 4 block from base 1 - FAILED\n");
        }
    } else       
        if( mode == 11 ) {
            total_fails++;
            printf( "raid_read_tests: read 4 block from base 1 did not fail... - FAILED\n");
        }

    if( !(test_raid->ops->read(test_raid, 10, 4, read_test_buffer) == SUCCESS && validate_buffer( read_test_buffer, 10, 4)) ) {
        if( mode != 11 ) {
            total_fails++;
            printf( "raid_read_tests: read 4 block from base 10 - FAILED\n");
        }
    } else       
        if( mode == 11 ) {
            total_fails++;
            printf( "raid_read_tests: read 4 block from base 10 did not fail... - FAILED\n");
        }

    if( !(test_raid->ops->read(test_raid, 18, 10, read_test_buffer) == SUCCESS && validate_buffer( read_test_buffer, 18, 10)) ) {
        if( mode != 11 ) {
            total_fails++;
            printf( "raid_read_tests: read 10 block from base 18 - FAILED\n");
        }
    } else
        if( mode == 11 ) {
            total_fails++;
            printf( "raid_read_tests: read 10 block from base 18 did not fail... - FAILED\n");
        }

    if( total_fails == 0 )
        printf( "raid_read_tests: actual read tests - PASSED\n");
    else
        printf( "raid_read_tests: actual read tests - FAILED\n");


}
void run_write_tests( struct blkdev *raid, int mode ) {
    printf("Run RAID write tests:\n");
        /* try invalid tests first */
    if( 
            raid->ops->write(raid, -1, 1, read_test_buffer) == SUCCESS ||
            raid->ops->write(raid, 0, 0, read_test_buffer) == SUCCESS ||
            raid->ops->write(raid, 2, 0, read_test_buffer) == SUCCESS ||
            raid->ops->write(raid, 2, -2, read_test_buffer) == SUCCESS ||
            raid->ops->write(raid, 0, 150, read_test_buffer) == SUCCESS ||
            raid->ops->write(raid, 2, 150, read_test_buffer) == SUCCESS )
        printf( "raid_write_tests: invalid index or length test - FAILED\n");
    else        
        printf( "raid_write_tests: invalid index or length test - PASSED\n");
    int total_fails = 0;

    write_data( write_test_buffer, BLOCK_SIZE, 77 );
    if( !(raid->ops->write(raid, 0, 1, write_test_buffer) == SUCCESS &&
            raid->ops->read(raid, 0, 36, read_test_buffer) == SUCCESS &&
            validate_full_buffer( read_test_buffer, (int[])
                { 77,1,2, 0,1,2, 0,1,2, 0,1,2, 3,4,5, 3,4,5, 3,4,5, 3,4,5, 6,7,8, 6,7,8, 6,7,8, 6,7,8, 9,9,9,9  } ) ) ) {
        if( mode != 11 ) {
            total_fails++;
            printf( "raid_write_tests: write 1 block from base 0 - FAILED\n");
        }
    } else if( mode == 11 ) {
        total_fails++;
        printf( "raid_write_tests: write 1 block from base 0 did not fail... - FAILED\n");
    }

    write_data( write_test_buffer, BLOCK_SIZE, 88 );
    if( !(raid->ops->write(raid, 1, 1, write_test_buffer) == SUCCESS &&
            raid->ops->read(raid, 0, 36, read_test_buffer) == SUCCESS &&
            validate_full_buffer( read_test_buffer, (int[])
                { 77,88,2, 0,1,2, 0,1,2, 0,1,2, 3,4,5, 3,4,5, 3,4,5, 3,4,5, 6,7,8, 6,7,8, 6,7,8, 6,7,8, 9,9,9,9  } ) ) ) {
        if( mode != 11 ) {
            total_fails++;
            printf( "raid_write_tests: write 1 block from base 1 - FAILED\n");
        }
    } else if( mode == 11 ) {
        total_fails++;
        printf( "raid_write_tests: write 1 block from base 1 did not fail... - FAILED\n");
    }

    write_data( write_test_buffer, BLOCK_SIZE, 44 );
    write_data( write_test_buffer+BLOCK_SIZE, BLOCK_SIZE, 55 );
    write_data( write_test_buffer+2*BLOCK_SIZE, BLOCK_SIZE, 66 );
    write_data( write_test_buffer+3*BLOCK_SIZE, BLOCK_SIZE, 77 );
    if( !(raid->ops->write(raid, 0, 4, write_test_buffer) == SUCCESS &&
            raid->ops->read(raid, 0, 36, read_test_buffer) == SUCCESS &&
            validate_full_buffer( read_test_buffer, (int[])
                { 44,55,66, 77,1,2, 0,1,2, 0,1,2, 3,4,5, 3,4,5, 3,4,5, 3,4,5, 6,7,8, 6,7,8, 6,7,8, 6,7,8, 9,9,9,9  } ) ) ) {
        if( mode != 11 ) {
            total_fails++;
            printf( "raid_write_tests: write 4 block from base 0 - FAILED\n");
        }
    } else if( mode == 11 ) {
        total_fails++;
        printf( "raid_write_tests: write 4 block from base 0 did not fail... - FAILED\n");
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
    if( !(raid->ops->write(raid, 10, 10, write_test_buffer) == SUCCESS &&
            raid->ops->read(raid, 0, 36, read_test_buffer) == SUCCESS &&
            validate_full_buffer( read_test_buffer, (int[])
                { 44,55,66, 77,1,2, 0,1,2, 0,23,34, 45,56,67, 78,88,99, 28,82,5, 3,4,5, 6,7,8, 6,7,8, 6,7,8, 6,7,8, 9,9,9,9  } ) ) ) {
        if( mode != 11 ) {
            total_fails++;
            printf( "raid_write_tests: write 10 block from base 10 - FAILED\n");
        }
    } else if( mode == 11 ) {
        total_fails++;
        printf( "raid_write_tests: write 10 block from base 10 did not fail... - FAILED\n");
    }

    // reset data for next test
    for( int b=0; b<36; b++ ) {
        int stripe = b/12;
        int st_off = b%3;
        int disk_block = 3*stripe + st_off;
        write_data( write_test_buffer, BLOCK_SIZE, disk_block );
        raid->ops->write(raid, b, 1, write_test_buffer);
    }

    if( total_fails == 0 )
        printf( "stripe_write_tests: actual write tests - PASSED\n");
    else
        printf( "stripe_write_tests: actual write tests - FAILED\n");


}
void run_close_tests( struct blkdev *test_raid, int mode ) {
    printf("Run RAID close tests:\n");
}

int main(){
    printf( "\nFunctional tests:\n\n");


    /* TODO: Your tests here */
    run_create_tests();
    struct blkdev *test_raid = NULL;
    for( int mode=0; mode<12; mode++ ) {
        test_raid = setup_test_raid( test_raid, mode, 5, 10, 3 );
        run_read_tests( test_raid, mode );
        run_write_tests( test_raid, mode );
        run_close_tests( test_raid, mode );
    }
}
