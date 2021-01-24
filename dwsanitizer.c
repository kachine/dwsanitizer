/*
 Monolithic DirectWave file (*.dwp) storage information sanitizer
 
 ATTENTION:
   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   THERE IS NO GUARANTEE OR WARRANTY AT ALL, USE AT YOUR OWN RISK.
   !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   dwp file is Image-line's proprietary format, no information is disclosed about it.
   So, this tool might not be work as I intended. (I tested using DirectWave in FL STUDIO v20.6.2 build 1549, v20.7.2 build 1863)
 
 What is this:
   In the DirectWave monolithic *.dwp file, some user personal information will be included in the path.
   For example, the path of dwp file itself, such a information seems to be unnecessary, is included in the binary like below:
     C:\Users\%USERNAME%\Documents\Image-Line\DirectWave\foo.dwp
       *Note: %USERNAME% is not recorded as environmental variable, actual raw value (might be sensitive information) is recorded in the dwp file.
   Or, monolithic dwp file contains waveform itself, so it doesn't require the path of the source sample wave file. But the information is recorded in the file like below:
     %SystemDrive%\PATH_TO\bar.wav
 
   If you share a dwp files to the others, theose information will be included unintentionally. I don't feel comfortable if those information were shared with the others.
   This tool removes these informations from monolithic dwp file.
 
 How to build:
   gcc dwsanitizer.c -o dwsanitizer
 
 How to use:
   If you want to sanitize foo.dwp,
     ./dwsanitizer foo.dwp
   would generates O_foo.dwp and the file will be sanitized.
     *Note: Output filename is simply added prefix "O_" to input file, if the same filename is exists, it will be overwritten without notice.
            Do NOT specify the path to target dwp file, it's not supported.
 
 Limitation (Known bug):
   The dwp file format is proprietary and not disclosed to public. So, this implementation is only an ad hoc based on my very limited research.
   If waveforms in the monolithic dwp file contains binary sequences 0xF6 0x01 0x00 0x00 [ANY] 0x00 0x00 0x00 0x00 0x00 0x00 0x00, this tool would be in malfunction.
   I recommend to keep original dwp file to prevent data loss or future incompatibilities.
*/
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<inttypes.h>

