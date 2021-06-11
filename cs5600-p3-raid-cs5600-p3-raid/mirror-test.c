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
            for( int b=0; b<blocks; b++ )
                for(int pos=0; pos<BLOCK_SIZE; pos++) {
                    fseek(image, b * BLOCK_SIZE +pos, SEEK_SET);
                    char c = (char)b;
                    fwrite(&c, 1, 1, image);
                }
            break;
        case 2: /* init to alpha data  */
            for( int b=0; b<blocks; b++ )
                for(int pos=0; pos<BLOCK_SIZE; pos++) {
                    fseek(image, b * BLOCK_SIZE + pos, SEEK_SET);
                    char c = (char) ((b * BLOCK_SIZE + pos) % 26 + 'a');
                    fwrite(&c, 1, 1, image);
                }
            break;
    }
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
    printf( "mirror_create_tests: \n\n");
    // a - pass different size drives in - fail expected
    struct blkdev* mirror_drives[2];
    mirror_drives[0] = create_new_image("mirror-create-test-badsize-image-a", 2, 0);
    mirror_drives[1] = create_new_image("mirror-create-test-badsize-image-b", 4, 0);
    struct blkdev *mirror = mirror_create(mirror_drives);
    if( mirror == NULL )
        printf( "mirror_create_tests: bad drive test - PASSED\n");
    else
        printf( "mirror_create_tests: bad drive test - FAILED\n");

    mirror_drives[0] = NULL;
    mirror = mirror_create(mirror_drives);
    if( mirror == NULL )
        printf( "mirror_create_tests: NULL drive a test - PASSED\n");
    else
        printf( "mirror_create_tests: NULL drive a test - FAILED\n");
    
    // b - pass one null drives in - fail expected
    mirror_drives[0] = mirror_drives[1];
    mirror_drives[1] = NULL;
    mirror = mirror_create(mirror_drives);
    if( mirror == NULL )
        printf( "mirror_create_tests: NULL drive b test - PASSED\n");
    else
        printf( "mirror_create_tests: NULL drive b test - FAILED\n");

    // c - pass both null drives in - fail expected
    mirror_drives[0] = mirror_drives[1] = NULL;    
    mirror = mirror_create(mirror_drives);
    if( mirror == NULL )
        printf( "mirror_create_tests: both NULL drives test - PASSED\n");
    else
        printf( "mirror_create_tests: both NULL drives test - FAILED\n");

    // d - pass same size drives in a:block, b:alpha - pass expected mirror reports correct size
    mirror_drives[0] = create_new_image("mirror-create-test-block-image-a", 5, 1);
    mirror_drives[1] = create_new_image("mirror-create-test-alpha-image-b", 5, 2);
    mirror = mirror_create(mirror_drives);

    struct mirror_dev *dev = (struct mirror_dev *)(mirror->private);

    if( mirror != NULL && mirror->ops->num_blocks(mirror) == 5)
        printf( "mirror_create_tests: good create test - PASSED\n");
    else
        printf( "mirror_create_tests: good create test - FAILED\n");


    char read_buffer[BLOCK_SIZE];
    int status = E_UNAVAIL;


    // following tests to show that image contents are not affected on create

    bzero(read_buffer, BLOCK_SIZE);
    if (mirror != NULL && (status=mirror->ops->read(mirror, 2, 1, read_buffer)) == SUCCESS) {
        for( int i=0; i<BLOCK_SIZE; i++ )
            if( read_buffer[i] != 2 ) {
                mirror = NULL;
                break;
            }
    }
    if( mirror == NULL || status != SUCCESS)
        printf( "mirror_create_tests: readback of 'block' data - FAILED\n");

    image_fail( mirror_drives[0] );
    bzero(read_buffer, BLOCK_SIZE);
    if (mirror != NULL && (status=mirror->ops->read(mirror, 2, 1, read_buffer)) == SUCCESS) {
        for( int i=0; i<BLOCK_SIZE; i++ )
            if( !( 'a' <= read_buffer[i] && read_buffer[i] <= 'z') ) {
                mirror = NULL;
                break;
            }
    }
    if( mirror == NULL || status != SUCCESS)
        printf( "mirror_create_tests: readback of 'alpha' data - FAILED\n");

    struct blkdev *repl_drive = create_new_image("mirror-create-test-block-image-a-repl", 5, 1);
    mirror_replace( mirror, 0, repl_drive );
    bzero(read_buffer, BLOCK_SIZE);
    if (mirror != NULL && (status=mirror->ops->read(mirror, 2, 1, read_buffer)) == SUCCESS) {
        for( int i=0; i<BLOCK_SIZE; i++ )
            if( !( 'a' <= read_buffer[i] && read_buffer[i] <= 'z') ) {
                mirror = NULL;
                break;
            }
    }
    if( mirror == NULL || status != SUCCESS)
        printf( "mirror_create_tests: readback of 'alpha' data after replace - FAILED\n");
    else
        printf( "mirror_create_tests: test inital image contents - PASSED\n");
}

