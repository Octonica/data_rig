/* contrib/data_rig/data_rig.h */


typedef struct FACT
{
	/* varlena header (do not touch directly!) */
	int32		vl_len_;

	/*----------
	 * Header contains info about FACT. 
	 *
	 * Following information is stored:
	 *
	 *	bits 0-7  : number of fact dimensions;
	 *----------
	 */
	unsigned int header;

	/*
	 * The lower left coordinates for each dimension come first, followed by
	 * upper right coordinates unless the point flag is set.
	 */
	int32_t		x[FLEXIBLE_ARRAY_MEMBER];
} FACT;

