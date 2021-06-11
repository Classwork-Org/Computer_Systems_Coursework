CXX=gcc

CXXFLAGS=-g -Wall

CEASER_TARGET= ceaser.bin 
CEASER_OBJS= ceaser.o

QUEUE_TARGET= queue.bin
QUEUE_OBJS= queue.o
QUEUE_INCLUDES= queue.h

TARGET= $(CEASER_TARGET) $(QUEUE_TARGET)
OBJS=  $(CEASER_OBJS) $(QUEUE_OBJS)

CXXCOMPILE=$(CXX) $(CXXFLAGS)

all: clean $(CEASER_TARGET) $(QUEUE_TARGET)

%.o: %.c 
	$(CXXCOMPILE) -c -o $@  $<

$(CEASER_TARGET): $(CEASER_OBJS)
	$(CXXCOMPILE) $^ -g -o $@

$(QUEUE_TARGET): $(QUEUE_INCLUDES) $(QUEUE_OBJS)
	$(CXXCOMPILE) $^ -g -o $@

ceaser: $(CEASER_TARGET)
	./ceaser.bin

queue: $(QUEUE_TARGET)
	./queue.bin

clean: 
	rm -rf $(TARGET) $(OBJS)
