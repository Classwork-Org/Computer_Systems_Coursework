#
# Makefile for Project 2 -- barber shop
#
# KNOB DEFAULTS
BARBER_SLEEP_TIME = 1.2
CUSTOMER_SLEEP_TIME = 10
CUSTOMER_COUNT = 10
BARBER_COUNT = 1
MAX_CHAIR_COUNT = 4

define KNOBS
-DBARBER_SLEEP_TIME=$(BARBER_SLEEP_TIME) \
-DCUSTOMER_SLEEP_TIME=$(CUSTOMER_SLEEP_TIME) \
-DCUSTOMER_COUNT=$(CUSTOMER_COUNT) \
-DBARBER_COUNT=$(BARBER_COUNT) \
-DMAX_CHAIR_COUNT=$(MAX_CHAIR_COUNT)
endef

# these flags to turn on warnings and to include additional debugging information
CFLAGS = -Wall -ggdb3 
CC = gcc

# indicate the location of the pth library, by default we are using your local directory
# you need to change this if you installed it on your system instead
PTH = pth-2.0.7/install

part-2: queue.h queue.c homework.h homework.c misc.c
	rm -f part-2
	$(CC) -g -O1 $(CFLAGS) -DNO_KNOBS -DMAKE_TEST_SCRIPT_HAPPY -o part-2 queue.c homework.c misc.c -DQ2 -lpthread -lm

part-3-HAPPY: queue.h homework.h homework.c misc.o
	rm -f part-3
	$(CC) -I $(PTH)/include -g $(CFLAGS) -DNO_KNOBS -DMAKE_TEST_SCRIPT_HAPPY -o part-3 queue.c homework.c misc.c -DQ3 -L $(PTH)/lib -Wl,-rpath=$(PTH)/lib -lpth -lm 

part-3: queue.h homework.h homework.c misc.o
	rm -f part-3
	$(CC) -I $(PTH)/include -g $(CFLAGS) $(KNOBS) -o part-3 queue.c homework.c misc.c -DQ3 -L $(PTH)/lib -Wl,-rpath=$(PTH)/lib -lpth -lm 


clean:
	rm -f *.o part-2 part-3 out.txt
