/**
 * @file config.h
 * @brief Global configuration parameters and mode selection macros for HLSCAM.
 *
 * This file defines the compile-time constants that govern the CAM table
 * dimensions, as well as the preprocessor macros used to select the CAM
 * variant and the optimization strategy that should be synthesised.
 *
 * Exactly one CAM-type macro (USE_BCAM / USE_TCAM / USE_STCAM) and exactly
 * one mode macro (BF / BL / HS / HS_H) should be enabled at a time.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <ap_int.h>

/** @brief Number of entries in the CAM table. */
const int CAM_DEPTH = 8192;

/** @brief Bit-width of each lookup key. */
const int CAM_KEY_WIDTH = 64;

/** @brief Bit-width of the value associated with each entry. */
const int CAM_VALUE_WIDTH = 1;

/**
 * @defgroup cam_type CAM Type Selection
 * Enable exactly one of the following macros to choose the CAM variant.
 * @{
 */
#define USE_BCAM    /**< Use Binary CAM (exact-match). */
//#define USE_TCAM  /**< Use Ternary CAM (wildcard/mask-based match). */
//#define USE_STCAM /**< Use Semi-TCAM (longest-prefix match). */
/** @} */

/**
 * @defgroup opt_mode Optimization Mode Selection
 * Enable exactly one of the following macros to choose the synthesis strategy.
 * @{
 */
//#define BF   /**< Brute-Force: sequential scan stored in LUTRAM, lowest resource usage. */
//#define BL   /**< Balanced: pipelined with cyclic array partitioning for a trade-off between speed and area. */
//#define HS   /**< High-Speed: fully pipelined with complete array partitioning for maximum throughput. */
//#define HS_H /**< High-Speed Hierarchical: pipelined with a compile-time binary-tree search for maximum frequency. */
/** @} */

#endif // CONFIG_H
