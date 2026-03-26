/**
 * @file bcam.h
 * @brief Binary Content Addressable Memory (BCAM) template class.
 *
 * A BCAM performs exact-key lookup: given a search key it returns the value
 * associated with a matching entry, or reports no-match.  The class is
 * parameterised on key width, value width, and table depth, making it
 * straightforward to size for different use cases without modifying the logic.
 *
 * Four synthesis strategies are supported via preprocessor macros defined in
 * config.h:
 *  - @b BF  (Brute Force)            – sequential scan, stored in LUTRAM.
 *  - @b BL  (Balanced)               – pipelined with cyclic partitioning.
 *  - @b HS  (High Speed)             – pipelined with full partitioning.
 *  - @b HS_H (High-Speed Hierarchical) – pipelined hierarchical binary search.
 */

#ifndef BCAM_H
#define BCAM_H

#include "config.h"

/**
 * @brief Binary CAM (exact-match) template.
 *
 * @tparam KEY_WIDTH   Bit-width of each lookup key.
 * @tparam VALUE_WIDTH Bit-width of the value stored alongside each key.
 * @tparam DEPTH       Maximum number of entries in the table.
 */
template <int KEY_WIDTH, int VALUE_WIDTH, int DEPTH>
class BCAM {
public:
    /**
     * @brief A single CAM entry holding a key–value pair and a validity flag.
     */
    struct Entry {
        ap_uint<KEY_WIDTH>   key;   /**< Stored lookup key. */
        ap_uint<VALUE_WIDTH> value; /**< Value associated with the key. */
        bool                 valid; /**< True when this slot is occupied. */
    };

    /** @brief Storage array for all CAM entries. */
    volatile Entry entries[DEPTH];

    /**
     * @brief Default constructor – entry array is left uninitialised.
     */
    BCAM() {
    }

    // ------------------------------------- HS_H mode ---------------------------------------

    /**
     * @brief Compile-time binary-tree search over entries[START..END].
     *
     * Used exclusively in the @b HS_H synthesis mode.  The recursion is fully
     * resolved at compile time, producing a balanced comparator tree that the
     * HLS tool can pipeline at a high clock frequency.
     *
     * @tparam KW    Key width (mirrors the outer BCAM template parameter).
     * @tparam VW    Value width (mirrors the outer BCAM template parameter).
     * @tparam D     Table depth (mirrors the outer BCAM template parameter).
     * @tparam START First entry index of this recursion level (inclusive).
     * @tparam END   Last entry index of this recursion level (inclusive).
     */
    template<int KW, int VW, int D, int START, int END>
    struct BinaryMatchIndex
    {
        /**
         * @brief Searches @p x.entries[START..END] for an entry matching @p key.
         *
         * @param[in] x   BCAM instance to search.
         * @param[in] key Search key.
         * @return Index of the first matching valid entry, or @c -1 if none found.
         */
        static int compute(const BCAM<KW, VW, D> &x, const ap_uint<KW> &key) {
            constexpr int MID = (START + END) / 2;

            int idx_left = BinaryMatchIndex<KW, VW, D, START, MID>::compute(x, key);
            int idx_right = BinaryMatchIndex<KW, VW, D, MID + 1, END>::compute(x, key);

            if (idx_left != -1)
                return idx_left;
            else if (idx_right != -1)
                return idx_right;

            return -1;
        }
    };

    /**
     * @brief Base-case specialisation of BinaryMatchIndex for a single entry.
     *
     * @tparam KW    Key width.
     * @tparam VW    Value width.
     * @tparam D     Table depth.
     * @tparam INDEX The single entry index checked by this specialisation.
     */
    template<int KW, int VW, int D, int INDEX>
    struct BinaryMatchIndex<KW, VW, D, INDEX, INDEX>
    {
        /**
         * @brief Checks whether entry @p INDEX is valid and matches @p key.
         *
         * @param[in] x   BCAM instance to search.
         * @param[in] key Search key.
         * @return @p INDEX if the entry matches, otherwise @c -1.
         */
        static int compute(const BCAM<KW, VW, D> &x, const ap_uint<KW> &key) {
#pragma HLS INLINE
        	if (x.entries[INDEX].valid && (static_cast<const ap_uint<KW>>(x.entries[INDEX].key) == key))
        		return INDEX;
        	else
        		return -1;
        }
    };
    // ------------------------------------- HS_H mode ---------------------------------------

    /**
     * @brief Writes a key–value pair directly to the slot addressed by the
     *        numeric value of @p key (used for testing / initialisation only).
     *
     * @param[in]     key   Key whose integer value is used as the array index.
     * @param[in]     value Value to store.
     */
    void dummy_add(const ap_uint<KEY_WIDTH> &key, ap_uint<VALUE_WIDTH> &value) {
        const int i = key.to_uint();
    	entries[i].key = key;
        entries[i].value = value;
        entries[i].valid = true;
    }

    /**
     * @brief Inserts a new key–value entry into the first available (invalid)
     *        slot in the table.
     *
     * All slots are checked in parallel (fully unrolled) so the operation
     * completes in a single clock cycle when pipelined.  If the table is full
     * the insertion is silently dropped.
     *
     * @param[in] key   Key to insert.
     * @param[in] value Associated value.
     */
    void add_entry(const ap_uint<KEY_WIDTH> &key, const ap_uint<VALUE_WIDTH> &value) {
        #pragma HLS INLINE
        for (int i = 0; i < DEPTH; i++) {
            #pragma HLS UNROLL
            if (!entries[i].valid) {
                entries[i].key = key;
                entries[i].value = value;
                entries[i].valid = true;
                break;
            }
        }
    }

    /**
     * @brief Invalidates the first entry whose key matches @p key.
     *
     * The search is fully unrolled; when the first match is found, the valid
     * bit is cleared and the loop exits.
     *
     * @param[in] key Key identifying the entry to remove.
     */
    void remove_entry(ap_uint<KEY_WIDTH> key) {
        #pragma HLS INLINE
        for (int i = 0; i < DEPTH; i++) {
            #pragma HLS UNROLL
            if (entries[i].valid && (entries[i].key == key)) {
                entries[i].valid = false;
                break;
            }
        }
    }

    /**
     * @brief Searches for an entry matching @p key and returns its value.
     *
     * The search strategy depends on the active synthesis mode:
     * - @b BF: inlined sequential scan (lowest area, highest latency).
     * - @b BL / @b HS: pipelined parallel scan over all entries.
     *
     * @param[in]  key   Key to look up.
     * @param[out] value Set to the matching entry's value when found.
     * @return @c true if a matching valid entry was found, @c false otherwise.
     */
    bool search_entry(const ap_uint<KEY_WIDTH> &key, ap_uint<VALUE_WIDTH> &value) {
#if defined(BF)
#pragma HLS INLINE
#else
#pragma HLS PIPELINE
#endif
    	bool flag = false;
        for (int i = 0; i < DEPTH; i++) {
            #pragma HLS UNROLL
            if (entries[i].valid && (ap_uint<KEY_WIDTH>(entries[i].key) == key)) {
                value = entries[i].value;
                flag = true;
            }
        }
        return flag;
    }
};

#endif // BCAM_H
