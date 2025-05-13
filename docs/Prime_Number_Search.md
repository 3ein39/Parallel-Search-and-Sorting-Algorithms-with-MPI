## 1. Core Functions Explanation

### `isPrime` Function
```cpp
bool isPrime(int n)
{
    if (n <= 1)
        return false;
    for (int i = 2; i * i <= n; ++i)
        if (n % i == 0)
            return false;
    return true;
}
```
This is a helper function that checks if a given number is prime:
- Returns `false` if the number is less than or equal to 1 (since these are not prime)
- Uses trial division up to the square root of `n` to check for divisibility
- If any divisor is found, the number is not prime and it returns `false`
- If no divisors are found, it returns `true`

### `findPrimes` Function
```cpp
vector<int> findPrimes(int start, int end)
{
    vector<int> primes;
    for (int i = start; i <= end; ++i)
        if (isPrime(i))
            primes.push_back(i);
    return primes;
}
```
This function searches for prime numbers in a given range:
- It checks each number between `start` and `end` (inclusive)
- If a number is prime (using the `isPrime` function), it adds it to the `primes` vector
- It returns the collection of prime numbers found

### `parallelPrimeSearch` Function
```cpp
void parallelPrimeSearch(int start, int end)
{
    double start_time, end_time;
    start_time = MPI_Wtime();

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int range = end - start + 1;
    int chunk = range / size;
    int remainder = range % size;
    int local_start = start + rank * chunk + (rank < remainder ? rank : remainder);
    int local_end = local_start + chunk - 1 + (rank < remainder ? 1 : 0);

    vector<int> local_primes = findPrimes(local_start, local_end);
    int local_count = local_primes.size();

    vector<int> counts(size);
    MPI_Gather(&local_count, 1, MPI_INT, counts.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

    vector<int> displs(size);
    int total = 0;
    if (rank == 0)
    {
        for (int i = 0; i < size; ++i)
        {
            displs[i] = total;
            total += counts[i];
        }
    }

    vector<int> all_primes(total);
    MPI_Gatherv(local_primes.data(), local_count, MPI_INT, all_primes.data(), counts.data(), displs.data(), MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0)
    {
        end_time = MPI_Wtime();

        cout << "Execution time: " << (end_time - start_time) * 1000 << " ms\n";
        
        FILE *out = fopen("out.txt", "w");
        fprintf(out, "Prime Number Search Results:\n");
        fprintf(out, "Found %d primes between %d and %d\n", total, start, end);
        for (int i = 0; i < (total > 100 ? 10 : total); ++i)
            fprintf(out, "%d ", all_primes[i]);
        if (total > 100)
            fprintf(out, "... (and %d more)", total - 10);
        fclose(out);
    }
}
```

This is the main function that implements the parallel prime number search:

- **Step 1**: Initializes timing and gets process information (rank and size)
  - Each process finds its rank within the MPI communicator
  - The total number of processes is stored in `size`

- **Step 2**: Divides the search range among processes
  - Calculates range size and chunk size per process
  - Handles uneven distributions with the remainder
  - Each process calculates its local start and end points

- **Step 3**: Each process finds primes in its assigned range
  - Calls `findPrimes` on the local range
  - Counts how many primes were found locally

- **Step 4**: Gathers counts from all processes
  - Process 0 collects the count of primes found by each process
  - Creates displacement array for the subsequent `Gatherv` operation

- **Step 5**: Combines all local results
  - Process 0 prepares a vector to hold all prime numbers
  - All processes send their local primes to process 0 using `MPI_Gatherv`

- **Step 6**: Process 0 handles the final output
  - Calculates and displays execution time
  - Writes the results to an output file
  - Limits output for large results to avoid excessive file sizes

## 2. MPI Functions In More Detail

### MPI_Gather
```cpp
MPI_Gather(&local_count, 1, MPI_INT, counts.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);
```
This function:
- Collects the local prime counts from all processes
- Each process sends one integer (its local count)
- Process 0 receives all counts into the `counts` array
- This information is needed to prepare for the `MPI_Gatherv` operation

### MPI_Gatherv
```cpp
MPI_Gatherv(local_primes.data(), local_count, MPI_INT, all_primes.data(), counts.data(), displs.data(), MPI_INT, 0, MPI_COMM_WORLD);
```
This function:
- Collects variable-sized arrays of prime numbers from all processes
- Each process sends its array of prime numbers (`local_primes`) with `local_count` elements
- Process 0 receives all arrays into the `all_primes` array
- The `counts` array specifies how many elements to receive from each process
- The `displs` array specifies where in the output array to place the data from each process

## 3. Step-by-Step Example

Let's walk through a simple example with 4 processes searching for primes in the range [1, 20]:

### Initial Setup:
- 4 processes (ranks 0-3)
- Search range: 1 to 20 (20 numbers)
- Range size = 20
- Chunk size = 20 ÷ 4 = 5 numbers per process

### Range Distribution:
- Process 0: numbers 1-5
- Process 1: numbers 6-10
- Process 2: numbers 11-15
- Process 3: numbers 16-20

### Local Prime Search:
- Process 0 finds: 2, 3, 5 (3 primes)
- Process 1 finds: 7 (1 prime)
- Process 2 finds: 11, 13 (2 primes)
- Process 3 finds: 17, 19 (2 primes)

### Count Gathering:
- Process 0 collects counts: [3, 1, 2, 2]
- Creates displacements: [0, 3, 4, 6]
- Total primes = 8

### Prime Gathering:
- All processes send their primes to process 0
- Process 0 combines them into: [2, 3, 5, 7, 11, 13, 17, 19]

### Final Output:
- Process 0 writes the results to the output file
- Reports: "Found 8 primes between 1 and 20"
- Lists all 8 primes

This demonstrates how the work is evenly distributed across processes, with each process responsible for checking primality in its assigned range. The final result collects all found primes into a single sorted array.

## 4. Performance Considerations

1. **Work Distribution**
   - The algorithm uses a block partitioning approach with careful handling of remainder elements
   - Each process gets roughly equal work, which is important for load balancing

2. **Communication Pattern**
   - Minimal communication: only two collective operations (Gather and Gatherv)
   - No process-to-process communication, only process-to-root communication

3. **Scalability**
   - For large ranges, adding more processes effectively divides the work
   - However, there is diminishing returns as gather operations may become bottlenecks
   - For very large ranges, a hierarchical approach might be more efficient

4. **Memory Usage**
   - Each process only allocates memory for its local primes
   - Only process 0 needs to allocate memory for all primes

5. **Optimization Opportunities**
   - For extremely large numbers, a more efficient primality test could be used
   - The Sieve of Eratosthenes could be implemented in parallel for potentially better performance
   - I/O operations could be optimized further for very large result sets

## 5. Complexity Analysis

### Time Complexity
- **Sequential**: O(N√N) where N is the range size (end - start + 1)
  - Each number requires O(√n) operations to check primality
- **Parallel**: O(N√N/p) where p is the number of processes
  - Each process handles approximately N/p numbers
  - Ideal speedup is linear with the number of processes

### Space Complexity
- **Sequential**: O(π(N)) where π(N) is the prime-counting function
  - Storage needed for all prime numbers in the range
- **Parallel**: O(π(N)/p) per process plus O(π(N)) for the root process
  - Each process stores only its local primes
  - Root process ultimately stores all primes

### Communication Costs
- **Gather operation**: O(p) small messages + O(π(N)) total data
  - Each process sends one integer (count) to the root
- **Gatherv operation**: O(π(N)) total data transferred
  - Each process sends its local primes to the root
  - Communication is one-sided (process→root only)
  - No intermediate communication during computation
