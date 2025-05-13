## 1. Core Functions Explanation

### `compareAndSwap` Function
```cpp
void compareAndSwap(vector<int>& arr, int i, int j, bool dir) {
    if (dir == (arr[i] > arr[j])) {
        swap(arr[i], arr[j]);
    }
}
```
This is a helper function that compares and potentially swaps two elements in an array:
- `dir` controls the sorting direction (true for ascending, false for descending)
- If `dir` is true, it swaps when `arr[i] > arr[j]` (ascending order)
- If `dir` is false, it swaps when `arr[i] < arr[j]` (descending order)

### `bitonicMerge` Function
```cpp
void bitonicMerge(vector<int>& local_data, vector<int>& recv_data, bool dir) {
    int n = local_data.size();
    
    for (int i = 0; i < n; i++) {
        if (dir == (local_data[i] > recv_data[i])) {
            swap(local_data[i], recv_data[i]);
        }
    }
}
```

This function implements the core "compare-split" operation of bitonic sort:
- It compares corresponding elements from two arrays (`local_data` and `recv_data`)
- It ensures `local_data` contains smaller elements and `recv_data` contains larger ones if `dir` is true
- It does the opposite if `dir` is false
- This effectively performs one step of the bitonic merge operation across distributed data

### `bitonicSortParallel` Function
```cpp
void bitonicSortParallel(vector<int>& local_data, int total_n, int rank, int size, MPI_Comm comm) {
    int local_n = local_data.size();
    vector<int> recv_buffer(local_n);
    
    // Each process starts with a locally sorted array
    sort(local_data.begin(), local_data.end());
    
    // Main bitonic sort algorithm
    for (int step = 1; step < size; step = step << 1) {
        for (int substep = step; substep > 0; substep = substep >> 1) {
            // Find partner process for this substep
            int partner = rank ^ substep;
            
            // Skip invalid partners
            if (partner >= size) continue;
            
            // Determine sort direction
            bool dir = ((rank / (2 * substep)) % 2 == 0);
            
            // Exchange data with partner
            MPI_Sendrecv(local_data.data(), local_n, MPI_INT, partner, 0,
                        recv_buffer.data(), local_n, MPI_INT, partner, 0,
                        comm, MPI_STATUS_IGNORE);
            
            // Determine which half to keep based on rank and partner
            if (rank < partner) {
                // Compare-split operation: keep smaller elements if rank < partner
                bitonicMerge(local_data, recv_buffer, dir);
            } else {
                // Compare-split operation: keep larger elements if rank > partner
                bitonicMerge(recv_buffer, local_data, dir);
            }
        }
    }
}
```

This is the heart of the parallel bitonic sort algorithm:

- **Step 1**: Each process first sorts its local data chunk
- **Step 2**: The algorithm works in phases (outer loop with `step`)
  - `step` starts at 1 and doubles each time (1, 2, 4, 8, ...)
  - There are log₂(size) phases total
  
- **Step 3**: Each phase consists of multiple merge stages (inner loop with `substep`)
  - `substep` starts at `step` and halves each time
  - This implements the bitonic merge network pattern
  
- **Step 4**: For each stage:
  - Processes are paired using bitwise XOR (`^`) operator: `partner = rank ^ substep`
  - The sort direction is determined based on process rank: `dir = ((rank / (2 * substep)) % 2 == 0)`
  - Processes exchange data using `MPI_Sendrecv`
  - Each process keeps either the smaller or larger half of data based on the comparison with its partner

## 2. The Wrapper Function `runBitonicSort`

This function manages the entire sorting process:

1. **Input Processing**: 
   - Process 0 reads the input data and checks if the number of processes is a power of 2
   - It broadcasts the size of the array to all processes using `MPI_Bcast`

2. **Data Distribution**:
   - Calculates how many elements each process will handle
   - Ensures each process gets equal work by padding the array
   - Distributes data using `MPI_Scatter` to all processes

3. **Sorting**:
   - Calls the `bitonicSortParallel` function to perform actual sorting
   - Times the execution using `MPI_Wtime`

4. **Result Collection**:
   - Gathers the sorted chunks back to process 0 using `MPI_Gather`
   - Process 0 removes padding values and verifies the sorting
   - Writes the result to the output file

## 3. Step-by-Step Example

Let's walk through a simple example with 4 processes (ranks 0-3) sorting an array of 16 elements:

### Initial Setup:
- Input array: `[14, 10, 3, 1, 7, 9, 8, 2, 11, 6, 15, 5, 13, 4, 12, 0]`
- 4 processes, each handling 4 elements

### Data Distribution Phase:
After `MPI_Scatter`:
- Process 0 has: `[14, 10, 3, 1]`
- Process 1 has: `[7, 9, 8, 2]`
- Process 2 has: `[11, 6, 15, 5]`
- Process 3 has: `[13, 4, 12, 0]`

