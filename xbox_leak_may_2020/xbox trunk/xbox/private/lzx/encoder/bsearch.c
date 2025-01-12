/*
 * bsearch.c
 *
 * Binary search for optimal encoder
 */
#include "encoder.h"


#define left    context->enc_Left
#define right   context->enc_Right


#ifdef __BOUNDSCHECKER__
#include "nmevtrpt.h"
#endif

/*
 * Define this to force checking that all search locations visited
 * are valid.
 *
 * For debugging purposes only.
 */
#ifdef DIAMOND_DEBUG
#   define VERIFY_SEARCHES
#endif

#define VERIFY_SEARCH_CODE(routine_name) \
{ \
	int debug_search; \
	for (debug_search = 0; debug_search < clen; debug_search++) \
	{ \
		if (context->enc_MemWindow[ptr+debug_search] != context->enc_MemWindow[BufPos+debug_search]) \
		{ \
			_RPT2( \
				_CRT_WARN, \
				routine_name \
				" char mismatch @%3d (clen=%d)\n", \
				debug_search, clen); \
			\
			_RPT3( \
				_CRT_WARN, \
				" ptr=%8d, bufpos=%8d, end_pos=%8d\n\n", \
				ptr, BufPos, end_pos); \
		} \
	} \
}

#define VERIFY_MULTI_TREE_SEARCH_CODE(routine_name) \
_ASSERTE (context->enc_MemWindow[BufPos] == context->enc_MemWindow[ptr]); \
_ASSERTE (context->enc_MemWindow[BufPos+1] == context->enc_MemWindow[ptr+1]);



/*
 * Finds the closest matches of all possible lengths, MIN_MATCH <= x <= MAX_MATCH,
 * at position BufPos.
 *
 * The positions of each match location are stored in context->enc_matchpos_table[]
 *
 * Returns the longest such match length found, or zero if no matches found.
 */