struct blkdev* cur_test_drives[2];
struct blkdev *setup_test_mirror( struct blkdev *ref_mirror, int mode ) {
    printf( "\n\nsetup mirror for tests: ");
    struct blkdev * mirror = ref_mirror;

    switch( mode ) {
        case 0: /* normal mode */
            printf( "'Normal' mode -\n\n");
            struct blkdev* mirror_drives[2];
            cur_test_drives[0] = create_new_image("mirror-create-test-normal-image-a", 5, 1);
            cur_test_drives[1] = create_new_image("mirror-create-test-normal-image-b", 5, 1);
            mirror = mirror_create(cur_test_drives);
            break;
        case 1: /* degrade drive a */
            printf( "'degrade drive a' mode -\n\n");
            image_fail(cur_test_drives[0]);
            break;
        case 2: /* replace drive a */
            printf( "'replace drive a' mode -\n\n");
            cur_test_drives[0] = create_new_image("mirror-create-test-repl-image-a", 5, 0);
            mirror_replace( mirror, 0, cur_test_drives[0] );
            break;
        case 3: /* degrade drive b */
            printf( "'degrade drive b' mode -\n\n");
            image_fail(cur_test_drives[1]);
            break;
        case 4: /* replace drive b */
            printf( "'replace drive b' mode -\n\n");
            cur_test_drives[1] = create_new_image("mirror-create-test-repl-image-b", 5, 0);
            mirror_replace( mirror, 1, cur_test_drives[1] );
            break;
        case 5: /* both drives fail */
            printf( "'degrade both drives' mode -\n\n");
            image_fail(cur_test_drives[0]);
            image_fail(cur_test_drives[1]);
            break;
    }
    return mirror;
}


char read_test_buffer[ 6*BLOCK_SIZE ];
char write_test_buffer[ 6*BLOCK_SIZE ];
int validate_buffer( char *b, int start_blk, int nbr_blk) {
    for( int i=0; i<nbr_blk*BLOCK_SIZE; i++ ) {
        if( b[i] != (i/BLOCK_SIZE)+start_blk)
            return 0;
    }
    return 1;
}
int validate_full_buffer( char *b, int *bvals) {
    for( int bk=0; bk<5; bk++ )
        for( int i=0; i<BLOCK_SIZE; i++ ) {
            if( b[bk*BLOCK_SIZE+i] != bvals[ bk ] ) {
                printf( " item %d/%d %d <> %d \n", bk,i, (int)b[bk*BLOCK_SIZE+i], bvals[bk]);

                return 0;
            }
        }
    return 1;
}

