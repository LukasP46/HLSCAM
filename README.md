# HLSCAM: High-Level Synthesis-Based Content Addressable Memory for Packet Processing

This repository contains the source code for **HLSCAM**, a flexible and high-performance Content Addressable Memory (CAM) framework implemented using **High-Level Synthesis (HLS)** for **FPGA-based packet processing**, available at: https://www.mdpi.com/2079-9292/14/9/1765

HLSCAM supports:
- **BCAM** (Binary CAM) for exact matching
- **TCAM** (Ternary CAM) for wildcard-based matching
- **STCAM** (Semi-TCAM) for prefix-based matching (Longest Prefix Match) (not investigated in the paper)

The design is templated and highly configurable, enabling easy adjustment of key width, value width, table depth, and optimization strategies.

## Features
- Written in standard **C++** for **AMD (Xilinx) Vitis HLS** toolchain
- Supports **Brute Force (BF)**, **Balanced (BL)**, **High-Speed (HS)**, and **High-Speed Hierarchical (HS_H)** optimization modes
- Flexible memory partitioning to balance between resource usage and lookup latency
- Achieves competitive operating frequencies without relying on BRAMs
- Suitable for **high-speed packet classification**, **firewall rule matching**, **flow table lookup**, and **5G core user management**

## Optimization Modes

| Mode | Macro | Description |
|------|-------|-------------|
| Brute Force | `BF` | Sequential scan with LUTRAM storage. Lowest resource usage, highest latency. |
| Balanced | `BL` | Pipelined with cyclic array partitioning (factor 16). Balances speed and area. |
| High Speed | `HS` | Fully pipelined with complete array partitioning. Maximum throughput. |
| High Speed Hierarchical | `HS_H` | Pipelined with a compile-time binary-tree search. Targets maximum clock frequency through a balanced comparator tree. |

Enable the desired mode by uncommenting the corresponding macro in `src/config.h`.

## Clone the repository:
   ```bash
   git clone https://github.com/abbasmollaei/HLSCAM
   ```
   
## Requirements
- AMD/Xilinx Vitis HLS 2023.2 or newer
- AMD/Xilinx Vivado 2023.2 or newer for implementation and place-and-route evaluation

# How to cite the paper

**Bibtex:**<br />
@Article{electronics14091765, <br />
&nbsp;&nbsp;&nbsp;&nbsp;AUTHOR = {Abbasmollaei, Mostafa and Ould-Bachir, Tarek and Savaria, Yvon}, <br />
&nbsp;&nbsp;&nbsp;&nbsp;TITLE = {HLSCAM: Fine-Tuned HLS-Based Content Addressable Memory Implementation for Packet Processing on FPGA}, <br />
&nbsp;&nbsp;&nbsp;&nbsp;JOURNAL = {Electronics}, <br />
&nbsp;&nbsp;&nbsp;&nbsp;VOLUME = {14}, <br />
&nbsp;&nbsp;&nbsp;&nbsp;YEAR = {2025}, <br />
&nbsp;&nbsp;&nbsp;&nbsp;NUMBER = {9}, <br />
&nbsp;&nbsp;&nbsp;&nbsp;ARTICLE-NUMBER = {1765}, <br />
&nbsp;&nbsp;&nbsp;&nbsp;URL = {https://www.mdpi.com/2079-9292/14/9/1765}, <br />
&nbsp;&nbsp;&nbsp;&nbsp;ISSN = {2079-9292}, <br />
&nbsp;&nbsp;&nbsp;&nbsp;DOI = {10.3390/electronics14091765} <br />
}