#ifndef ASM_BSEARCH_FINDMATCH
long binary_search_findmatch(t_encoder_context *context, long BufPos)
{
	ulong       ptr;
	ulong       a, b;
	ulong       *small_ptr, *big_ptr;
	ulong       end_pos;
	int         val; /* must be signed */
	int         bytes_to_boundary;
	int         clen;
	int         same;
	int         match_length;
	int         small_len, big_len;
	int         i, best_repeated_offset;
#ifdef MULTIPLE_SEARCH_TREES
	ushort      tree_to_use;

#ifdef __BOUNDSCHECKER__
    //
    //  we're going to pickup two bytes of uncompressed data and use them as a direct index
    //  to select a tree root.  If those two bytes just happen to be 0xBF, 0xBF, BC6 will
    //  think bad things are happening.  We don't always disable because we'd still like to
    //  see checks and it's faster to do it only when needed.
    //
    int fReenable = 0;

    if ((context->enc_RealMemWindow[BufPos - SLIDE] == BC_FILL_BYTE) &&
        (context->enc_RealMemWindow[BufPos - SLIDE + 1] == BC_FILL_BYTE))
    {
        fReenable = StopEvtReporting();
    }
#endif

	/*
	 * Retrieve root node of tree to search, and insert current node at
	 * the root.
	 */
#ifdef STRICT_POINTERS
    tree_to_use = *((ushort UNALIGNED *) &context->enc_RealMemWindow[BufPos - SLIDE]);
#else
	tree_to_use = *((ushort UNALIGNED *) &context->enc_MemWindow[BufPos]);
#endif
	
	ptr        = context->enc_tree_root[tree_to_use];
	context->enc_tree_root[tree_to_use] = BufPos;

#ifdef __BOUNDSCHECKER__
    if (fReenable)
    {
        StartEvtReporting();
    }
#endif

#else
	ptr = context->enc_single_tree_root;
	context->enc_single_tree_root = BufPos;
#endif
	/*
	 * end_pos is the furthest location back we will search for matches 
	 *
	 * Remember that our window size is reduced by 3 bytes because of
	 * our repeated offset codes.
	 *
	 * Since BufPos starts at context->enc_window_size when compression begins,
	 * end_pos will never become negative.  
	 */
	end_pos = BufPos - (context->enc_window_size-4);

	/*
	 * Root node is either NULL, or points to a really distant position.
	 */
	if (ptr <= end_pos)
	{
#ifdef STRICT_POINTERS
        context->enc_RealLeft[BufPos - SLIDE] = context->enc_RealRight[BufPos - SLIDE] = 0; //BC6
#else
		left[BufPos] = right[BufPos] = 0;
#endif
		return 0;
	}

#ifdef MULTIPLE_SEARCH_TREES
	/*
	 * confirmed length (no need to check the first clen chars in a search)
	 *
	 * note: clen is always equal to min(small_len, big_len)
	 */
	clen            = 2;

	/*
	 * current best match length
	 */
	match_length    = 2;

	/*
	 * longest match which is < our string
	 */
	small_len       = 2;

	/*
	 * longest match which is > our string
	 */
	big_len         = 2;

	/*
	 * record match position for match length 2
	 */
	context->enc_matchpos_table[2] = BufPos - ptr + 2;

#ifdef VERIFY_SEARCHES
	VERIFY_MULTI_TREE_SEARCH_CODE("binary_search_findmatch()");
#endif

#else /* !MULTIPLE_SEARCH_TREES */

	clen            = 0;
	match_length    = 0;
	small_len       = 0;
	big_len         = 0;

#endif /* MULTIPLE_SEARCH_TREES */

	/*
	 * pointers to nodes to check
	 */
#ifdef STRICT_POINTERS
	small_ptr             = &context->enc_RealLeft[BufPos - SLIDE];     //BC6
	big_ptr               = &context->enc_RealRight[BufPos - SLIDE];    //BC6
#else
	small_ptr             = &left[BufPos];
	big_ptr               = &right[BufPos];
#endif

	do
	{
		/* compare bytes at current node */
		same = clen;

#ifdef VERIFY_SEARCHES
		VERIFY_SEARCH_CODE("binary_search_findmatch()")
#endif

		/* don't need to check first clen characters */
		a    = ptr + clen;
		b    = BufPos + clen;

#ifdef STRICT_POINTERS
		while ((val = ((int) context->enc_RealMemWindow[a++ - SLIDE]) - ((int) context->enc_RealMemWindow[b++ - SLIDE])) == 0)  //BC6
#else
		while ((val = ((int) context->enc_MemWindow[a++]) - ((int) context->enc_MemWindow[b++])) == 0)
#endif
		{
			/* don't exceed MAX_MATCH */
			if (++same >= MAX_MATCH)
				goto long_match;
		}

		if (val < 0)
		{
			if (same > big_len)
			{
				if (same > match_length)
				{
long_match:
					do
					{
						context->enc_matchpos_table[++match_length] = BufPos-ptr+(NUM_REPEATED_OFFSETS-1);
					} while (match_length < same);

					if (same >= BREAK_LENGTH)
					{
#ifdef STRICT_POINTERS
                        *small_ptr = context->enc_RealLeft[ptr - SLIDE];
                        *big_ptr   = context->enc_RealRight[ptr - SLIDE];
#else
						*small_ptr = left[ptr];
						*big_ptr   = right[ptr];
#endif
						goto end_bsearch;
					}
				}

				big_len = same;
				clen = min(small_len, big_len);
			}

			*big_ptr = ptr;
#ifdef STRICT_POINTERS
   			big_ptr  = &context->enc_RealLeft[ptr - SLIDE];
#else
			big_ptr  = &left[ptr];
#endif
			ptr      = *big_ptr;
		}
		else
		{
			if (same > small_len)
			{
				if (same > match_length)
				{
					do
					{
						context->enc_matchpos_table[++match_length] = BufPos-ptr+(NUM_REPEATED_OFFSETS-1);
					} while (match_length < same);

					if (same >= BREAK_LENGTH)
					{
#ifdef STRICT_POINTERS
                        *small_ptr = context->enc_RealLeft[ptr - SLIDE];
                        *big_ptr   = context->enc_RealRight[ptr - SLIDE];
#else
						*small_ptr = left[ptr];
						*big_ptr   = right[ptr];
#endif
						goto end_bsearch;
					}
				}

				small_len = same;
				clen = min(small_len, big_len);
			}
		
			*small_ptr = ptr;
#ifdef STRICT_POINTERS
			small_ptr  = &context->enc_RealRight[ptr - SLIDE];
#else
			small_ptr  = &right[ptr];
#endif
			ptr        = *small_ptr;
		}
	} while (ptr > end_pos); /* while we don't go too far backwards */

	*small_ptr = 0;
	*big_ptr   = 0;


end_bsearch:

	/*
	 * If we have multiple search trees, we are already guaranteed
	 * a minimum match length of 2 when we reach here.
	 *
	 * If we only have one tree, then we're not guaranteed anything.
	 */
#ifndef MULTIPLE_SEARCH_TREES
	if (match_length < MIN_MATCH)
		return 0;
#endif

	/*
	 * Check to see if any of our match lengths can 
	 * use repeated offsets.
	 */

	/*
	 * repeated offset 1 
	 */
	for (i = 0; i < match_length; i++)
	{
#ifdef STRICT_POINTERS
		if (context->enc_RealMemWindow[BufPos+i - SLIDE] != context->enc_RealMemWindow[BufPos-context->enc_last_matchpos_offset[0]+i - SLIDE])
#else
		if (context->enc_MemWindow[BufPos+i] != context->enc_MemWindow[BufPos-context->enc_last_matchpos_offset[0]+i])
#endif
			break;
	}

	/*
	 * the longest repeated offset
	 */
	best_repeated_offset = i;

	if (i >= MIN_MATCH)
	{
		/*
		 * Yes, we can do a repeated offset for some match lengths; replace
		 * their positions with the repeated offset position
		 */
		do
		{
			context->enc_matchpos_table[i] = 0; /* first repeated offset position */
		} while (--i >= MIN_MATCH);

		/* A speed optimization to cope with long runs of bytes */
		if (best_repeated_offset > BREAK_LENGTH)
			goto quick_return;
	}

	/*
	 * repeated offset 2 
	 */
	for (i = 0; i < match_length; i++)
	{
#ifdef STRICT_POINTERS
		if (context->enc_RealMemWindow[BufPos+i - SLIDE] != context->enc_RealMemWindow[BufPos-context->enc_last_matchpos_offset[1]+i - SLIDE])
#else
		if (context->enc_MemWindow[BufPos+i] != context->enc_MemWindow[BufPos-context->enc_last_matchpos_offset[1]+i])
#endif
			break;
	}
		
	/*
	 * Does the second repeated offset provide a longer match?
	 *
	 * If so, leave the first repeated offset alone, but fill out the
	 * difference in match lengths in the table with repeated offset 1.
	 */
	if (i > best_repeated_offset)
	{               
		do
		{
			context->enc_matchpos_table[++best_repeated_offset] = 1;
		} while (best_repeated_offset < i);
	}

	/* 
	 * repeated offset 3 
	 */
	for (i = 0; i < match_length; i++)
	{
#ifdef STRICT_POINTERS
		if (context->enc_RealMemWindow[BufPos+i - SLIDE] != context->enc_RealMemWindow[BufPos-context->enc_last_matchpos_offset[2]+i - SLIDE])
#else
		if (context->enc_MemWindow[BufPos+i] != context->enc_MemWindow[BufPos-context->enc_last_matchpos_offset[2]+i])
#endif
			break;
	}
		
	/*
	 * Does the third repeated offset provide a longer match?
	 */
	if (i > best_repeated_offset)
	{               
		do
		{
			context->enc_matchpos_table[++best_repeated_offset] = 2;
		} while (best_repeated_offset < i);
	}

quick_return:

	/*
	 * Don't let a match cross a 32K boundary
	 */
	bytes_to_boundary = (CHUNK_SIZE-1) - ((int) BufPos & (CHUNK_SIZE-1));

	if (match_length > bytes_to_boundary)
	{
		match_length = bytes_to_boundary;
		
		if (match_length < MIN_MATCH)
			match_length = 0;
	}

	return (long) match_length;
}
#endif


