/**
 * @file stcam.h
 * @brief Semi-TCAM (STCAM) template class for Longest Prefix Match (LPM).
 *
 * An STCAM is a specialised TCAM variant where masks are restricted to
 * contiguous prefix masks (i.e., masks of the form 1...10...0).  This
 * constraint allows the mask to be stored as a single shift value rather
 * than a full-width bitmask, reducing memory usage while still supporting
 * Longest Prefix Match (LPM) semantics.
 *
 * When multiple entries match the search key, the entry with the smallest
 * @c mask_shift (i.e., the most specific / longest prefix) wins.
 *
 * Supported synthesis strategies (via macros in config.h):
 *  - @b BF (Brute Force)  – sequential scan, stored in LUTRAM.
 *  - @b BL (Balanced)     – pipelined with cyclic partitioning.
 *  - @b HS (High Speed)   – pipelined with full partitioning.
 *
 * @note STCAM is not investigated in the referenced paper; it is provided as
 *       an additional utility for prefix-match workloads.
 */

#ifndef STCAM_H
#define STCAM_H

#include <ap_int.h>
#include "array_util.h"

/**
 * @brief Semi-TCAM (prefix-match / LPM) template.
 *
 * @tparam KEY_WIDTH   Bit-width of each lookup key.
 * @tparam VALUE_WIDTH Bit-width of the value stored alongside each entry.
 * @tparam DEPTH       Maximum number of entries in the table.
 */
template <int KEY_WIDTH, int VALUE_WIDTH, int DEPTH>
class STCAM {
public:
    /**
     * @brief A single STCAM entry holding a key, a prefix-length shift, a
     *        value, and a validity flag.
     */
    struct Entry {
        ap_uint<KEY_WIDTH>   key;        /**< Stored lookup key (network prefix). */
        ap_uint<8>           mask_shift; /**< Number of least-significant bits to ignore
                                              (KEY_WIDTH − prefix_length). */
        ap_uint<VALUE_WIDTH> value;      /**< Value associated with the prefix. */
        bool                 valid;      /**< True when this slot is occupied. */
    };

    /** @brief Storage array for all CAM entries. */
    volatile Entry entries[DEPTH];

    /**
     * @brief Default constructor – entry array is left uninitialised.
     */
    STCAM() {
    }

    /**
     * @brief Writes a key–mask_shift–value triple directly to the slot
     *        addressed by the numeric value of @p key (used for testing /
     *        initialisation only).
     *
     * @param[in] key        Key whose integer value is used as the array index.
     * @param[in] mask_shift Number of least-significant bits to ignore.
     * @param[in] value      Value to store.
     */
    void dummy_add(const ap_uint<KEY_WIDTH> &key, const ap_uint<8> &mask_shift, const ap_uint<VALUE_WIDTH> &value) {
    	const int i = key.to_uint();
        entries[i].key = key;
        entries[i].mask_shift = mask_shift;
        entries[i].value = value;
        entries[i].valid = true;
    }

    /**
     * @brief Inserts a new key–mask_shift–value entry into the first available
     *        (invalid) slot in the table.
     *
     * All slots are checked in parallel (fully unrolled) so the operation
     * completes in a single clock cycle when pipelined.  If the table is full
     * the insertion is silently dropped.
     *
     * @param[in] key        Prefix key to insert.
     * @param[in] mask_shift Number of least-significant bits to ignore
     *                       (KEY_WIDTH − prefix_length).
     * @param[in] value      Associated value.
     */
    void add_entry(const ap_uint<KEY_WIDTH> &key, const ap_uint<8> &mask_shift, const ap_uint<VALUE_WIDTH> &value) {
#pragma HLS INLINE
        for (int i = 0; i < DEPTH; i++) {
#pragma HLS UNROLL
            if (!entries[i].valid) {
                entries[i].key = key;
                entries[i].mask_shift = mask_shift;
                entries[i].value = value;
                entries[i].valid = true;
                break;
            }
        }
    }

    /**
     * @brief Invalidates the first entry whose key and mask_shift match the
     *        supplied arguments.
     *
     * @param[in] key        Key identifying the entry to remove.
     * @param[in] mask_shift Shift value identifying the entry to remove.
     */
    void remove_entry(const ap_uint<KEY_WIDTH> &key, const ap_uint<8> &mask_shift) {
#pragma HLS INLINE
        for (int i = 0; i < DEPTH; i++) {
            #pragma HLS UNROLL
            if (entries[i].valid && ap_uint<KEY_WIDTH>(entries[i].key) == key && ap_uint<8>(entries[i].mask_shift) == mask_shift) {
                entries[i].valid = false;
                break;
            }
        }
    }

    /**
     * @brief Performs a Longest Prefix Match (LPM) search for @p key.
     *
     * Each entry is compared by shifting both the search key and the stored
     * key right by @c mask_shift bits and checking equality.  The entry with
     * the smallest (i.e., most specific) @c mask_shift among all matches is
     * returned via MinIndex.
     *
     * The search strategy depends on the active synthesis mode:
     * - @b BF: inlined sequential scan (lowest area, highest latency).
     * - @b BL / @b HS: pipelined parallel scan over all entries.
     *
     * @param[in]  key   Key to look up.
     * @param[out] value Set to the best-matching entry's value when found.
     * @return @c true if at least one matching valid entry was found,
     *         @c false otherwise.
     */
    bool search_entry(const ap_uint<KEY_WIDTH> &key, ap_uint<VALUE_WIDTH> &value) {
#if defined(BF)
#pragma HLS INLINE
#else
#pragma HLS PIPELINE
#endif
        ap_uint<8> least_masked = KEY_WIDTH + 1;
        bool found = false;

        ap_uint<8> masked[DEPTH] = {least_masked};
#pragma HLS ARRAY_PARTITION variable=masked complete dim=1

        for (int i = 0; i < DEPTH; i++) {
//#pragma HLS PIPELINE
#pragma HLS UNROLL
            if (entries[i].valid) {
                const ap_uint<KEY_WIDTH> shifted_key = key >> ap_uint<8>(entries[i].mask_shift);
                const ap_uint<KEY_WIDTH> shifted_stored_key = ap_uint<KEY_WIDTH>(entries[i].key) >> ap_uint<8>(entries[i].mask_shift);

                masked[i] = (shifted_key == shifted_stored_key) ? ap_uint<8>(entries[i].mask_shift) : ap_uint<8>(KEY_WIDTH + 1);
            }
        }

        int index = MinIndex<ap_uint<8>, 0, DEPTH - 1>::compute(masked, least_masked);
        if (least_masked < KEY_WIDTH + 1) {
        	value = entries[index].value;
        	found = true;
        }
        return found;
    }

};

#endif // STCAM_H
