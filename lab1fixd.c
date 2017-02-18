#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>

// Archive file stream.
FILE *larc;
// General-purpose buffer. Useful for changing data through functions.
char *buf;
// Contains path to packing/unpacking directory 
// so it can be cut off or added where necessary.
char *packingDir;

// Takes full path of the file and truncates name of the file.
void GetFilePath(char *fullPath)
{
	int i = 0;
	int n = 0;
	while (fullPath[i] != '\0')  {
		if (fullPath[i] == '/')
		  n = i;
		i++;
	}
	buf = (char *)calloc(n + 1, 1);
	i = 0;
	for (int j = 0; j < n; i++, j++)
		buf[i] = fullPath[j];
}

// Corrects path of the directory or file, cutting off the part
// that is in packingDir buffer (to make relative paths out of absolute
// for future unpacking).
void CorrectPackingPath(void)
{
	int dif = strlen(buf) - strlen(packingDir);
	char *tbuf = (char *)calloc(dif + 1, 1);
	int offset = strlen(packingDir);
	for (int i = 0; i <= dif; i++)
		tbuf[i] = buf[i + offset];
	free(buf);
	buf = (char *)calloc(1024, 1);
	for (int i = 0; i < dif; i++)
		buf[i] = tbuf[i];
	free(tbuf);
}

// Recursive function which packs information about directories and files
// into one file using strict data arrangement.
void PackIntoArchive(char *dirToPack)
{
	// Open directory.
	printf("packing dir  %s\n", dirToPack);
	chdir(dirToPack);
	DIR *curDir = opendir(dirToPack);
	// Write dir info to restore directories on unpacking.
	if (buf != NULL)
		free(buf);
	buf = (char *)calloc(1024, 1);
	getcwd(buf, 1024);
	CorrectPackingPath();
	size_t l = strlen(buf);
	fwrite(&l, 1, sizeof(l), larc);
	fwrite(buf, 1, l, larc);
	long int i = -1;
	fwrite(&i, 1, sizeof(i), larc);
	if (buf != NULL)
		free(buf);
	buf = NULL;
	// Looking through dir entries. 
	// Information about cur entry will be in 'fileinfo'.
	struct dirent *fileinfo;
	while ((fileinfo = readdir(curDir)) != NULL)  {
		struct stat filestat;
		lstat(fileinfo->d_name, &filestat);
		// If it is not a file, but a subdirectory...
		if (S_ISDIR(filestat.st_mode))  {
			// Skip directories-'references'.
			if (strcmp(".", fileinfo->d_name) == 0 || strcmp("..", fileinfo->d_name) == 0)
				continue;
			// Recursively go to subdirectory.
			buf = (char *)calloc(1024, 1);
			getcwd(buf, 1024);
			strcat(buf, "/");
			strcat(buf, fileinfo->d_name);
			PackIntoArchive(buf);
			if (buf != NULL)
				free(buf);
			buf = NULL;  
		}  else  {
			FILE *f = fopen(fileinfo->d_name, "rb");
			
			// For file - add to archive:
			// - path length
			buf = (char *)calloc(1024, 1);
			getcwd(buf, 1024);
			CorrectPackingPath();
			strcat(buf, "/");
			strcat(buf, fileinfo->d_name);
			printf("packing file %s\n", buf);
			size_t len = strlen(buf);
			fwrite(&len, 1, sizeof(len), larc);

			// - relative path of the file with its name
			fwrite(buf, 1, len, larc);

			// - file length
			len = filestat.st_size;
			fwrite(&len, 1, sizeof(len), larc);

			// - file itself
			free(buf);
			buf = (char *)calloc(len, 1);
			fread(buf, 1, (size_t)(len), f);
			fwrite(buf, 1, (size_t)(len), larc);

			free(buf);
			buf = NULL;
			fclose(f);
		}
	}

	closedir(curDir);
	chdir("..");
}

// Simple function which decompiles archive into directories and files
// using the knowledge about strict archive structure.
void UnpackArchive(void)
{
	size_t len;
	fread(&len, 1, sizeof(len), larc);
	while (!feof(larc))  {
		buf = (char *)calloc(len+1, 1);
		fread(buf, 1, len, larc);
		long int l;
		fread(&l, 1, sizeof(l), larc);
		// Make a path (suits both for folders and files).
		char *tbuf = (char *)calloc(strlen(packingDir) + strlen(buf) + 1, 1);
		strcat(tbuf, packingDir);
		strcat(tbuf, buf);
		printf("creating entry %s\n", tbuf);
		if (l == -1)  {
			// It's a dir. Create it if it doesn't exist.
			if (chdir(tbuf) == -1)
				mkdir(tbuf, 0600);
		}  else  {
			// It's a file. Create it and fill in data.
			char *filebuf = (char *)calloc(l, 1);
			fread(filebuf, 1, l, larc);
			FILE *nf = fopen(tbuf, "wb");
			fwrite(filebuf, 1, l, nf);
			fclose(nf);
			free(filebuf);
		}
		free(tbuf);
		free(buf);
		fread(&len, 1, sizeof(len), larc);
	}
}

// Main function, handles input arguments, errors and starts packing/unpacking.
int main(int argc, char *argv[])
{
	if (argc == 1)  {
		printf("To use larc, write two arguments:\n");
		printf("1) 'pack' or 'unpack' action keyword;\n");
		printf("  after completing action, target directory");
		printf(" and archive will be in the same folder.\n");
		printf("2) path to directory which you want to pack\n\t\t\tOR\n");
		printf("   path to archive you want to unpack.\n");
		exit(1);
	}  else if (argc < 3 || argc > 3)  {
		printf("Error! Wrong number of arguments.\n");
		printf("larc stopped with error.\n");
		exit(2);
	}  else  {
		if (strcmp(argv[1], "pack") == 0)  {
			packingDir = (char *)calloc(1024, 1);
			if (chdir(argv[2]) == -1)  {
				printf("ERROR! Cannot pack directory with path = '%s'\n", argv[2]);
				printf("Maybe you spell argument '2' wrongly.\n");
				printf("larc stopped with error.\n");
				exit(3);
			}
			chdir("..");
			getcwd(packingDir, 1024);
			// Create path for an archive and the archive itself.
			chdir(argv[2]);
			buf = (char *)calloc(1024, 1);
			getcwd(buf, 1024);
			strcat(buf, ".larc");
			larc = fopen(buf, "wb");
			free(buf);
			buf = NULL;
			// Start packing.
			PackIntoArchive(argv[2]);
			fclose(larc);
			free(packingDir);
		}  else if (strcmp(argv[1], "unpack") == 0)  {
			GetFilePath(argv[2]);
			packingDir = (char *)calloc(strlen(buf)+1, 1);
			strcpy(packingDir, buf);
			printf("%s\n", packingDir);
			free(buf);
			larc = fopen(argv[2], "rb");
			if (larc == NULL)  {
				printf("ERROR! Archive with path = '%s' doesn't exist.\n", argv[2]);
				printf("larc stopped with error.\n");
				exit(5);
			}
			UnpackArchive();
			fclose(larc);
			free(packingDir);
		}  else  {
			printf("ERROR! Cannot understand argument '1' = '%s'\n", argv[1]);
			printf("larc stopped with error.\n");
			exit(4);
		}
	}
	exit(0);
}