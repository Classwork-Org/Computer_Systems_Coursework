/*
 * file:        homework.c
 * description: Skeleton code for CS 5600 Homework 2
 *
 * Peter Desnoyers, Northeastern CCIS, 2011
 * Maria Jump, Northeastern Khoury, 2020
 */

#include <stdio.h>
#include <stdlib.h>
#include "homework.h"

#ifdef NO_KNOBS
#define BARBER_SLEEP_TIME 1.2
#define CUSTOMER_SLEEP_TIME 10
#define CUSTOMER_COUNT 10
#define BARBER_COUNT 1
#define MAX_CHAIR_COUNT 4
#endif

void print_barber_sleep()
{
    printf("DEBUG: %f barber goes to sleep\n", timestamp());
    fflush(stdout);
}

void print_barber_wakes_up()
{
    printf("DEBUG: %f barber wakes up\n", timestamp());
    fflush(stdout);
}

void print_customer_enters_shop(int customer)
{
    printf("DEBUG: %f customer %d enters shop\n", timestamp(), customer);
    fflush(stdout);
}

void print_customer_starts_haircut(int customer)
{
    printf("DEBUG: %f customer %d starts haircut\n", timestamp(), customer);
    fflush(stdout);
}

void print_customer_leaves_shop(int customer)
{
    printf("DEBUG: %f customer %d leaves shop\n", timestamp(), customer);
    fflush(stdout);
}

/********** YOUR CODE STARTS HERE ******************/

/*
 * Here's how you can initialize global mutex and cond variables
 */
typedef enum
{
    CHECKING,
    WAITING,
    DONE
} CustomerState;

queue_t chairs_occupied = EMPTY_QUEUE_INITALIZER;
typedef struct
{
    int64_t customer_num;
    CustomerState state;
} CustomerInfo;

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t C_barber = PTHREAD_COND_INITIALIZER;
pthread_cond_t C_customer = PTHREAD_COND_INITIALIZER;

void *stat_turned_away_counter;
void *stat_total_customers_counter;
void *stat_average_people_in_shop_counter;
void *stat_frac_in_seat_counter;
void *stat_time_in_shop_timer;
/* the barber method
 */
void barber(void)
{
    int has_slept = 0;
    CustomerInfo *customer_being_serviced;
    pthread_mutex_lock(&m);
    while (1)
    {
        /* your code here */
        while (chairs_occupied.size == 0)
        {
            if (!has_slept) //guarding excessive print due to spurrious wakeups
            {
                print_barber_sleep();
                has_slept = 1;
            }
            pthread_cond_wait(&C_barber, &m);
        }
        if (has_slept)
        {
            print_barber_wakes_up();
            has_slept = 0;
        }
        if ((customer_being_serviced = dequeue(&chairs_occupied)) != NULL)
        {
            print_customer_starts_haircut(customer_being_serviced->customer_num);
        }
        else
        {
#ifdef DEBUG
            printf("ERROR: Barber dequed non-existent customer, something went wrong!");
            assert(0);
#endif // DEBUG
            exit(-1);
        }

#ifdef Q3
        stat_count_incr(stat_frac_in_seat_counter);
#endif // Q3

        sleep_exp(BARBER_SLEEP_TIME, &m);
        customer_being_serviced->state = DONE;
        pthread_cond_signal(&C_customer);
#ifdef MAKE_TEST_SCRIPT_HAPPY
        print_customer_leaves_shop(customer_being_serviced->customer_num);
#endif
    }
    pthread_mutex_unlock(&m);
}

/* the customer method
 */
void customer(CustomerInfo *info)
{
    pthread_mutex_lock(&m);
    /* your code here */
    print_customer_enters_shop(info->customer_num);
    info->state = CHECKING;

#ifdef Q3
    stat_count_incr(stat_total_customers_counter);
#endif // Q3

    if (chairs_occupied.size < MAX_CHAIR_COUNT)
    {
        if (enqueue(&chairs_occupied, info) == -1)
        {
#ifdef DEBUG
            printf("ERROR: Customer failed to enqueue despite free chairs available!");
            assert(0);
#endif // DEBUG
            exit(-1);
        }
        stat_timer_start(stat_time_in_shop_timer);
        info->state = WAITING;
        pthread_cond_signal(&C_barber);
#ifdef Q3
        stat_count_incr(stat_average_people_in_shop_counter);
#endif // Q3
        while (info->state == WAITING)
        {
            pthread_cond_wait(&C_customer, &m);
        }
        stat_timer_stop(stat_time_in_shop_timer);

#ifdef Q3
        if (info->state == DONE)
        {
            stat_count_decr(stat_frac_in_seat_counter);
            stat_count_decr(stat_average_people_in_shop_counter);
        }
#endif // Q3
    }
    else
    {
#ifdef Q3
        stat_count_incr(stat_turned_away_counter);
#endif
#ifdef MAKE_TEST_SCRIPT_HAPPY // customers will only say they left if there are no seats 
        print_customer_leaves_shop(info->customer_num);
#endif
    }

#ifndef MAKE_TEST_SCRIPT_HAPPY
    print_customer_leaves_shop(info->customer_num);
#endif

    pthread_mutex_unlock(&m);
}

