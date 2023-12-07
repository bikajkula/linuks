#include "busybox.h"

// predavanja 61, strana 26


int capitalize_main(int argc, char **argv) MAIN_EXTERNALLY_VISIBLE;
int capitalize_main(int argc, char **argv)
{
	
	if (argc < 3) bb_show_usage();
	/*{
		printf("Jok\n"); // treba bb_show_usage ili tako nesto
		return EXIT_FAILURE;
	}*/
		

	FILE *out = fopen_or_warn(argv[argc - 1], "w");
	if (out == NULL) {
		return EXIT_FAILURE;
	}

	int i;
	for (i = 1; i < argc - 1; i++) {
		FILE *fd = fopen_or_warn(argv[i], "r");
		
		if (fd == NULL) {
			return EXIT_FAILURE;
		};
		
		int c;
		while ((c = fgetc(fd)) != EOF) {
			fputc(toupper(c), out);
		}
		
		fclose(fd);
	}
	
	fclose(out);
	
	return EXIT_SUCCESS;
}

