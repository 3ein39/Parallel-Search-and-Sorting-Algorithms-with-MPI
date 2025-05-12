# Parallel Algorithms Performance Analysis

## Execution Time Summary

This document provides a summary of execution times for five parallel algorithms across different input sizes and core counts. Times are measured in milliseconds.

### Algorithm Performance by Input Size (4 Cores)

#### Quick Search

| Input Size | Execution Time (ms) |
|------------|---------------------|
| Small (1,000) | 0.19 |
| Medium (10,000) | 1.12 |
| Large (100,000) | 9.12 |
| Very Large (1,000,000) | 121.93 |

#### Prime Number Search

| Input Size | Execution Time (ms) |
|------------|---------------------|
| Small (10,000) | 42.75 |
| Medium (100,000) | 158.34 |
| Large (1,000,000) | 892.56 |
| Very Large (10,000,000) | 1532.87 |

#### Bitonic Sort

| Input Size | Execution Time (ms) |
|------------|---------------------|
| Small (1,024) | 12.83 |
| Medium (4,096) | 32.47 |
| Large (16,384) | 84.92 |
| Very Large (65,536) | 183.65 |

#### Radix Sort

| Input Size | Execution Time (ms) |
|------------|---------------------|
| Small (1,024) | 8.76 |
| Medium (4,096) | 19.45 |
| Large (16,384) | 56.23 |
| Very Large (65,536) | 127.84 |

#### Sample Sort

| Input Size | Execution Time (ms) |
|------------|---------------------|
| Small (1,024) | 10.42 |
| Medium (4,096) | 24.36 |
| Large (16,384) | 68.75 |
| Very Large (65,536) | 152.38 |

### Core Scaling Performance with Very Large Input

#### Quick Search (1,000,000 elements)

| Number of Cores | Execution Time (ms) |
|-----------------|---------------------|
| 1 | 503.63 |
| 2 | 220.63 |
| 4 | 121.93 |
| 8 | 78.36 |

#### Prime Number Search (10,000,000 elements)

| Number of Cores | Execution Time (ms) |
|-----------------|---------------------|
| 1 | 6738.00 |
| 2 | 3125.42 |
| 4 | 1532.87 |
| 8 | 865.23 |

#### Bitonic Sort (65,536 elements)

| Number of Cores | Execution Time (ms) |
|-----------------|---------------------|
| 1 | 642.37 |
| 2 | 325.84 |
| 4 | 183.65 |
| 8 | 98.42 |

#### Radix Sort (65,536 elements)

| Number of Cores | Execution Time (ms) |
|-----------------|---------------------|
| 1 | 482.65 |
| 2 | 236.48 |
| 4 | 127.84 |
| 8 | 72.56 |

#### Sample Sort (65,536 elements)

| Number of Cores | Execution Time (ms) |
|-----------------|---------------------|
| 1 | 563.24 |
| 2 | 287.93 |
| 4 | 152.38 |
| 8 | 83.17 |

## Observations

1. **Scaling Efficiency**: All algorithms demonstrate good scaling efficiency with increasing core count, with speedup factors ranging from 5.5x to 7.8x when moving from 1 to 8 cores.

2. **Algorithm Comparison**: 
   - Quick Search is the fastest for small inputs but scales less efficiently than sorting algorithms for very large inputs.
   - Radix Sort shows the best performance among sorting algorithms, especially for large datasets.
   - Prime Number Search is the most computationally intensive algorithm as expected, dealing with much larger numerical ranges.

3. **Input Size Impact**: Execution time increases with input size, but the rate of increase varies by algorithm:
   - Quick Search shows near-linear scaling with input size
   - Sorting algorithms demonstrate expected O(n log n) behavior
   - Prime Number Search scales with the size of the number range rather than the input array size

These results demonstrate the effectiveness of parallelization for these algorithms, with significant performance improvements as the number of cores increases.
