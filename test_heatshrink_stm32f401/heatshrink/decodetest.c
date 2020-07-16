/** Heatshrink decoder test
 * 
 * Decodes a buffer encoded with heatshrink application and calculates its md5 checksum.
 * Note: encoding parameters must match with Heatshrink configuration: Wbits=8, Lbits=4
 */

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "heatshrink_common.h"
#include "heatshrink_config.h"
#include "heatshrink_decoder.h"
#include "decodetest.h"
#include "greatest.h"
#include "md5.h"

greatest_run_info greatest_info;

static heatshrink_decoder hsd;
uint8_t decomp[32768];

int decodeTest(uint8_t* comp, unsigned int compressed_size)
{
	uint32_t sunk = 0;
	size_t count = 0;
	uint32_t polled = 0;
	uint32_t decomp_sz = sizeof(decomp) + sizeof(decomp)/2 + 4;
	int log_lvl = 0;

	memset(decomp, 0xea, sizeof(decomp));

	printf("Compressed size: %i\n", compressed_size);
	printf("Input buffer md5: %s\n", md5sum((char*)comp, compressed_size));

	heatshrink_decoder_reset(&hsd);

	while (sunk < compressed_size) {
		ASSERT(heatshrink_decoder_sink(&hsd, &comp[sunk], compressed_size - sunk, &count) >= 0);
    sunk += count;
    if (log_lvl > 1) printf("^^ sunk %zd total %zd\n", count, sunk);
			if (sunk == compressed_size) {
				ASSERT_EQ(HSDR_FINISH_MORE, heatshrink_decoder_finish(&hsd));
    }

    HSD_poll_res pres;
    do{
			pres = heatshrink_decoder_poll(&hsd, &decomp[polled], decomp_sz - polled, &count);
      ASSERT(pres >= 0);
      polled += count;
      if (log_lvl > 1) printf("^^ polled %zd total %zd\n", count, polled);
    } while (pres == HSDR_POLL_MORE);
    ASSERT_EQ(HSDR_POLL_EMPTY, pres);
    if (sunk == compressed_size) {
			HSD_finish_res fres = heatshrink_decoder_finish(&hsd);
      ASSERT_EQ(HSDR_FINISH_DONE, fres);
    }

    if (polled > sizeof(decomp)) {
			FAILm("Decompressed data is larger than original input");
    }
	}

	/* Print out statistics and result hash */
	printf("Sunk %i polled %i count %i\n", sunk, polled, count);
	printf("Output md5: %s\n", md5sum((char*)decomp, sizeof(decomp)));

	return 0;
}
