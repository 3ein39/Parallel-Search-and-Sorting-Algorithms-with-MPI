# Sequential Bitonic Sort Performance Analysis

## Algorithm Description

Bitonic Sort is a comparison-based sorting algorithm that first builds a bitonic sequence (a sequence that monotonically increases and then monotonically decreases, or vice versa) and then sorts it. The algorithm has the following properties:

- Time Complexity: $O(log² n)$
- Space Complexity: $O(n.log² n)$ (in-place sorting)
- Requires the input size to be a power of 2

## Performance Metrics

The sequential implementation was tested on various input sizes (powers of 2) to measure execution time:

| Array Size | Expected Time (ms) |
|------------|-------------------|
| 1,024      | 0.388               |
| 2,048      | 0.758               |
| 4,096      | 1.321               |
| 8,192      | 2.988               |
| 16,384     | 6.993               |
| 32,768     | 14.744               |
| 65,536     | 31.870               |

## Testing Methodology

1. For each array size, a random array of integers (range 0-1000) is generated
2. The sorting time is measured using high-resolution clock
3. The correctness of sorting is verified for smaller arrays
4. Multiple runs can be performed to calculate average performance (currently set to 1 run)


## Factors Affecting Performance

- Input size: Bitonic Sort's performance is heavily dependent on the size of the input array
- Hardware specifications: CPU speed, memory bandwidth, etc.
