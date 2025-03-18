all:
	gcc -o mysmtp_server mysmtp_server.c -pthread
	gcc -o mysmtp_client mysmtp_client.c
	./mysmtp_server 2525
clean:
	rm -f mysmtp_server mysmtp_client
	
# In another terminal
# ./mysmtp_client 192.168.1.100 2525