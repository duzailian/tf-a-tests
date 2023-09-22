#ifndef __TRANSFER_LIST_H
#define __TRANSFER_LIST_H

#define	TRANSFER_LIST_SIGNATURE		U(0x006ed0ff)

struct transfer_list_header {
	uint32_t	signature;
	uint8_t		checksum;
	uint8_t		version;
	uint8_t		hdr_size;
	uint8_t		alignment;	// max alignment of TE data
	uint32_t	size;		// TL header + all TEs
	uint32_t	max_size;
	/*
	 * Commented out element used to visualize dynamic part of the
	 * data structure.
	 *
	 * Note that struct transfer_list_entry also is dynamic in size
	 * so the elements can't be indexed directly but instead must be
	 * traversed in order
	 *
	 * struct transfer_list_entry entries[];
	 */
};


struct transfer_list_entry {
	uint16_t	tag_id;
	uint8_t		reserved0;	// place holder
	uint8_t		hdr_size;
	uint32_t	data_size;
	/*
	 * Commented out element used to visualize dynamic part of the
	 * data structure.
	 *
	 * Note that padding is added at the end of @data to make to reach
	 * a 8-byte boundary.
	 *
	 * uint8_t	data[ROUNDUP(data_size, 8)];
	 */
};
#endif /*__TRANSFER_LIST_H*/
