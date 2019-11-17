#Makefile
all: ./proxyserver.c
	gcc proxyserver.c -pthread -g -o ./webproxy -lcrypto -lssl

clean:
	$(RM) *.o ./webproxy