/*
 * Inserts the string at the current BufPos into the tree.
 *
 * Does not record all the best match lengths or otherwise attempt
 * to search for matches
 *
 * Similar to the above function.
 */
#ifndef ASM_QUICK_INSERT_BSEARCH_FINDMATCH
void quick_insert_bsearch_findmatch(t_encoder_context *context, long BufPos, long end_pos)
{
	long        ptr;
	ulong       a,b;
	ulong       *small_ptr, *big_ptr;
	int         val;
	int         small_len, big_len;
	int         same;
	int         clen;
#ifdef MULTIPLE_SEARCH_TREES
	ushort      tree_to_use;

#ifdef __BOUNDSCHECKER__
    //
    //  we're going to pickup two bytes of uncompressed data and use them as a direct index
    //  to select a tree root.  If those two bytes just happen to be 0xBF, 0xBF, BC6 will
    //  think bad things are happening.  We don't always disable because we'd still like to
    //  see checks and it's faster to do it only when needed.
    //
    int fReenable = 0;

    if ((context->enc_RealMemWindow[BufPos - SLIDE] == BC_FILL_BYTE) &&
        (context->enc_RealMemWindow[BufPos - SLIDE + 1] == BC_FILL_BYTE))
    {
        fReenable = StopEvtReporting();
    }
#endif

#ifdef STRICT_POINTERS
	tree_to_use = *((ushort UNALIGNED *) &context->enc_RealMemWindow[BufPos - SLIDE]);
#else
	tree_to_use = *((ushort UNALIGNED *) &context->enc_MemWindow[BufPos]);
#endif
	ptr        = context->enc_tree_root[tree_to_use];
	context->enc_tree_root[tree_to_use] = BufPos;

#ifdef __BOUNDSCHECKER__
    if (fReenable)
    {
        StartEvtReporting();
    }
#endif

#else
	ptr = context->enc_single_tree_root;
	context->enc_single_tree_root = BufPos;
#endif

	if (ptr <= end_pos)
	{
#ifdef STRICT_POINTERS
        context->enc_RealLeft[BufPos - SLIDE] = context->enc_RealRight[BufPos - SLIDE] = 0;
#else
		left[BufPos] = right[BufPos] = 0;
#endif
		return;
	}

#ifdef MULTIPLE_SEARCH_TREES
	clen            = 2;
	small_len       = 2;
	big_len         = 2;

#ifdef VERIFY_SEARCHES
	VERIFY_MULTI_TREE_SEARCH_CODE("quick_insert_bsearch_findmatch()");
#endif

#else
	clen            = 0;
	small_len       = 0;
	big_len         = 0;
#endif

#ifdef STRICT_POINTERS
    small_ptr       = &context->enc_RealLeft[BufPos - SLIDE];
    big_ptr         = &context->enc_RealRight[BufPos - SLIDE];
#else
	small_ptr       = &left[BufPos];
	big_ptr         = &right[BufPos];
#endif

	do
	{
		_ASSERTE ((ulong) ptr >= (ulong) (context->enc_RealLeft - context->enc_Left));

		same = clen;

		a    = ptr+clen;
		b    = BufPos+clen;

#ifdef VERIFY_SEARCHES
		VERIFY_SEARCH_CODE("quick_insert_bsearch_findmatch()")
#endif

#ifdef STRICT_POINTERS
		while ((val = ((int) context->enc_RealMemWindow[a++ - SLIDE]) - ((int) context->enc_RealMemWindow[b++ - SLIDE])) == 0)
#else
		while ((val = ((int) context->enc_MemWindow[a++]) - ((int) context->enc_MemWindow[b++])) == 0)
#endif
		{
			/*
			 * Here we break on BREAK_LENGTH, not MAX_MATCH
			 */
			if (++same >= BREAK_LENGTH) 
				break;
		}

		if (val < 0)
		{
			if (same > big_len)
			{
				if (same >= BREAK_LENGTH)
				{
#ifdef STRICT_POINTERS
                    *small_ptr = context->enc_RealLeft[ptr - SLIDE];
                    *big_ptr = context->enc_RealRight[ptr - SLIDE];
#else
					*small_ptr = left[ptr];
					*big_ptr = right[ptr];
#endif
					return;
				}

				big_len = same;
				clen = min(small_len, big_len);
			}
			
			*big_ptr = ptr;
#ifdef STRICT_POINTERS
            big_ptr  = &context->enc_RealLeft[ptr - SLIDE];
#else
			big_ptr  = &left[ptr];
#endif
			ptr      = *big_ptr;
		}
		else
		{
			if (same > small_len)
			{
				if (same >= BREAK_LENGTH)
				{
#ifdef STRICT_POINTERS
                    *small_ptr = context->enc_RealLeft[ptr - SLIDE];
                    *big_ptr = context->enc_RealRight[ptr - SLIDE];
#else
					*small_ptr = left[ptr];
					*big_ptr = right[ptr];
#endif
					return;
				}

				small_len = same;
				clen = min(small_len, big_len);
			}

			*small_ptr = ptr;
#ifdef STRICT_POINTERS
            small_ptr  = &context->enc_RealRight[ptr - SLIDE];
#else
			small_ptr  = &right[ptr];
#endif
			ptr        = *small_ptr;
		}
   } while (ptr > end_pos);

	*small_ptr = 0;
	*big_ptr   = 0;
}
#endif


