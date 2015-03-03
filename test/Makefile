SRC_FILES= $(wildcard *.c)
OBJ_FILES= $(SRC_FILES:.c=.o)

TESTS = generators-0 generators-1 \
	generators-3 generators-4 generators-5 \
	file-pdu file-pdu-2 file-pdu-3 \
	probes-1 probes-2 probes-3 probes-4 \
	muxdemux rr-mux \
	drr \
	source-1 source-2 \
	event-test
#	debits \
#	muxfcfs-1 \
#	intconf


.PHONY: clean 

all : $(TESTS)

tests :  $(TESTS)
	./run-tests.sh $(TESTS)

event-test : event-test.o ../$(SRC_DIR)/libndes.a
	$(CC) event-test.o -o event-test $(LDFLAGS)

source-1 : source-1.o ../$(SRC_DIR)/libndes.a
	$(CC) source-1.o -o source-1 $(LDFLAGS)

source-2 : source-2.o ../$(SRC_DIR)/libndes.a
	$(CC) source-2.o -o source-2 $(LDFLAGS)

drr : drr.o ../$(SRC_DIR)/libndes.a
	$(CC) drr.o -o drr $(LDFLAGS)

rr-mux : rr-mux.o ../$(SRC_DIR)/libndes.a
	$(CC) rr-mux.o -o rr-mux $(LDFLAGS)

muxdemux : muxdemux.o ../$(SRC_DIR)/libndes.a
	$(CC) muxdemux.o -o muxdemux $(LDFLAGS)

generators-0 : generators-0.o ../$(SRC_DIR)/libndes.a
	$(CC) generators-0.o -o generators-0 $(LDFLAGS)

generators-1 : generators-1.o ../$(SRC_DIR)/libndes.a
	$(CC) generators-1.o -o generators-1 $(LDFLAGS)

generators-3 : generators-3.o ../$(SRC_DIR)/libndes.a
	$(CC) generators-3.o -o generators-3 $(LDFLAGS)

generators-4 : generators-4.o ../$(SRC_DIR)/libndes.a
	$(CC) generators-4.o -o generators-4 $(LDFLAGS)

generators-5 : generators-5.o ../$(SRC_DIR)/libndes.a
	$(CC) generators-5.o -o generators-5 $(LDFLAGS)

intconf : intconf.o ../$(SRC_DIR)/libndes.a
	$(CC) $(LDFLAGS) intconf.o -o intconf $(LDFLAGS)

debits : debits.o ../$(SRC_DIR)/libndes.a
	$(CC) $(LDFLAGS) debits.o -o debits $(LDFLAGS)

muxfcfs-1 : muxfcfs-1.o ../$(SRC_DIR)/libndes.a
	$(CC) $(LDFLAGS) muxfcfs-1.o -o muxfcfs-1 $(LDFLAGS)

probes-1 : probes-1.o ../$(SRC_DIR)/libndes.a
	$(CC) $(LDFLAGS) probes-1.o -o probes-1 $(LDFLAGS)

probes-2 : probes-2.o ../$(SRC_DIR)/libndes.a
	$(CC) $(LDFLAGS) probes-2.o -o probes-2 $(LDFLAGS)

probes-3 : probes-3.o ../$(SRC_DIR)/libndes.a
	$(CC) $(LDFLAGS) probes-3.o -o probes-3 $(LDFLAGS)

probes-4 : probes-4.o ../$(SRC_DIR)/libndes.a
	$(CC) $(LDFLAGS) probes-4.o -o probes-4 $(LDFLAGS)

file-pdu : file-pdu.o ../$(SRC_DIR)/libndes.a
	$(CC) $(LDFLAGS) file-pdu.o -o file-pdu $(LDFLAGS)

file-pdu-2 : file-pdu-2.o ../$(SRC_DIR)/libndes.a
	$(CC) $(LDFLAGS) file-pdu-2.o -o file-pdu-2 $(LDFLAGS)

file-pdu-3 : file-pdu-3.o ../$(SRC_DIR)/libndes.a
	$(CC) $(LDFLAGS) file-pdu-3.o -o file-pdu-3 $(LDFLAGS)

src-exp : src-exp.o ../$(SRC_DIR)/libndes.a
	$(CC) $(LDFLAGS) src-exp.o -o src-exp $(LDFLAGS)

clean :
	\rm -f $(OBJ_FILES) $(TESTS)

.c.o :
	$(CC) $(CFLAGS) -I../$(INCL_DIR) $< -c

