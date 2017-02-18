all:

	gcc -o larc lab1fixd.c
	gcc -o larc_mem -fsanitize=address lab1fixd.c
