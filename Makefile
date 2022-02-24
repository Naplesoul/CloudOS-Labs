# NOTE: Feel free to change the makefile to suit your own need.

# compile and link flags
CCFLAGS = -Wall -g -std=c++11
LDFLAGS = -Wall -g -std=c++11

# make rules
TARGETS = rdt_sim 

all: $(TARGETS)

.cc.o:
	g++ $(CCFLAGS) -c -o $@ $<

rdt_sender.o: 	rdt_struct.h rdt_sender.h utils/buffer.h utils/checksum.h utils/timer.h

rdt_receiver.o:	rdt_struct.h rdt_receiver.h utils/buffer.h utils/checksum.h

utils/buffer.o: utils/buffer.h

utils/checksum.o: utils/checksum.h

utils/timer.o: utils/timer.h

rdt_sim.o: 	rdt_struct.h

rdt_sim: rdt_sim.o rdt_sender.o rdt_receiver.o utils/buffer.o utils/checksum.o utils/timer.o
	g++ $(LDFLAGS) -o $@ $^

clean:
	rm -f *~ *.o utils/*.o tmp.txt $(TARGETS)
