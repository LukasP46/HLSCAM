/**
 * @file access_memory.cpp
 * @brief Implementation of the top-level CAM access function(s) for HLSCAM.
 *
 * Each @c access_cam() overload is the HLS top-level function synthesised into
 * an FPGA kernel.  The active variant is selected by the preprocessor macros
 * defined in config.h:
 *
 *  CAM type  | Macro     | Description
 *  --------- | --------- | -----------------------------------------
 *  BCAM      | USE_BCAM  | Binary (exact-match) CAM
 *  TCAM      | USE_TCAM  | Ternary (wildcard) CAM
 *  STCAM     | USE_STCAM | Semi-TCAM (longest-prefix match)
 *
 *  Mode      | Macro | HLS directives applied
 *  --------- | ----- | ---------------------------------------------------
 *  Brute Force | BF  | LUTRAM storage, no array partitioning
 *  Balanced    | BL  | PIPELINE + cyclic factor-16 partitioning + LUTRAM
 *  High Speed  | HS  | PIPELINE + complete partitioning + AGGREGATE
 *  HS Hierarch.| HS_H| PIPELINE + complete partitioning + compile-time tree
 */

#include "access_memory.h"
#include "bcam.h"
#include "tcam.h"
#include "stcam.h"

#if defined(USE_BCAM) // -------- BCAM --------
static BCAM<CAM_KEY_WIDTH, CAM_VALUE_WIDTH, CAM_DEPTH> bcam;

#if defined(HS_H)

/**
 * @brief BCAM access kernel for the HS_H (High-Speed Hierarchical) mode.
 *
 * In write mode, the entry is inserted using dummy_add (direct-index write).
 * In read mode, a compile-time binary-tree search (BinaryMatchIndex) locates
 * the matching entry without iterating linearly, maximising clock frequency.
 *
 * HLS directives applied:
 *  - PIPELINE       – single-cycle II target
 *  - ARRAY_PARTITION complete – all entries accessible in one cycle
 *  - AGGREGATE      – pack the Entry struct fields into a single wide word
 *
 * @param[in]     key   Key to insert or search.
 * @param[in,out] value Value to write (write mode) or value read out (read mode).
 * @param[out]    index Slot index of the match, or @c -1 on a lookup miss.
 * @param[in]     write @c true = insert; @c false = lookup.
 * @return @c true if the operation succeeded, @c false on a lookup miss.
 */
bool access_cam(const ap_uint<CAM_KEY_WIDTH> &key, ap_uint<CAM_VALUE_WIDTH> &value, int &index, bool write)
{
#pragma HLS PIPELINE
#pragma HLS ARRAY_PARTITION variable=bcam.entries complete dim=1
#pragma HLS AGGREGATE variable=bcam.entries

	if (write) {
		bcam.dummy_add(key, value);
		index = 0;
	}
	else {
		index = BCAM<CAM_KEY_WIDTH, CAM_VALUE_WIDTH, CAM_DEPTH>::BinaryMatchIndex<CAM_KEY_WIDTH, CAM_VALUE_WIDTH, CAM_DEPTH, 0, CAM_DEPTH - 1>::compute(bcam, key);
	}
	return (index != -1);
}

#else

/**
 * @brief BCAM access kernel for the standard synthesis modes (BF, BL, HS).
 *
 * In write mode, the entry is inserted using dummy_add (direct-index write).
 * In read mode, search_entry() performs a parallel exact-key comparison across
 * all valid entries.
 *
 * HLS directives depend on the active mode macro:
 *  - @b BF  : LUTRAM storage, no array partitioning (lowest area).
 *  - @b HS  : PIPELINE + complete partitioning + AGGREGATE (highest speed).
 *  - @b BL  : PIPELINE + cyclic factor-16 partitioning + LUTRAM (balanced).
 *
 * @param[in]     key   Key to insert or search.
 * @param[in,out] value Value to write (write mode) or value read out (read mode).
 * @param[in]     write @c true = insert; @c false = lookup.
 * @return @c true if the operation succeeded, @c false on a lookup miss.
 */
