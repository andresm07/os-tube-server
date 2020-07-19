server: server.c utils.c server.h
	gcc server.c -o server -lpthread -lcrypto -lrt


test: testfile.c utils.c server.h
	gcc testfile.c -o test -lcrypto