void run_read_tests( struct blkdev *mirror, int mode ) {
    printf( "mirror_read_tests: \n");

    /* try invalid tests first */
    if( 
            mirror->ops->read(mirror, -1, 1, read_test_buffer) == SUCCESS ||
            mirror->ops->read(mirror, 0, 0, read_test_buffer) == SUCCESS ||
            mirror->ops->read(mirror, 2, 0, read_test_buffer) == SUCCESS ||
            mirror->ops->read(mirror, 2, -2, read_test_buffer) == SUCCESS ||
            mirror->ops->read(mirror, 0, 6, read_test_buffer) == SUCCESS ||
            mirror->ops->read(mirror, 2, 4, read_test_buffer) == SUCCESS )
        printf( "mirror_read_tests: invalid index or length test - FAILED\n");
    else        
        printf( "mirror_read_tests: invalid index or length test - PASSED\n");

    int total_fails = 0;

    if( !(mirror->ops->read(mirror, 0, 1, read_test_buffer) == SUCCESS && validate_buffer( read_test_buffer, 0, 1)) ) {
        total_fails++;
        if( mode != 5 )
            printf( "mirror_read_tests: read 1 block from base 0 - FAILED\n");
    }
    if( !(mirror->ops->read(mirror, 1, 1, read_test_buffer) == SUCCESS && validate_buffer( read_test_buffer, 1, 1)) ) {
        total_fails++;
        if( mode != 5 )
            printf( "mirror_read_tests: read 1 block from base 1 - FAILED\n");
    }
    if( !(mirror->ops->read(mirror, 0, 3, read_test_buffer) == SUCCESS && validate_buffer( read_test_buffer, 0, 3)) ) {
        total_fails++;
        if( mode != 5 )
            printf( "mirror_read_tests: read 3 block from base 0 - FAILED\n");
    }
    if( !(mirror->ops->read(mirror, 2, 3, read_test_buffer) == SUCCESS && validate_buffer( read_test_buffer, 2, 3)) ) {
        total_fails++;
        if( mode != 5 )
            printf( "mirror_read_tests: read 3 block from base 2 - FAILED\n");
    }

    if( (total_fails == 0 && mode != 5) || (total_fails == 4 && mode == 5) )
        printf( "mirror_read_tests: actual read tests - PASSED\n");
    else
        printf( "mirror_read_tests: actual read tests - FAILED\n");

}
void run_write_tests( struct blkdev *mirror, int mode ) {
    printf( "mirror_write_tests: \n");
    bzero(write_test_buffer, 6*BLOCK_SIZE);

    /* try invalid tests first */
    if( 
            mirror->ops->write(mirror, -1, 1, write_test_buffer) == SUCCESS ||
            mirror->ops->write(mirror, 0, 0, write_test_buffer) == SUCCESS ||
            mirror->ops->write(mirror, 2, 0, write_test_buffer) == SUCCESS ||
            mirror->ops->write(mirror, 2, -2, write_test_buffer) == SUCCESS ||
            mirror->ops->write(mirror, 0, 6, write_test_buffer) == SUCCESS ||
            mirror->ops->write(mirror, 2, 4, write_test_buffer) == SUCCESS )
        printf( "mirror_write_tests: invalid index or length test - FAILED\n");
    else        
        printf( "mirror_write_tests: invalid index or length test - PASSED\n");

    int total_fails = 0;

    write_data( write_test_buffer, BLOCK_SIZE, 77 );
    if( !(mirror->ops->write(mirror, 0, 1, write_test_buffer) == SUCCESS &&
            mirror->ops->read(mirror, 0, 5, read_test_buffer) == SUCCESS &&
            validate_full_buffer( read_test_buffer, (int[]){ 77, 1, 2, 3, 4 } ) ) ) {
        total_fails++;
        if( mode != 5 )
            printf( "mirror_write_tests: write 1 block from base 0 - FAILED\n");
    }

    write_data( write_test_buffer, BLOCK_SIZE, 88 );
    if( !(mirror->ops->write(mirror, 1, 1, write_test_buffer) == SUCCESS &&
            mirror->ops->read(mirror, 0, 5, read_test_buffer) == SUCCESS &&
            validate_full_buffer( read_test_buffer, (int[]){ 77, 88, 2, 3, 4 } ) ) ) {
        total_fails++;
        if( mode != 5 )
            printf( "mirror_write_tests: write 1 block from base 1 - FAILED\n");
    }

    write_data( write_test_buffer, BLOCK_SIZE, 44 );
    write_data( write_test_buffer+BLOCK_SIZE, BLOCK_SIZE, 55 );
    write_data( write_test_buffer+2*BLOCK_SIZE, BLOCK_SIZE, 66 );
    if( !(mirror->ops->write(mirror, 0, 3, write_test_buffer) == SUCCESS &&
            mirror->ops->read(mirror, 0, 5, read_test_buffer) == SUCCESS &&
            validate_full_buffer( read_test_buffer, (int[]){ 44, 55, 66, 3, 4 } ) ) ) {
        total_fails++;
        if( mode != 5 )
            printf( "mirror_write_tests: write 3 block from base 0 - FAILED\n");
    }
    
    write_data( write_test_buffer, BLOCK_SIZE, 22 );
    write_data( write_test_buffer+BLOCK_SIZE, BLOCK_SIZE, 33 );
    write_data( write_test_buffer+2*BLOCK_SIZE, BLOCK_SIZE, 99 );
    if( !(mirror->ops->write(mirror, 2, 3, write_test_buffer) == SUCCESS &&
            mirror->ops->read(mirror, 0, 5, read_test_buffer) == SUCCESS &&
            validate_full_buffer( read_test_buffer, (int[]){ 44, 55, 22, 33, 99 } ) ) ) {
        total_fails++;
        if( mode != 5 )
            printf( "mirror_write_tests: write 3 block from base 2 - FAILED\n");
    }

    if( (total_fails == 0 && mode != 5) || (total_fails == 4 && mode == 5) )
        printf( "mirror_write_tests: actual write/validation tests - PASSED\n");
    else
        printf( "mirror_write_tests: actual write/validation tests - FAILED\n");

    // reset data for next test
    for( int i=0; i<5; i++ ) {
        write_data( write_test_buffer, BLOCK_SIZE, i );
        mirror->ops->write(mirror, i, 1, write_test_buffer);
    }


}
void run_close_tests( struct blkdev *mirror, int mode ) {
    printf( "mirror_close_tests: \n");
}