#define DEBUG 0
#define OUTPREFIX "O_"
#define DWPID "DwPr&"
#define ADDR_PROG_LEN 0x5E
#define ADDR_PROG_NAME	0x66
#define ADDR_OFFSET_PROG_PATH_LEN 4
#define ADDR_OFFSET_PROG_PATH 12
#define ZONEID	0xF6
//#define DUMMYSAMPLE "%SystemDrive%\\x.wav"
unsigned char ZONEHDR[] = {0x01, 0x00, 0x00};
unsigned char ZONEGAP[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

int main(int argc, char **argv) {
	int ret_code = EXIT_SUCCESS;
	char inputfile[1024];
	char outputfile[1024] = OUTPREFIX;
	unsigned char buf[255];
	FILE *ifp, *ofp;

	// Check arguments
	if (argc < 2) {
		fprintf(stderr, "Usage: %s inputfile\nThe inputfile should be DirectWave's monolithic dwp file.\n", argv[0]);
		ret_code = EXIT_FAILURE;
		return ret_code;
	}

	// Get input filename
	strncpy(inputfile, argv[1], sizeof(inputfile));
	// Generate output filename
	strcat(outputfile, inputfile);

	// Open file in binary read mode
	ifp = fopen(inputfile, "rb");
	if (ifp == NULL) {
		fprintf(stderr, "File open error: %s\n", inputfile);
		ret_code = EXIT_FAILURE;
		return ret_code;
	}

	// Check the file is whether DirectWave dwp
	fread(buf, sizeof(DWPID) - 1, 1, ifp);
	if (strncmp(buf, DWPID, sizeof(DWPID) - 1) != 0) {
		fprintf(stderr, "Input file %s is not supported file\nThe inputfile should be DirectWave's monolithic dwp file.\n", inputfile);
		ret_code = EXIT_FAILURE;
		return ret_code;
	}

	// Open file in binary write mode
	ofp = fopen(outputfile, "wb");
	if (ofp == NULL) {
		ret_code = EXIT_FAILURE;
		return ret_code;
	}
	// Write DirectWave file id
	fwrite(buf, 1, sizeof(DWPID) - 1, ofp);

	// Read the rest of entire file
	unsigned long fileAddr = sizeof(DWPID) - 1;
	uint8_t dwProgNameLength = 0;
	char dwProgName[256] = "";
	uint8_t progPathLength = 0;
	char progPath[256] = "";
	uint8_t samplePathLength = 0;
	char samplePath[256] = "";

	int c = EOF;
	while ( (c = fgetc(ifp)) != EOF) {
		if (fileAddr == ADDR_PROG_LEN) {
			dwProgNameLength = c;
			printf("DirectWave Program name length: %"PRId8"\n", dwProgNameLength);
			fputc(c, ofp);
		} else if (fileAddr >= ADDR_PROG_NAME && fileAddr < ADDR_PROG_NAME + dwProgNameLength) {
			dwProgName[fileAddr - ADDR_PROG_NAME] = (unsigned char) c;
			if (fileAddr == ADDR_PROG_NAME + dwProgNameLength - 1) {
				printf("DirectWave Program name: %s\n", dwProgName);
			}
			fputc(c, ofp);
		} else if (fileAddr == ADDR_PROG_NAME + dwProgNameLength + ADDR_OFFSET_PROG_PATH_LEN) {
			progPathLength = c;
			printf("Program path length: %"PRId8" (Addr:0x%lX)\n", progPathLength, fileAddr);
			// change to shortest dummy length
			c = 1;
			fputc(c, ofp);
		} else if (fileAddr >= ADDR_PROG_NAME + dwProgNameLength + ADDR_OFFSET_PROG_PATH && fileAddr < ADDR_PROG_NAME + dwProgNameLength + ADDR_OFFSET_PROG_PATH + progPathLength) {
			progPath[fileAddr - (ADDR_PROG_NAME + dwProgNameLength + ADDR_OFFSET_PROG_PATH)] = (unsigned char) c;
			if (fileAddr == ADDR_PROG_NAME + dwProgNameLength + ADDR_OFFSET_PROG_PATH + progPathLength - 1) {
				printf("Program path (will be masked): %s\n", progPath);
			}
			// change to shortest dummy character
			if (fileAddr == ADDR_PROG_NAME + dwProgNameLength + ADDR_OFFSET_PROG_PATH) {
				c = (int) 'X';
				fputc(c, ofp);
			}
		} else if (c == ZONEID) {
			fputc(c, ofp);
			fread(buf, sizeof(ZONEHDR), 1, ifp);
			fwrite(buf, 1, sizeof(ZONEHDR), ofp);
			fileAddr += sizeof(ZONEHDR);
			if (strncmp(buf, ZONEHDR, sizeof(ZONEHDR)) == 0) {
				printf("Zone found!\n");
				if ((c = fgetc(ifp)) != EOF) {
					fileAddr++;
					samplePathLength = (unsigned char) c;
					printf("Sample path length: %"PRId8" (Addr:0x%lX)\n", samplePathLength, fileAddr);
					// Do not change to shortest dummy length, or dwp file will be corrupted
					//c = sizeof(DUMMYSAMPLE) - 1;
					
					fputc(c, ofp);
					fread(buf, sizeof(ZONEGAP), 1, ifp);
					fwrite(buf, 1, sizeof(ZONEGAP), ofp);
					fileAddr += sizeof(ZONEGAP);
					if (strncmp(buf, ZONEGAP, sizeof(ZONEGAP)) == 0) {
						fread(samplePath, samplePathLength, 1, ifp);
						printf("Sample path  (will be masked): %s\n", samplePath);
						c = (int) 'X';
						// Do not change to shortest dummy character, or dwp file will be corrupted
						//fputc(c, ofp);
						
						// Fill the dummy character in the entire length
						for (unsigned short i = 0; i < samplePathLength; i++) {
							fputc(c, ofp);
						}
					} else {
						fprintf(stderr, "Unknown file format.\n");
						ret_code = EXIT_FAILURE;
						return ret_code;
					}
				} else {
					fprintf(stderr, "Unknown file format.\n");
					ret_code = EXIT_FAILURE;
					return ret_code;
				}
			}
		} else {
			fputc(c, ofp);
		}
		fileAddr++;
	}
	fclose(ofp);
	fclose(ifp);
	return ret_code;
}
