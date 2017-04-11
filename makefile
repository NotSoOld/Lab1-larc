all:

	gcc -o larc lab1.c
	gcc -o larc_mem lab1.c -fsanitize=address
	
# Add -std=c99 if old compiler jerks
