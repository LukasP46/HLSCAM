/**
 * @file array_util.h
 * @brief Compile-time divide-and-conquer utility for finding the index of the
 *        minimum element in a fixed-size array.
 *
 * The MinIndex template is used by the STCAM longest-prefix-match logic to
 * select the entry with the smallest mask shift value (i.e., the most specific
 * prefix) in a fully unrolled, pipelinable manner that is friendly to HLS
 * synthesis.
 */

#ifndef ARRAY_UTIL_H
#define ARRAY_UTIL_H

/**
 * @brief Recursive template that finds the index of the minimum element in the
 *        sub-array @p x[START..END] using a divide-and-conquer strategy.
 *
 * @tparam T     Element type (must support comparison operators).
 * @tparam START First index of the sub-array range (inclusive).
 * @tparam END   Last index of the sub-array range (inclusive).
 */
template<typename T, int START, int END>
struct MinIndex
{
    /**
     * @brief Finds the index of the minimum element in @p x[START..END].
     *
     * @param[in]  x   Array to search.
     * @param[out] min Updated with the minimum value found in the range.
     * @return Index of the minimum element within @p x[START..END].
     */
    static int compute(const T x[], T& min) {
        constexpr int MID = (START + END) / 2;

        T min_left = min, min_right = min;
        const int min_idx_left = MinIndex<T, START, MID>::compute(x, min_left);
        const int min_idx_right = MinIndex<T, MID + 1, END>::compute(x, min_right);

        if (min_left >= min_right) {
            min = min_right;
            return min_idx_right;
        } else {
        	min = min_right;
            return min_idx_left;
        }
    }
};

/**
 * @brief Base-case specialization of MinIndex for a single-element range.
 *
 * @tparam T     Element type.
 * @tparam INDEX The single index of this specialization.
 */
template<typename T, int INDEX>
struct MinIndex<T, INDEX, INDEX>
{
    /**
     * @brief Returns the only element's index and sets @p min to its value.
     *
     * @param[in]  x   Array to search.
     * @param[out] min Set to @p x[INDEX].
     * @return @p INDEX.
     */
    static int compute(const T x[], T& min) {
    	min = x[INDEX];
        return INDEX;
    }
};

#endif // ARRAY_UTIL_H
