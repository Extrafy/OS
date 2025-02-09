all: fibo.c main.c
	gcc -c fibo.c -I../include
	gcc -c main.c -I../include
	gcc fibo.o main.o -o fibo
	cp fibo ..
	rm fibo
clean: 
	rm *.o
