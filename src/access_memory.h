/**
 * @file access_memory.h
 * @brief Top-level CAM access function declarations for HLSCAM.
 *
 * This header exposes a single @c access_cam() function that acts as the HLS
 * top-level kernel.  The exact signature is selected at compile time based on
 * the CAM type (USE_BCAM / USE_TCAM / USE_STCAM) and the optimisation mode
 * (HS_H vs. all other modes) defined in config.h.
 *
 * In the standard modes (BF, BL, HS) a Boolean @p write flag controls whether
 * the function performs an insertion (write = true) or a lookup (write = false).
 *
 * In the HS_H mode an additional integer @p index output returns the physical
 * slot index of the matched entry (or @c -1 on a miss).
 */

#ifndef ACCESS_MEMORY_H
#define ACCESS_MEMORY_H

#include "config.h"

#if defined(HS_H)

#if defined(USE_BCAM)
/**
 * @brief BCAM access function for the HS_H (High-Speed Hierarchical) mode.
 *
 * @param[in]     key   Key to insert or search.
 * @param[in,out] value Value to write (write mode) or value read out (read mode).
 * @param[out]    index Physical slot index of the matched entry, or @c -1 on miss.
 * @param[in]     write @c true  – insert @p key / @p value into the table.
 *                      @c false – search for @p key and return its value.
 * @return @c true if the operation succeeded (entry found or inserted),
 *         @c false on a lookup miss.
 */
bool access_cam(const ap_uint<CAM_KEY_WIDTH> &key, ap_uint<CAM_VALUE_WIDTH> &value, int &index, bool write);

#elif defined(USE_TCAM)
/**
 * @brief TCAM access function for the HS_H (High-Speed Hierarchical) mode.
 *
 * @param[in]     key   Key to insert or search.
 * @param[in]     mask  Care-bit mask (used for both insert and search).
 * @param[in,out] value Value to write (write mode) or value read out (read mode).
 * @param[out]    index Physical slot index of the matched entry, or @c -1 on miss.
 * @param[in]     write @c true  – insert the entry into the table.
 *                      @c false – search for a ternary match and return its value.
 * @return @c true if the operation succeeded, @c false on a lookup miss.
 */
bool access_cam(const ap_uint<CAM_KEY_WIDTH> &key, const ap_uint<CAM_KEY_WIDTH> mask, ap_uint<CAM_VALUE_WIDTH> &value, int &index, bool write);

#elif defined(USE_STCAM)
/**
 * @brief STCAM access function for the HS_H (High-Speed Hierarchical) mode.
 *
 * @param[in]  key   Key to insert or search.
 * @param[out] valid Set to @c true when a matching entry is found.
 * @param[out] index Physical slot index of the best-matching entry, or @c -1.
 * @param[in]  write @c true  – insert the key into the table.
 *                   @c false – perform a Longest Prefix Match lookup.
 * @return Value associated with the best-matching entry.
 */
ap_uint<CAM_VALUE_WIDTH> access_cam(const ap_uint<CAM_KEY_WIDTH> &key, bool &valid, int &index, bool write);

#endif

#else  /* standard modes: BF, BL, HS */

#if defined(USE_BCAM)
/**
 * @brief BCAM access function for the standard synthesis modes (BF, BL, HS).
 *
 * @param[in]     key   Key to insert or search.
 * @param[in,out] value Value to write (write mode) or value read out (read mode).
 * @param[in]     write @c true  – insert @p key / @p value into the table.
 *                      @c false – search for @p key and return its value.
 * @return @c true if the operation succeeded (entry found or inserted),
 *         @c false on a lookup miss.
 */
bool access_cam(const ap_uint<CAM_KEY_WIDTH> &key, ap_uint<CAM_VALUE_WIDTH> &value, bool write);

#elif defined(USE_TCAM)
/**
 * @brief TCAM access function for the standard synthesis modes (BF, BL, HS).
 *
 * @param[in]     key   Key to insert or search.
 * @param[in]     mask  Care-bit mask (used for both insert and search).
 * @param[in,out] value Value to write (write mode) or value read out (read mode).
 * @param[in]     write @c true  – insert the entry into the table.
 *                      @c false – search for a ternary match and return its value.
 * @return @c true if the operation succeeded, @c false on a lookup miss.
 */
bool access_cam(const ap_uint<CAM_KEY_WIDTH> &key, const ap_uint<CAM_KEY_WIDTH> mask, ap_uint<CAM_VALUE_WIDTH> &value, bool write);

#elif defined(USE_STCAM)
/**
 * @brief STCAM access function for the standard synthesis modes (BF, BL, HS).
 *
 * @param[in]  key   Key to insert or search.
 * @param[out] valid Set to @c true when a matching entry is found.
 * @param[in]  write @c true  – insert the key into the table.
 *                   @c false – perform a Longest Prefix Match lookup.
 * @return Value associated with the best-matching entry.
 */
ap_uint<CAM_VALUE_WIDTH> access_cam(const ap_uint<CAM_KEY_WIDTH> &key, bool &valid, bool write);
#endif

#endif

#endif // ACCESS_MEMORY_H
