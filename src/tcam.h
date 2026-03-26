/**
 * @file tcam.h
 * @brief Ternary Content Addressable Memory (TCAM) template class.
 *
 * A TCAM extends exact-match lookup with per-bit masking: each stored entry
 * carries a mask that marks bits as "care" (1) or "don't-care" (0).  A search
 * key matches an entry when @c (key XOR entry_key) AND entry_mask == 0.
 *
 * Four synthesis strategies are supported via preprocessor macros defined in
 * config.h:
 *  - @b BF   (Brute Force)             – sequential scan, stored in LUTRAM.
 *  - @b BL   (Balanced)                – pipelined with cyclic partitioning.
 *  - @b HS   (High Speed)              – pipelined with full partitioning.
 *  - @b HS_H (High-Speed Hierarchical) – pipelined hierarchical binary search.
 */

#ifndef TCAM_H
#define TCAM_H

#include <ap_int.h>

/**
 * @brief Ternary CAM (wildcard/mask-based match) template.
 *
 * @tparam KEY_WIDTH   Bit-width of each lookup key (and mask).
 * @tparam VALUE_WIDTH Bit-width of the value stored alongside each entry.
 * @tparam DEPTH       Maximum number of entries in the table.
 */
template <int KEY_WIDTH, int VALUE_WIDTH, int DEPTH>
class TCAM {
public:
    /**
     * @brief A single TCAM entry holding a key, a ternary mask, a value, and a
     *        validity flag.
     */
    struct Entry {
        ap_uint<KEY_WIDTH>   key;   /**< Stored lookup key. */
        ap_uint<KEY_WIDTH>   mask;  /**< Care-bit mask: 1 = must match, 0 = don't care. */
        ap_uint<VALUE_WIDTH> value; /**< Value associated with the entry. */
        bool                 valid; /**< True when this slot is occupied. */
    };

    /** @brief Storage array for all CAM entries. */
    volatile Entry entries[DEPTH];

    /**
     * @brief Default constructor – entry array is left uninitialised; the
     *        hardware reset is expected to clear the valid bits.
     */
    TCAM() {
    }


    // ------------------------------------- HS_H mode ---------------------------------------

    /**
     * @brief Compile-time binary-tree ternary search over entries[START..END].
     *
     * Used exclusively in the @b HS_H synthesis mode.  The recursion is fully
     * resolved at compile time, producing a balanced comparator tree that the
     * HLS tool can pipeline at a high clock frequency.
     *
     * @tparam KW    Key width (mirrors the outer TCAM template parameter).
     * @tparam VW    Value width (mirrors the outer TCAM template parameter).
     * @tparam D     Table depth (mirrors the outer TCAM template parameter).
     * @tparam START First entry index of this recursion level (inclusive).
     * @tparam END   Last entry index of this recursion level (inclusive).
     */
    template<int KW, int VW, int D, int START, int END>
    struct TernaryMatchIndex
    {
        /**
         * @brief Searches @p x.entries[START..END] for a ternary match of @p key.
         *
         * @param[in] x   TCAM instance to search.
         * @param[in] key Search key.
         * @return Index of the first matching valid entry, or @c -1 if none found.
         */
        static int compute(const TCAM<KW, VW, D> &x, const ap_uint<KW> &key) {
            constexpr int MID = (START + END) / 2;

            int idx_left = TernaryMatchIndex<KW, VW, D, START, MID>::compute(x, key);
            int idx_right = TernaryMatchIndex<KW, VW, D, MID + 1, END>::compute(x, key);

            if (idx_left != -1)
                return idx_left;
            else if (idx_right != -1)
                return idx_right;

            return -1;
        }
    };

    /**
     * @brief Base-case specialisation of TernaryMatchIndex for a single entry.
     *
     * @tparam KW    Key width.
     * @tparam VW    Value width.
     * @tparam D     Table depth.
     * @tparam INDEX The single entry index checked by this specialisation.
     */
    template<int KW, int VW, int D, int INDEX>
    struct TernaryMatchIndex<KW, VW, D, INDEX, INDEX>
    {
        /**
         * @brief Checks whether entry @p INDEX is valid and ternary-matches @p key.
         *
         * A match occurs when @c (key XOR entry_key) AND entry_mask == 0.
         *
         * @param[in] x   TCAM instance to search.
         * @param[in] key Search key.
         * @return @p INDEX if the entry matches, otherwise @c -1.
         */
        static int compute(const TCAM<KW, VW, D> &x, const ap_uint<KW> &key) {
#pragma HLS INLINE
        	if (x.entries[INDEX].valid && !((key ^ static_cast<const ap_uint<KW>>(x.entries[INDEX].key)) & static_cast<const ap_uint<KW>>(x.entries[INDEX].mask)))
        		return INDEX;
        	else
        		return -1;
        }
    };
    // ------------------------------------- HS_H mode ---------------------------------------

