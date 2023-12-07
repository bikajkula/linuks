//usage:#define showtext_trivial_usage
//usage:       "showtext INPUT_FILE.txt NUM_OF_UPPERCASE NUM_OF_LOWERCASE"
//usage:#define showtext_full_usage "\n\n"
//usage:       "showtext prints the first N characters of input file in uppercase and M characters in lowercase.\n"

#include "busybox.h"
//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>

char toUpper(char c);
char toLower(char c);

int showtext_main(int argc, char **argv)
{

	int retval = EXIT_SUCCESS;
	if (argc < 4) {
		bb_show_usage();
		//printf("invalid arg\n");
	} 
	
	int num_of_up = atoi(argv[2]);
	int num_of_low = atoi(argv[3]);
	
	char output[num_of_up + num_of_low + 1];
	int c, i;
	int j = 0;

	FILE *file = fopen(argv[1], "r");
	if(file == NULL) {
		printf("Couldn't open file!\n");
		retval = EXIT_FAILURE;
	}

	for(i = 0; i < num_of_up; i++) {
		c = fgetc(file);
		output[j++] = toUpper(c);
	}
	for(i = 0; i < num_of_low; i++) {
		c = fgetc(file);
		output[j++] = toLower(c);
	}
	output[j] = '\0';
	
	printf("Output: %s\n", output);
	fclose(file);


	return retval;
}

char toUpper(char c) {
	if(c >= 'a' && c <= 'z') {
		c -= 32;
		return c;
	}
	else {
		return c;
	}
}

char toLower(char c) {
	if(c >= 'A' && c <= 'Z') {
		c += 32;
		return c;
	}
	else {
		return c;
	}
}