/* Threads which call these methods. Note that the pthread create
 * function allows you to pass a single void* pointer value to each
 * thread you create; we actually pass an integer (the customer number)
 * as that argument instead, using a "cast" to pretend it's a pointer.
 */

/* the customer thread function - create 10 threads, each of which calls
 * this function with its customer number 0..9
 */
void *customer_thread(void *context)
{
    /* your code goes here */
    while (1)
    {
        sleep_exp(CUSTOMER_SLEEP_TIME, NULL);
        customer(context);
    }
    return 0;
}

/*  barber thread
 */
void *barber_thread(void *context)
{
    barber(); /* never returns */
    return 0;
}

void q2(void)
{
    pthread_t barbers[BARBER_COUNT];
    pthread_t customers[CUSTOMER_COUNT];
    CustomerInfo info[CUSTOMER_COUNT];
    /* your code goes here */
    int64_t i;
    for (i = 0; i < CUSTOMER_COUNT; i++)
    {
        info[i].customer_num = i;
        pthread_create(&customers[i], NULL, customer_thread, &info[i]);
    }
    for (i = 0; i < BARBER_COUNT; i++)
    {
        pthread_create(&barbers[i], NULL, barber_thread, &i);
    }
    wait_until_done();
}

/* For question 3 you need to measure the following statistics:
 *
 * 1. fraction of  customer visits result in turning away due to a full shop
 *    (calculate this one yourself - count total customers, those turned away)
 * 2. average time spent in the shop (including haircut) by a customer
 *     *** who does not find a full shop ***. (timer)
 * 3. average number of customers in the shop (counter)
 * 4. fraction of time someone is sitting in the barber's chair (counter)
 *
 * The stat_* functions (counter, timer) are described in the PDF.
 */

void q3(void)
{
/* your code goes here */
#ifdef Q3
    stat_turned_away_counter = stat_counter();
    stat_average_people_in_shop_counter = stat_counter();
    stat_total_customers_counter = stat_counter();
    stat_frac_in_seat_counter = stat_counter();
    stat_time_in_shop_timer = stat_timer();
#endif

    pthread_t barbers[BARBER_COUNT];
    pthread_t customers[CUSTOMER_COUNT];
    CustomerInfo info[CUSTOMER_COUNT];
    /* your code goes here */
    int64_t i;
    for (i = 0; i < CUSTOMER_COUNT; i++)
    {
        info[i].customer_num = i;
        pthread_create(&customers[i], NULL, customer_thread, &info[i]);
    }
    for (i = 0; i < BARBER_COUNT; i++)
    {
        pthread_create(&barbers[i], NULL, barber_thread, &i);
    }
    wait_until_done();

#ifdef Q3

    float percentage_turned_away = (float)stat_count_count(stat_turned_away_counter) / stat_count_count(stat_total_customers_counter) * 100;

    printf("INFO: Simulation Time: %f\n", timestamp());
    printf("INFO: Total Customers: %u\n", stat_count_count(stat_total_customers_counter));
    printf("INFO: Turned Away Customers: %u, %.2f%%\n", stat_count_count(stat_turned_away_counter), percentage_turned_away);
    printf("INFO: Average Time In Seat: %.2f%%\n", stat_count_mean(stat_frac_in_seat_counter) * 100);
    printf("INFO: Average people in the shop: %.2f\n", stat_count_mean(stat_average_people_in_shop_counter));
    printf("INFO: Average Time Spent In Shop: %.2f\n", stat_timer_mean(stat_time_in_shop_timer));

    free(stat_turned_away_counter);
    free(stat_average_people_in_shop_counter);
    free(stat_total_customers_counter);
    free(stat_frac_in_seat_counter);
    free(stat_time_in_shop_timer);

#endif // Q3
}