    /**
     * @brief Writes a key–mask–value triple directly to the slot addressed by
     *        the numeric value of @p key (used for testing / initialisation only).
     *
     * @param[in]     key   Key whose integer value is used as the array index.
     * @param[in]     mask  Care-bit mask for ternary matching.
     * @param[in]     value Value to store.
     */
    void dummy_add(const ap_uint<KEY_WIDTH> &key, const ap_uint<KEY_WIDTH> &mask, ap_uint<VALUE_WIDTH> &value) {
#pragma HLS PIPELINE
        const int i = key.to_uint();
        entries[i].key = key;
        entries[i].mask = mask;
        entries[i].value = value;
        entries[i].valid = true;
    }

    /**
     * @brief Inserts a new key–mask–value entry into the first available
     *        (invalid) slot in the table.
     *
     * All slots are checked in parallel (fully unrolled) so the operation
     * completes in a single clock cycle when pipelined.  If the table is full
     * the insertion is silently dropped.
     *
     * @param[in] key   Key to insert.
     * @param[in] mask  Care-bit mask for ternary matching.
     * @param[in] value Associated value.
     */
    void add_entry(const ap_uint<KEY_WIDTH> &key, const ap_uint<KEY_WIDTH> &mask, const ap_uint<VALUE_WIDTH> &value) {
#pragma HLS INLINE
        for (int i = 0; i < DEPTH; i++) {
#pragma HLS UNROLL
            if (!entries[i].valid) {
                entries[i].key = key;
                entries[i].mask = mask;
                entries[i].value = value;
                entries[i].valid = true;
                break;
            }
        }
    }

    /**
     * @brief Invalidates the first entry whose key and mask match the supplied
     *        arguments.
     *
     * The search is fully unrolled; when the first match is found, the valid
     * bit is cleared and the loop exits.
     *
     * @param[in] key  Key identifying the entry to remove.
     * @param[in] mask Mask identifying the entry to remove (must match exactly).
     */
    void remove_entry(const ap_uint<KEY_WIDTH> &key, const ap_uint<KEY_WIDTH> &mask) {
#pragma HLS INLINE
        for (int i = 0; i < DEPTH; i++) {
            #pragma HLS UNROLL
            if (entries[i].valid && ap_uint<KEY_WIDTH>(entries[i].key) == key && ap_uint<KEY_WIDTH>(entries[i].mask) == mask) {
                entries[i].valid = false;
                break;
            }
        }
    }

    /**
     * @brief Searches for an entry whose ternary pattern matches @p key and
     *        returns the associated value.
     *
     * All entries are evaluated in parallel in two stages:
     * 1. A ternary comparison builds a @p masked[] flag array.
     * 2. A scan over @p masked[] captures the last matching value.
     *
     * The search strategy depends on the active synthesis mode:
     * - @b BASELINE: inlined sequential scan (lowest area, highest latency).
     * - @b BL / @b HS: pipelined parallel scan over all entries.
     *
     * @param[in]  key   Key to look up.
     * @param[out] value Set to the matching entry's value when found.
     * @return @c true if a matching valid entry was found, @c false otherwise.
     */
    bool search_entry(const ap_uint<KEY_WIDTH> &key, ap_uint<VALUE_WIDTH> &value) {
#if defined(BASELINE)
#pragma HLS INLINE
#else
#pragma HLS PIPELINE
#endif
    	bool flag = false;
    	bool masked[DEPTH];
#pragma HLS ARRAY_PARTITION variable=masked complete dim=1

    	for (int i = 0; i < DEPTH; i++) {
#pragma HLS UNROLL
    		const ap_uint<KEY_WIDTH> entry_key(entries[i].key);
    		const ap_uint<KEY_WIDTH> entry_mask(entries[i].mask);
    		masked[i] = (entries[i].valid && !((key ^ entry_key) & entry_mask)) ? true : false;
    	}
    	for (int i = 0; i < DEPTH; i++) {
#pragma HLS UNROLL
    		if (masked[i]) {
    			value = entries[i].value;
    			flag = true;
    		}
    	}
    	return flag;
    }
};


#endif // TCAM_H
