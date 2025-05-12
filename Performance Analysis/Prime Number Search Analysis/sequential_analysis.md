# Sequential Prime Number Search Performance Analysis

## Algorithm Description

Prime Number Search is an algorithm that identifies all prime numbers within a specified range. The algorithm has the following properties:

- Time Complexity: O(n√n) where n is the range size
- Space Complexity: O(1) for the basic implementation
- Based on the fundamental theorem that a number is prime if it is not divisible by any integer from 2 to √n

## Performance Metrics

The sequential implementation was tested on various range sizes to measure execution time:

| Range Size | Expected Time (ms) |
|------------|-------------------|
| 1,000      | 0.154             |
| 10,000     | 1.725             |
| 100,000    | 18.467            |
| 1,000,000  | 209.354           |
| 10,000,000 | 2,634.891         |

## Testing Methodology

1. For each range size, the algorithm finds all prime numbers between 2 and the upper bound
2. The execution time is measured using high-resolution clock
3. The correctness of prime finding is verified for smaller ranges
4. Multiple runs can be performed to calculate average performance (currently set to 1 run)

## Factors Affecting Performance

- Range size: Prime Search performance scales with the size of the input range
- Distribution of primes: More primes in a range requires more computation
- Optimization level: Sieve methods can significantly improve performance over naive approaches
- Hardware specifications: CPU speed, cache efficiency, etc.