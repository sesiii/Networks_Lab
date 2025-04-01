all: libksocket.a initksocket user1 user2

libksocket.a: ksocket.o
	ar rcs libksocket.a ksocket.o

ksocket.o: ksocket.c ksocket.h
	gcc -c ksocket.c

initksocket: initksocket.o libksocket.a
	gcc -o initksocket initksocket.o -L. -lksocket -lpthread

initksocket.o: initksocket.c ksocket.h
	gcc -c initksocket.c

user1: user1.o libksocket.a
	gcc -o user1 user1.o -L. -lksocket

user1.o: user1.c ksocket.h
	gcc -c user1.c

user2: user2.o libksocket.a
	gcc -o user2 user2.o -L. -lksocket

user2.o: user2.c ksocket.h
	gcc -c user2.c

clean:
	rm -f *.o libksocket.a initksocket user1 user2