all : ./server/uftp_server.c ./client/uftp_client.c
	gcc ./server/uftp_server.c -o ./server/server
	gcc ./client/uftp_client.c -o ./client/client

clean:
	$(RM) *.o ./client/client ./server/server