bool access_cam(const ap_uint<CAM_KEY_WIDTH> &key, ap_uint<CAM_VALUE_WIDTH> &value, bool write)
{
#if defined(BF)
#pragma HLS bind_storage variable=bcam.entries type=ram_2p impl=lutram
#pragma HLS ARRAY_PARTITION variable=bcam.entries off

#elif defined(HS)
#pragma HLS PIPELINE
#pragma HLS ARRAY_PARTITION variable=bcam.entries complete dim=1
#pragma HLS AGGREGATE variable=bcam.entries

#elif defined(BL)
#pragma HLS PIPELINE
#pragma HLS ARRAY_PARTITION variable=bcam.entries type=cyclic factor=16 dim=1
#pragma HLS bind_storage variable=bcam.entries type=ram_2p impl=lutram
#pragma HLS AGGREGATE variable=bcam.entries

#elif defined(HASH_KEY)
#pragma HLS PIPELINE
#pragma HLS bind_storage variable=bcam.entries type=ram_2p impl=lutram

#endif

	bool flag = false;
	if (write) {
		bcam.dummy_add(key, value);
		flag = true;
	}
	else {
		flag = bcam.search_entry(key, value);
	}
	return flag;
}

#endif

#elif defined(USE_TCAM)  // -------- TCAM --------
static TCAM<CAM_KEY_WIDTH, CAM_VALUE_WIDTH, CAM_DEPTH> tcam;

#if defined(HS_H)

/**
 * @brief TCAM access kernel for the HS_H (High-Speed Hierarchical) mode.
 *
 * In write mode, the entry is inserted using dummy_add (direct-index write).
 * In read mode, a compile-time binary-tree ternary search (TernaryMatchIndex)
 * locates the matching entry, maximising clock frequency.
 *
 * HLS directives applied:
 *  - PIPELINE       – single-cycle II target
 *  - ARRAY_PARTITION complete – all entries accessible in one cycle
 *  - AGGREGATE      – pack the Entry struct fields into a single wide word
 *
 * @param[in]     key   Key to insert or search.
 * @param[in]     mask  Care-bit mask used during insert and ternary search.
 * @param[in,out] value Value to write (write mode) or value read out (read mode).
 * @param[out]    index Slot index of the match, or @c -1 on a lookup miss.
 * @param[in]     write @c true = insert; @c false = lookup.
 * @return @c true if the operation succeeded, @c false on a lookup miss.
 */
bool access_cam(const ap_uint<CAM_KEY_WIDTH> &key, const ap_uint<CAM_KEY_WIDTH> mask, ap_uint<CAM_VALUE_WIDTH> &value, int &index, bool write)
{
#pragma HLS PIPELINE
#pragma HLS ARRAY_PARTITION variable=tcam.entries complete dim=1
#pragma HLS AGGREGATE variable=tcam.entries

	if (write) {
		tcam.dummy_add(key, mask, value);
		index = 0;
	}
	else {
		index = TCAM<CAM_KEY_WIDTH, CAM_VALUE_WIDTH, CAM_DEPTH>::TernaryMatchIndex<CAM_KEY_WIDTH, CAM_VALUE_WIDTH, CAM_DEPTH, 0, CAM_DEPTH - 1>::compute(tcam, key);
	}
	return (index != -1);
}

#else

