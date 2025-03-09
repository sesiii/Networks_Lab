all:
	gcc ksocket.c initksocket.c -o initksocket -lpthread
	gcc ksocket.c user1.c -o user1 -lpthread
	gcc ksocket.c user2.c -o user2 -lpthread
	./initksocket

clean:
	rm -f initksocket user1 user2