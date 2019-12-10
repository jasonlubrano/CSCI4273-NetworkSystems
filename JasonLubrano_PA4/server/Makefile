#Makefile
all: ./dfs.c
	gcc dfs.c -pthread -g -o ./dfsserver -lcrypto -lssl

clean:
	$(RM) *.o ./dfsserver
