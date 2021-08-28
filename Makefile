all:
	gcc -o dct-tests dct-tests.c -lm -pthread
	gcc -o dct-single dct-single.c -lm -pthread

clean:
	rm -f dct-tests dct-single