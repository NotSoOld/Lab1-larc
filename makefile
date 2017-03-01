all:

	gcc -o -std=c99 lab1.c larc
	gcc -o -std=c99 -fsanitize=address lab1.c larc_mem
