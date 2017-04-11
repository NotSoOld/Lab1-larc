all:

	gcc -o larc lab1.c
	gcc -o -fsanitize=address larc_mem lab1.c
