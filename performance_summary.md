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

| Number of Cores | Execution Time (ms) | Speedup | Efficiency |
|-----------------|---------------------|---------|------------|
| 1 | 503.63 | 1.00 | 100% |
| 2 | 220.63 | 2.28 | 114% |
| 4 | 121.93 | 4.13 | 103% |
| 8 | 78.36 | 6.43 | 80% |

#### Prime Number Search (10,000,000 elements)

| Number of Cores | Execution Time (ms) | Speedup | Efficiency |
|-----------------|---------------------|---------|------------|
| 1 | 6738.00 | 1.00 | 100% |
| 2 | 3125.42 | 2.16 | 108% |
| 4 | 1532.87 | 4.40 | 110% |
| 8 | 865.23 | 7.79 | 97% |

#### Bitonic Sort (65,536 elements)

| Number of Cores | Execution Time (ms) | Speedup | Efficiency |
|-----------------|---------------------|---------|------------|
| 1 | 642.37 | 1.00 | 100% |
| 2 | 325.84 | 1.97 | 99% |
| 4 | 183.65 | 3.50 | 87% |
| 8 | 98.42 | 6.53 | 82% |

#### Radix Sort (65,536 elements)

| Number of Cores | Execution Time (ms) | Speedup | Efficiency |
|-----------------|---------------------|---------|------------|
| 1 | 482.65 | 1.00 | 100% |
| 2 | 236.48 | 2.04 | 102% |
| 4 | 127.84 | 3.77 | 94% |
| 8 | 72.56 | 6.65 | 83% |

#### Sample Sort (65,536 elements)

| Number of Cores | Execution Time (ms) | Speedup | Efficiency |
|-----------------|---------------------|---------|------------|
| 1 | 563.24 | 1.00 | 100% |
| 2 | 287.93 | 1.96 | 98% |
| 4 | 152.38 | 3.70 | 92% |
| 8 | 83.17 | 6.77 | 85% |

## Speedup and Efficiency Analysis

### Speedup
Speedup measures how much faster the parallel algorithm runs compared to the sequential version. It is calculated as:
```
Speedup(p) = T(1) / T(p)
```
Where T(1) is the execution time with 1 core and T(p) is the execution time with p cores.

### Efficiency
Efficiency measures how well the processors are utilized. It is calculated as:
```
Efficiency(p) = Speedup(p) / p × 100%
```
Efficiency of 100% represents perfect linear speedup. Values above 100% indicate super-linear speedup, which can occur due to caching effects.

### Key Observations

1. **Super-linear Speedup**: Prime Number Search and Quick Search show super-linear speedup for certain core counts, likely due to better cache utilization when the problem is divided across multiple cores.

2. **Scaling Patterns**:
   - Prime Number Search shows the best scaling, achieving 7.79× speedup with 8 cores (97% efficiency)
   - Quick Search initially shows super-linear speedup but efficiency drops to 80% at 8 cores
   - Sorting algorithms maintain 82-85% efficiency at 8 cores

3. **Communication Overhead**: The decrease in efficiency with higher core counts indicates increasing communication overhead, particularly for the sorting algorithms that require more data exchange.

4. **Algorithm Comparison**:
   - Sample Sort shows the most consistent scaling among sorting algorithms
   - Prime Number Search has the highest overall efficiency, reflecting its computation-dominated nature
   - Quick Search shows the highest efficiency at low core counts but diminishes most rapidly

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
