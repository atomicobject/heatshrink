#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include "../heatshrink_decoder.h"

void poll_data(heatshrink_decoder *hsd, int fddest) {
	HSD_poll_res pres;
	do {
		uint8_t out_buf[512];
		size_t poll_sz;
		pres = heatshrink_decoder_poll(hsd, out_buf, 512, &poll_sz);
		if (pres < 0) exit(1);
		write(fddest, out_buf, poll_sz);
	} while (pres == HSDR_POLL_MORE);
}


int main(int argc, char *argv[]) {

	if (argc < 2) {
		printf("%s compressed_file\n", argv[0]);
		return 1;
	}

	char *origfname = argv[1];

	// open compressed filename
	int fdorig = open(origfname, O_RDONLY);
	if (fdorig == -1) {
		printf("Error opening %s: %s\n", origfname, strerror(errno));
		return 1;
	}

	// create destination filename: origfname.unz
	char destfname[512];
	snprintf(destfname, 512, "%s.unz", origfname);
	int fddest = open(destfname, O_WRONLY | O_TRUNC | O_CREAT, 0644);
	if (fddest == -1) {
		printf("Error opening %s: %s\n", destfname, strerror(errno));
		return 1;
	}

	// alloc decoder using 8,4
    heatshrink_decoder *hsd = heatshrink_decoder_alloc(512, 8, 4);

	// read input file in 512 bytes blocks
	uint8_t buff[512];
	while(1) {
		int br = read(fdorig, buff, 512);
		if (br < 0) {
			printf("Error reading from %s: %s\n", origfname, strerror(errno));
			return 1;
		} else if (br == 0) // end of file
			break;

		size_t sunk = 0;
		do {
			// loop until buff is entire consumed, pushing data to decompression
			size_t consumed = 0;
			HSD_sink_res res = heatshrink_decoder_sink(hsd, &buff[sunk], br-sunk, &consumed);
			if (res  < 0) {
				printf("Decompressing error %d\n", res);
				return 1;
			}
			sunk += consumed;

			// retrieve uncompressed data
			poll_data(hsd, fddest);

		} while (sunk < br);
	}

	// tell decoder that's all
	HSD_finish_res fres = heatshrink_decoder_finish(hsd);

	// retrieve the remaining compressed data
	if (fres == HSDR_FINISH_MORE)
		poll_data(hsd, fddest);

	// cleanup
	heatshrink_decoder_free(hsd);
	close(fdorig);
	close(fddest);
}


