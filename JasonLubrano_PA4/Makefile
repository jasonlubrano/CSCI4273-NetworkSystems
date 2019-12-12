#Makefile
all: ./dfserv.c ./dfcli.c
	gcc dfserv.c -pthread -g -o ./dfserv -lcrypto -lssl -lm
	gcc dfcli.c -pthread -g -o ./dfcli -lcrypto -lssl -lm

clean:
	$(RM) *.o ./dfserv ./dfcli
