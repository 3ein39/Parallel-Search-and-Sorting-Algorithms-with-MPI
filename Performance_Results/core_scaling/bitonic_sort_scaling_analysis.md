# Bitonic Sort Core Scaling Analysis

## Test Configuration
- Date: Mon May 12 10:34:30 PM EEST 2025
- Fixed input size (very large)
- Each test averaged over 5 iterations

## Results

| Number of Cores | Max User Time (ms) | System Time (ms) | Speedup |
|-----------------|-------------------|------------------|---------|
| 1 | 40 | 50 | 1.00 |
| 2 | 32 | 63 | 1.25 |
| 4 | 27 | 127 | 1.48 |
| 8 | 26 | 296 | 1.53 |

## Scaling Analysis

This analysis shows how Bitonic Sort scales with increasing number of processor cores while keeping the input size constant at very large. Each test was run 5 times and the results were averaged to provide more reliable metrics.

### Observations

- Write your observations about scaling efficiency
- Analyze if the algorithm achieves linear speedup
- Identify potential bottlenecks in parallelization
- Ideal speedup would be equal to the number of cores

### Note on User Time

The 'Max User Time' column represents the estimated maximum CPU time used by any single core, rather than the sum of all cores. This gives a more accurate representation of actual processor utilization per core.

### Note on Multiple Iterations

Each test was run 5 times and the timing results were averaged. This approach provides more reliable performance metrics by reducing the impact of random system variations, background processes, and other factors that might affect individual test runs.

Speedup is calculated by dividing the single-core user time by the multi-core user time, showing how much faster the algorithm runs with more cores.

