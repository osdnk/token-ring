CC=gcc
OPT=-Wall
LIB=-pthread

all: client.o
	$(CC) $(OPT) client.o -o client $(LIB)

clean:
	rm -f *.o client
	make all

udp1:
	./client pierwszy 9011 127.0.0.1 9011 1 udp
udp2:
	./client drugi 9012 127.0.0.1 9011 0 udp
udp3:
	./client trzeci 9013 127.0.0.1 9011 0 udp

tcp1:
	./client pierwszy 9011 127.0.0.1 9011 1 tcp
tcp2:
	./client drugi 9012 127.0.0.1 9011 0 tcp
tcp3:
	./client trzeci 9013 127.0.0.1 9011 0 tcp


logger:
	node logger.js