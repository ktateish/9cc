9cc: main.c
	$(CC) -o 9cc main.c $(LDFLAGS)

test: 9cc
	./9cc -test
	./test.sh

clean:
	rm -f 9cc *.o *~ tmp*
