all : main

main : sicsim.o 
	gcc -g -Wall -lm -o sicsim.out sicsim.o

sicsim.o : sicsim.c
	gcc -g -c -Wall -lm sicsim.c

clear :
	rm -f sicsim.o sicsim.out