### Local Sort:
After `sort()` within each process:
- Process 0 has: `[1, 3, 10, 14]`  (ascending)
- Process 1 has: `[2, 7, 8, 9]`    (ascending)
- Process 2 has: `[5, 6, 11, 15]`  (ascending)
- Process 3 has: `[0, 4, 12, 13]`  (ascending)

### Bitonic Sort Phases:
The algorithm proceeds through phases based on powers of 2:

#### Phase 1 (step=1):
- Substep 1: 
  - Partners: 0↔1, 2↔3
  - Directions: Process 0 (ascending), Process 2 (ascending)
  - After exchange:
    - Process 0 has: `[1, 3, 10, 14]` (keeps smaller elements with Process 1)
    - Process 1 has: `[2, 7, 8, 9]` (keeps larger elements with Process 0)
    - Process 2 has: `[0, 4, 5, 6]` (keeps smaller elements with Process 3)
    - Process 3 has: `[11, 12, 13, 15]` (keeps larger elements with Process 2)

#### Phase 2 (step=2):
- Substep 2:
  - Partners: 0↔2, 1↔3
  - Directions: Process 0 (ascending), Process 1 (descending)
  - After exchange:
    - Process 0 has: `[0, 1, 3, 4]` (keeps smaller elements)
    - Process 2 has: `[5, 6, 10, 14]` (keeps larger elements)
    - Process 1 has: `[9, 11, 12, 13]` (keeps larger elements due to descending)
    - Process 3 has: `[2, 7, 8, 15]` (keeps smaller elements due to descending)

- Substep 1:
  - Partners: 0↔1, 2↔3
  - Directions vary based on rank
  - After exchange:
    - Process 0 has: `[0, 1, 3, 4]` (fully sorted segment)
    - Process 1 has: `[7, 8, 9, 11]` (fully sorted segment)
    - Process 2 has: `[5, 6, 10, 14]` (fully sorted segment)
    - Process 3 has: `[2, 12, 13, 15]` (fully sorted segment)

### Result Collection:
After `MPI_Gather` and reordering:
- Final sorted array: `[0, 1, 3, 4, 7, 8, 9, 11, 5, 6, 10, 14, 2, 12, 13, 15]`

Note: The example shows the general pattern, but the actual MPI implementation might have slight differences due to how exactly the bitonicMerge function handles the data and direction.

## 4. Key MPI Functions In More Detail

### MPI_Sendrecv
```cpp
MPI_Sendrecv(local_data.data(), local_n, MPI_INT, partner, 0,
            recv_buffer.data(), local_n, MPI_INT, partner, 0,
            comm, MPI_STATUS_IGNORE);
```
This is a crucial function that simultaneously:
- Sends `local_data` to the `partner` process
- Receives data from the `partner` into `recv_buffer`
- The "0" values are message tags that help identify messages
- Using a single function for both send and receive prevents deadlocks that could occur if separate send/receive calls were used

### MPI_Scatter
```cpp
MPI_Scatter(rank == 0 ? global_array.data() : nullptr,
           elements_per_process, MPI_INT,
           local_data.data(), elements_per_process, MPI_INT,
           0, comm);
```
This function:
- Divides `global_array` into equal-sized chunks
- Distributes one chunk to each process's `local_data`
- Only process 0 needs a valid send buffer; others can pass nullptr
- Every process gets exactly `elements_per_process` integers

### MPI_Gather
```cpp
MPI_Gather(local_data.data(), elements_per_process, MPI_INT,
          result.data(), elements_per_process, MPI_INT,
          0, comm);
```
This function:
- Collects `elements_per_process` integers from each process's `local_data`
- Combines them into a single array `result` on process 0
- It's the reverse operation of MPI_Scatter

These MPI functions, combined with the bitonic sort algorithm, create an efficient parallel sorting implementation that distributes work across multiple processes, significantly speeding up the sorting of large datasets.

## 5. Complexity Analysis

### Time Complexity
- **Sequential Bitonic Sort**: O(n log² n) where n is the array size
- **Parallel Bitonic Sort**: O((n/p) log² n) where p is the number of processes
  - Local sorting: O((n/p) log(n/p)) per process
  - Parallel merging: O((n/p) log p) per process
  - The log² n term comes from log n phases with log n steps each

### Space Complexity
- **Sequential**: O(n) for array storage, in-place operations thereafter
- **Parallel**: O(n/p) per process for local data storage
  - Additional O(n/p) space for communication buffers
  - Total memory usage across all processes remains O(n)

### Communication Costs
- **Number of Messages**: O(p log p) point-to-point messages
- **Message Size**: O(n/p) elements per message
- **Communication Volume**: O(n log p) total data transferred
- **Communication Pattern**: Regular and predictable
  - Each process communicates with log p partners
  - Communication partners are determined by bitwise operations on rank
