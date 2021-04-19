/*
 * "Finger" a DMK to get a set of basic info.
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include "libdmk.h"


unsigned int
sector_size(sector_mode_t encoding, int sizecode)
{
	if ((encoding == DMK_MFM) || !(sizecode & ~0x3)) {
		return 128 << (sizecode & 0x3);
	} else {
		return 16 * (sizecode ? sizecode : 256);
	}
}


void
dump_sector_data(const uint8_t *data, unsigned int data_size)
{
	unsigned int cols = 16;
	unsigned int hcols = cols / 2;

	for (int i = 0; i < data_size; ++i) {
		// Print out indented byte columns in hex.  Put an extra
		// space in the middle of the columns.
		unsigned int si = (i % cols) ? (8 - !(i % hcols)) : !i;

		printf("\n        %02x" + si, data[i]);

		if (!((i+1) % cols)) {
			printf("    ");
			for (int j = 0; j < cols; ++j) {
				char c = data[i+1-cols+j];
				printf("%c", isprint(c) ? c : '.');
			}
		}
	}
	printf("\n");
}


int
dump_track(struct dmk_state *dmkst, int track, int side)
{
	printf("Track %d, side %d", track, side);

	if (dmk_seek(dmkst, track, side)) {
		printf(":\n");
	} else {
		printf(": [seek error]\n");
		return 1;
	}

	int		sector;
	sector_info_t	si;

	for (sector = 0;
	     (sector < DMK_MAX_SECTOR) && dmk_read_id(dmkst, &si); ++sector) {
		uint16_t actual_crc, computed_crc;
		unsigned int data_size = sector_size(si.mode, si.size_code);

		printf("    cyl=%02x side=%02x sec=%02x size=%02x (%u)",
			si.cylinder, si.head, si.sector, si.size_code,
			data_size);

		uint8_t	*data = malloc((size_t)data_size);

		if (!data) {
			printf("\n");
			fprintf(stderr,
				"Failed to allocate %u bytes for "
				"sector buffer.\n", data_size);
			exit(1);
		}

		if (dmk_read_sector_with_crcs(dmkst, &si, data,
			&actual_crc, &computed_crc)) {
			printf(" crc=%04x\n", actual_crc);

			if (si.cylinder == 0 && si.head == 0 && si.sector == 0)
				dump_sector_data(data, data_size);
		} else {
			printf(" actual_crc=%04x computed_crc=%04x\n",
			       actual_crc, computed_crc);
			printf("        Failed to read sector data.\n");
		}

		free(data);
	}

	printf("    Sectors found: %d\n", sector);
	return 0;
}


int
main(int argc, char **argv)
{
	char	*fn;
	struct dmk_state *dmkst;
	int	tracks, ds, dd;

	if (argc < 2) {
		fprintf(stderr, "Must provide DMK file name argument.\n");
		return 1;
	}

	fn = argv[1];

	dmkst = dmk_open_image(fn, 0, &ds, &tracks, &dd);
	if (!dmkst) {
		fprintf(stderr, "Failed to open '%s' (%d [%s]).\n",
			fn, errno, strerror(errno));
		return 2;
	}

	printf("%s: %d tracks [%s-sided/%s-density]\n",
		fn, tracks,
		ds ? "double" : "single",
		dd ? "double" : "single");

	for (int t = 0; t < tracks; ++t) {
		for (int s = 0; s <= ds; ++s)
			dump_track(dmkst, t, s);
	}

	if (!dmk_close_image(dmkst)) {
		fprintf(stderr, "Close of '%s' failed.\n", fn);
		return 1;
	}

	return 0;
}