int main(){

    printf( "Initial provided tests:\n\n");
    
    struct blkdev* mirror_drives[2];
    /* Create two images for the mirror */
    mirror_drives[0] = create_new_image("mirror1", 2, 0);
    mirror_drives[1] = create_new_image("mirror2", 2, 0);
    /* Create the raid mirror */
    struct blkdev * mirror = mirror_create(mirror_drives);

    /* Write some data to the mirror, then read the data back and check that the
     * two buffers contain the same bytes.
     */
    char write_buffer1[BLOCK_SIZE];
    char write_buffer2[BLOCK_SIZE];
    write_data(write_buffer1, BLOCK_SIZE, 1);
    write_data(write_buffer2, BLOCK_SIZE, 2);
    if (blkdev_write(mirror, 0, 1, write_buffer1) != SUCCESS){
        printf("Write failed!\n");
        exit(0);
    }

    char read_buffer[BLOCK_SIZE];
    /* Zero out the buffer to make sure blkdev_read() actually does something */
    bzero(read_buffer, BLOCK_SIZE);

    if (blkdev_read(mirror, 0, 1, read_buffer) != SUCCESS){
        printf("Read failed!\n");
        exit(0);
    }

    /* For debugging, you can analyze these files manually */
    dump(write_buffer1, BLOCK_SIZE, "write-buffer");
    dump(read_buffer, BLOCK_SIZE, "read-buffer");

    if (memcmp(write_buffer1, read_buffer, BLOCK_SIZE) != 0){
        printf("Read doesn't match write!\n");
    } else {
        printf("Mirror read test passed\n");
    }

    printf( "\n\nAdditional functional tests:\n\n");


    /* TODO: Your tests here */
    run_create_tests();
    struct blkdev *test_mirror = NULL;
    for( int mode=0; mode<6; mode++ ) {
        test_mirror = setup_test_mirror( test_mirror, mode );
        run_read_tests( test_mirror, mode );
        run_write_tests( test_mirror, mode );
        run_close_tests( test_mirror, mode );
    }
}
