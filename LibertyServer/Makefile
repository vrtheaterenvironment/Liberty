CC=gcc
LIBS = -lpthread -lusb-1.0
CFLAGS = -Wall -O0 -DDEBUG -D_XOPEN_SOURCE=500
TARGET = server

all: $(TARGET) Makefile

$(TARGET): main.c IntList.o Server.o Liberty.o
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

%.o : %.c
	$(CC) -c $(CFLAGS) $<

.PHONY: clean archive
clean:
	rm -f $(TARGET) *~ *.o
archive:
	tar czvf ServerLiberty.tar.gz ./*.c ./*.h ./Makefile
