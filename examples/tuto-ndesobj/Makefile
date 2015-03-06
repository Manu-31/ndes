NDES_PATH=../..

export CC=gcc

export CFLAGS=-Wall -I$(NDES_PATH)/include -I. -DDEBUG_NDES
export LDFLAGS=-O3 -L$(NDES_PATH)/lib -lndes -lm -I.

OBJFILES=test-dir.o contact.o directory.o

test-dir : test-dir.o contact.o directory.o 
	$(CC) $(LDFLAGS) $(OBJFILES) -o test-dir -lndes -lm

clean :
	\rm -f $(OBJFILES) test-dir