/*
 * Remove a node from the search tree; this is ONLY done for the last
 * BREAK_LENGTH symbols (see optenc.c).  This is because we will have
 * inserted strings that contain undefined data (e.g. we're at the 4th
 * last byte from the file and binary_search_findmatch() a string into
 * the tree - everything from the 4th symbol onwards is invalid, and
 * would cause problems if it remained in the tree, so we have to
 * remove it).
 */
void binary_search_remove_node(t_encoder_context *context, long BufPos, ulong end_pos)
{
	ulong   ptr;
	ulong   left_node_pos;
	ulong   right_node_pos;
	ulong   *link;
#ifdef MULTIPLE_SEARCH_TREES
	ushort  tree_to_use;

#ifdef __BOUNDSCHECKER__
    //
    //  we're going to pickup two bytes of uncompressed data and use them as a direct index
    //  to select a tree root.  If those two bytes just happen to be 0xBF, 0xBF, BC6 will
    //  think bad things are happening.  We don't always disable because we'd still like to
    //  see checks and it's faster to do it only when needed.
    //
    int fReenable = 0;

    if ((context->enc_RealMemWindow[BufPos - SLIDE] == BC_FILL_BYTE) &&
        (context->enc_RealMemWindow[BufPos - SLIDE + 1] == BC_FILL_BYTE))
    {
        fReenable = StopEvtReporting();
    }
#endif

	/*
	 * The root node of tree_to_use should equal BufPos, since that is
	 * the most recent insertion into that tree - but if we never
	 * inserted this string (because it was a near match or a long
	 * string of zeroes), then we can't remove it.
	 */
#ifdef STRICT_POINTERS
	tree_to_use = *((ushort UNALIGNED *) &context->enc_RealMemWindow[BufPos - SLIDE]);
#else
	tree_to_use = *((ushort UNALIGNED *) &context->enc_MemWindow[BufPos]);
#endif


	/*
	 * If we never inserted this string, do not attempt to remove it
	 */

	if (context->enc_tree_root[tree_to_use] != (ulong) BufPos)
    {
#ifdef __BOUNDSCHECKER__
        if (fReenable)
        {
            StartEvtReporting();
        }
#endif

		return;
    }

	link = &context->enc_tree_root[tree_to_use];

#ifdef __BOUNDSCHECKER__
    if (fReenable)
    {
        StartEvtReporting();
    }
#endif

#else
	if (context->enc_single_tree_root != (ulong) BufPos)
		return;

	link = &context->enc_single_tree_root;
#endif

	/*
	 * If the last occurence was too far away
	 */
	if (*link <= end_pos)
	{
		*link = 0;
#ifdef STRICT_POINTERS
        context->enc_RealLeft[BufPos - SLIDE] = context->enc_RealRight[BufPos - SLIDE] = 0;
#else
		left[BufPos] = right[BufPos] = 0;
#endif
		return;
	}

	/*
	 * Most recent location of these chars
	 */
	ptr             = BufPos;

	/*
	 * Most recent location of a string which is "less than" it
	 */
#ifdef STRICT_POINTERS
    left_node_pos   = context->enc_RealLeft[ptr - SLIDE];
#else
	left_node_pos   = left[ptr];
#endif

	if (left_node_pos <= end_pos)
#ifdef STRICT_POINTERS
        left_node_pos = context->enc_RealLeft[ptr - SLIDE];
#else
		left_node_pos = left[ptr] = 0;
#endif

	/*
	 * Most recent location of a string which is "greater than" it
	 */
#ifdef STRICT_POINTERS
    right_node_pos  = context->enc_RealRight[ptr - SLIDE];
#else
	right_node_pos  = right[ptr];
#endif

	if (right_node_pos <= end_pos)
#ifdef STRICT_POINTERS
        right_node_pos = context->enc_RealRight[ptr - SLIDE] = 0;
#else
		right_node_pos = right[ptr] = 0;
#endif

	while (1)
	{
#ifdef VERIFY_SEARCHES
		_ASSERTE (left_node_pos < (ulong) BufPos);
		_ASSERTE (right_node_pos < (ulong) BufPos);
#endif

		/*
		 * If left node position is greater than right node position
		 * then follow the left node, since that is the more recent
		 * insertion into the tree.  Otherwise follow the right node.
		 */
		if (left_node_pos > right_node_pos)
		{
			/*
			 * If it's too far away, then store that it never happened
			 */
			if (left_node_pos <= end_pos)
				left_node_pos = 0;

			ptr = *link = left_node_pos;

			if (!ptr)
				break;

#ifdef STRICT_POINTERS
            left_node_pos   = context->enc_RealRight[ptr - SLIDE];
            link            = &context->enc_RealRight[ptr - SLIDE];
#else
			left_node_pos   = right[ptr];
			link            = &right[ptr];
#endif
		}
		else
		{
			/*
			 * If it's too far away, then store that it never happened
			 */
			if (right_node_pos <= end_pos)
				right_node_pos = 0;

			ptr = *link = right_node_pos;

			if (!ptr) 
				break;

#ifdef STRICT_POINTERS
            right_node_pos  = context->enc_RealLeft[ptr - SLIDE];
            link            = &context->enc_RealLeft[ptr - SLIDE];
#else
			right_node_pos  = left[ptr];
			link            = &left[ptr];
#endif
		}
	}
}