/**
 * @brief TCAM access kernel for the standard synthesis modes (BF, BL, HS).
 *
 * In write mode, the entry is inserted using dummy_add (direct-index write).
 * In read mode, search_entry() performs a parallel ternary comparison across
 * all valid entries.
 *
 * HLS directives depend on the active mode macro:
 *  - @b BF : LUTRAM storage, no array partitioning (lowest area).
 *  - @b HS : PIPELINE + complete partitioning + AGGREGATE (highest speed).
 *  - @b BL : PIPELINE + cyclic factor-16 partitioning + LUTRAM (balanced).
 *
 * @param[in]     key   Key to insert or search.
 * @param[in]     mask  Care-bit mask used during insert and ternary search.
 * @param[in,out] value Value to write (write mode) or value read out (read mode).
 * @param[in]     write @c true = insert; @c false = lookup.
 * @return @c true if the operation succeeded, @c false on a lookup miss.
 */
bool access_cam(const ap_uint<CAM_KEY_WIDTH> &key, const ap_uint<CAM_KEY_WIDTH> mask, ap_uint<CAM_VALUE_WIDTH> &value, bool write)
{
#if defined(BF)
#pragma HLS bind_storage variable=tcam.entries type=ram_2p impl=lutram
#pragma HLS ARRAY_PARTITION variable=tcam.entries off

#elif defined(HS)
#pragma HLS PIPELINE
#pragma HLS ARRAY_PARTITION variable=tcam.entries complete dim=1
#pragma HLS AGGREGATE variable=tcam.entries

#elif defined(BL)
#pragma HLS PIPELINE
#pragma HLS ARRAY_PARTITION variable=tcam.entries type=cyclic factor=16 dim=1
#pragma HLS bind_storage variable=tcam.entries type=ram_2p impl=lutram
#pragma HLS AGGREGATE variable=tcam.entries

#endif

	bool flag = false;
	if (write) {
		tcam.dummy_add(key, mask, value);
		flag = true;
	}
	else {
		flag = tcam.search_entry(key, value);
	}
	return flag;
}

#endif

// -------------------------------------------------------------------------------------------------------------------------

#elif defined(USE_STCAM) // -------- STCAM --------
static STCAM<CAM_KEY_WIDTH, CAM_VALUE_WIDTH, CAM_DEPTH> stcam;

/**
 * @brief STCAM access kernel for the standard synthesis modes (BF, BL, HS).
 *
 * In write mode the entry is inserted using dummy_add, using the lower 8 bits
 * of @p key as the mask_shift (i.e., the number of don't-care LSBs).
 * In read mode, search_entry() performs a Longest Prefix Match across all
 * valid entries.
 *
 * HLS directives depend on the active mode macro:
 *  - @b BF : LUTRAM storage, no array partitioning (lowest area).
 *  - @b HS : PIPELINE + complete partitioning (highest speed).
 *  - @b BL : PIPELINE + cyclic factor-2 partitioning + LUTRAM + AGGREGATE.
 *
 * @param[in]  key   Key to insert or search.
 * @param[out] value Set to the matched entry's value when found.
 * @param[in]  write @c true = insert; @c false = LPM lookup.
 * @return @c true if a matching valid entry was found, @c false otherwise.
 */
bool access_cam(const ap_uint<CAM_KEY_WIDTH> &key, ap_uint<CAM_VALUE_WIDTH> &value, bool write)
{
#if defined(BF)
#pragma HLS bind_storage variable=stcam.entries type=ram_2p impl=lutram
#pragma HLS ARRAY_PARTITION variable=stcam.entries off

#elif defined(HS)
#pragma HLS PIPELINE
//#pragma HLS bind_storage variable=stcam.entries type=ram_2p impl=lutram
#pragma HLS ARRAY_PARTITION variable=stcam.entries complete dim=1

#elif defined(BL)
#pragma HLS PIPELINE
#pragma HLS ARRAY_PARTITION variable=stcam.entries type=cyclic factor=2 dim=1
#pragma HLS bind_storage variable=stcam.entries type=ram_2p impl=lutram
#pragma HLS AGGREGATE variable=stcam.entries

#endif

	bool flag = false;
	if (write) {
		stcam.dummy_add(key, key.range(7, 0), value);
		flag = true;
	}
	else {
		flag = stcam.search_entry(key, value);
	}
	return flag;
}

#endif
