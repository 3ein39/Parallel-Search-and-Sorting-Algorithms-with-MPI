## 1. Core Functions Explanation

### `get_largest` Function
```cpp
int get_largest(const vector<int>& numbers) {
    if (numbers.empty()) return 0;
    return *max_element(numbers.begin(), numbers.end());
}
```
This helper function finds the largest value in an array:
- Uses the C++ STL `max_element` algorithm to efficiently find the maximum value
- Returns 0 if the array is empty
- The largest value is needed to determine how many digit positions to process

### `digit_count` Function
```cpp
int digit_count(int value) {
    if (value == 0) return 1;
    int digits = 0;
    while (value > 0) {
        value /= 10;
        digits++;
    }
    return digits;
}
```
This function calculates the number of decimal digits in a number:
- Special case for 0, which has 1 digit
- Counts digits by repeatedly dividing by 10
- Used to determine how many iterations the radix sort needs to perform

### `distribute_by_digit` Function
```cpp
void distribute_by_digit(const vector<int>& input, int divisor, int size,
                         vector<vector<int>>& buckets, vector<int>& counts) {
    fill(counts.begin(), counts.end(), 0);
    for (auto num : input) {
        int digit = (num / divisor) % BASE;
        int proc = (digit * size) / BASE;
        if (proc >= size) proc = size - 1;
        counts[proc]++;
    }

    for (int i = 0; i < size; ++i) {
        buckets[i].reserve(counts[i]);
    }

    for (auto num : input) {
        int digit = (num / divisor) % BASE;
        int proc = (digit * size) / BASE;
        if (proc >= size) proc = size - 1;
        buckets[proc].push_back(num);
    }
}
```

This function distributes numbers into buckets based on a specific digit position:

- **Input Parameters**:
  - `input`: The array of numbers to distribute
  - `divisor`: Determines which digit position to examine (1 for ones place, 10 for tens place, etc.)
  - `size`: The number of MPI processes (determines the number of buckets)
  - `buckets`: Vector of vectors to store distributed numbers
  - `counts`: Counts how many numbers go into each bucket

- **Algorithm Steps**:
  1. Reset the counts for all buckets to zero
  2. First pass: Count how many numbers will go to each process bucket based on the specific digit
  3. Reserve appropriate space in each bucket for efficiency
  4. Second pass: Actually place the numbers into the appropriate buckets

- **Bucket Assignment Strategy**:
  - Extracts the digit at position defined by `divisor`: `(num / divisor) % BASE`
  - Maps the digit (0-9) to a process number using: `proc = (digit * size) / BASE`
  - Ensures the process number is valid with a bounds check

## 2. The Main Function `runRadixSort`

```cpp
bool runRadixSort(const char* inputFile, const char* outputFile, int rank, int size, MPI_Comm comm) {
    vector<int> input_array;
    int array_size = 0;

    // Process 0 reads input from file
    if (rank == 0) {
        ifstream input_file(inputFile);
        if (!input_file.is_open()) {
            cerr << "Error: Unable to open " << inputFile << endl;
            return false;
        }

        int value;
        while (input_file >> value) {
            input_array.push_back(value);
        }
        array_size = input_array.size();
        input_file.close();
    }

    // Share array size with all processes
    MPI_Bcast(&array_size, 1, MPI_INT, 0, comm);

    // Calculate size of each process's partition
    int partition_size = array_size / size + (rank < array_size % size ? 1 : 0);
    vector<int> partition(partition_size);

    // Prepare for scattering data
    vector<int> counts_to_send(size), offsets(size);
    if (rank == 0) {
        int offset = 0;
        for (int i = 0; i < size; ++i) {
            counts_to_send[i] = array_size / size + (i < array_size % size ? 1 : 0);
            offsets[i] = offset;
            offset += counts_to_send[i];
        }
    }

    // Distribute input data to processes
    MPI_Scatterv(input_array.data(), counts_to_send.data(), offsets.data(), MPI_INT,
                 partition.data(), partition_size, MPI_INT, 0, comm);

    // Determine global maximum
    int local_max = get_largest(partition);
    int global_max;
    MPI_Allreduce(&local_max, &global_max, 1, MPI_INT, MPI_MAX, comm);
    int max_digits = digit_count(global_max);

    vector<int> counts_to_send_proc(size);
    vector<int> counts_to_recv(size);

    // Process each digit
    for (int digit_pos = 0; digit_pos < max_digits; ++digit_pos) {
        int divisor = 1;
        for (int i = 0; i < digit_pos; ++i) divisor *= 10;

        // Distribute numbers to buckets
        vector<vector<int>> buckets(size);
        distribute_by_digit(partition, divisor, size, buckets, counts_to_send_proc);

        // Share send counts
        MPI_Alltoall(counts_to_send_proc.data(), 1, MPI_INT, counts_to_recv.data(), 1, MPI_INT, comm);

        // Calculate displacements
        vector<int> send_offsets(size), recv_offsets(size);
        send_offsets[0] = recv_offsets[0] = 0;
        for (int i = 1; i < size; ++i) {
            send_offsets[i] = send_offsets[i - 1] + counts_to_send_proc[i - 1];
            recv_offsets[i] = recv_offsets[i - 1] + counts_to_recv[i - 1];
        }

        // Prepare send buffer
        int total_to_send = send_offsets[size - 1] + counts_to_send_proc[size - 1];
        vector<int> send_data(total_to_send);
        int pos = 0;
        for (int i = 0; i < size; ++i) {
            copy(buckets[i].begin(), buckets[i].end(), send_data.begin() + pos);
            pos += buckets[i].size();
        }

        // Update partition size
        partition_size = recv_offsets[size - 1] + counts_to_recv[size - 1];
        vector<int> recv_data(partition_size);

        // Exchange data between processes
        MPI_Alltoallv(send_data.data(), counts_to_send_proc.data(), send_offsets.data(), MPI_INT,
                      recv_data.data(), counts_to_recv.data(), recv_offsets.data(), MPI_INT,
                      comm);

        partition = move(recv_data);

        // Perform local counting sort if needed
        if (size < BASE) {
            vector<int> digit_counts(BASE, 0);
            vector<int> sorted(partition_size);
            for (auto num : partition) {
                int digit = (num / divisor) % BASE;
                digit_counts[digit]++;
            }
            for (int i = 1; i < BASE; ++i) {
                digit_counts[i] += digit_counts[i - 1];
            }
            for (int i = partition_size - 1; i >= 0; --i) {
                int digit = (partition[i] / divisor) % BASE;
                sorted[--digit_counts[digit]] = partition[i];
            }
            partition = move(sorted);
        }
    }

    // Gather partition sizes
    vector<int> final_counts(size);
    MPI_Allgather(&partition_size, 1, MPI_INT, final_counts.data(), 1, MPI_INT, comm);

    vector<int> final_offsets(size);
    final_offsets[0] = 0;
    for (int i = 1; i < size; ++i) {
        final_offsets[i] = final_offsets[i - 1] + final_counts[i - 1];
    }

    // Collect final sorted array
    if (rank == 0) {
        input_array.resize(array_size);
    }

    MPI_Gatherv(partition.data(), partition_size, MPI_INT, input_array.data(), final_counts.data(),
                final_offsets.data(), MPI_INT, 0, comm);

    // Write sorted array to output file
    if (rank == 0) {
        ofstream output_file(outputFile);
        if (!output_file.is_open()) {
            cerr << "Error: Unable to open " << outputFile << endl;
            return false;
        } else {
            for (auto num : input_array) {
                output_file << num << " ";
            }
            output_file << endl;
            output_file.close();
        }
    }

    return true;
}
```

This main function implements the parallel radix sort algorithm:

### Initialization Phase (Input Reading)
- Process 0 reads input data from the specified file
- The array size is broadcasted to all processes
- Each process calculates its partition size, handling uneven divisions
- Input data is distributed using `MPI_Scatterv`

### Global Maximum Determination
- Each process finds its local maximum value
- `MPI_Allreduce` with `MPI_MAX` is used to find the global maximum
- The maximum number of digits is calculated to determine iteration count

### Main Radix Sort Loop
For each digit position (starting from least significant):
1. **Bucket Distribution**:
   - Numbers are distributed into buckets based on the current digit
   - Each bucket corresponds to a range of digits to be sent to a specific process

2. **Communication Preparation**:
   - Each process informs others how much data it will send (`MPI_Alltoall`)
   - Send and receive displacement arrays are calculated

3. **Data Exchange**:
   - Data is exchanged between all processes using `MPI_Alltoallv`
   - After this exchange, each process has numbers with the same digit range

4. **Local Sorting** (when number of processes < 10):
   - If the number of processes is less than the BASE (10),
     a local counting sort is applied to sort by the current digit

### Result Collection
- Final partition sizes are exchanged using `MPI_Allgather`
- Process 0 collects all sorted partitions using `MPI_Gatherv`
- Process 0 writes the sorted array to the output file

## 3. Key MPI Functions In More Detail

### MPI_Alltoall
```cpp
MPI_Alltoall(counts_to_send_proc.data(), 1, MPI_INT, counts_to_recv.data(), 1, MPI_INT, comm);
```
This function:
- Performs an all-to-all exchange of data between processes
- Each process sends one integer to every other process
- Used to share information about how many elements each process will send in the next step
- Essential for preparing the subsequent `MPI_Alltoallv` operation

### MPI_Alltoallv
```cpp
MPI_Alltoallv(send_data.data(), counts_to_send_proc.data(), send_offsets.data(), MPI_INT,
              recv_data.data(), counts_to_recv.data(), recv_offsets.data(), MPI_INT,
              comm);
```
This function:
- Exchanges variable-sized data blocks between all processes
- Each process sends a different amount of data to each recipient process
- `counts_to_send_proc` specifies how many elements to send to each process
- `counts_to_recv` specifies how many elements to receive from each process
- `send_offsets` and `recv_offsets` specify where in the buffers the data for each process begins
- Critical for redistributing numbers based on their digits

### MPI_Allreduce
```cpp
MPI_Allreduce(&local_max, &global_max, 1, MPI_INT, MPI_MAX, comm);
```
This function:
- Combines values from all processes and distributes the result back to all processes
- Uses the MPI_MAX operation to find the maximum value across all processes
- Ensures all processes know the largest number, which determines the number of iterations needed

## 4. Step-by-Step Example

Let's trace through a simple example with 4 processes sorting an array of 12 integers:

### Initial Setup:
- Input array: `[329, 457, 657, 839, 436, 720, 355, 212, 9, 47, 78, 125]`
- 4 processes (ranks 0-3)

### Data Distribution:
After `MPI_Scatterv`:
- Process 0: `[329, 457, 657]`
- Process 1: `[839, 436, 720]`
- Process 2: `[355, 212, 9]`
- Process 3: `[47, 78, 125]`

### Global Maximum:
- Local maximums: 657, 839, 355, 125
- Global maximum after `MPI_Allreduce`: 839
- Number of digits: 3 (meaning 3 iterations of radix sort)

### First Iteration (Ones Place):
1. Extract the ones digit and distribute to buckets:
   - Digits ending in 0: 720
   - Digits ending in 5: 355, 125
   - Digits ending in 7: 457, 657, 47
   - Digits ending in 8: 78
   - Digits ending in 9: 329, 839, 9

2. After redistribution with `MPI_Alltoallv`:
   - Process 0 might have: `[720]` (digit 0)
   - Process 1 might have: `[355, 125]` (digits 5)
   - Process 2 might have: `[457, 657, 47, 78]` (digits 7-8)
   - Process 3 might have: `[329, 839, 9]` (digits 9)

3. After local sorting (if needed):
   - Each process has its local data sorted by the ones digit

### Second Iteration (Tens Place):
1. Extract the tens digit and distribute to buckets
2. After redistribution and local sorting:
   - Numbers are now sorted by tens and ones digits

### Third Iteration (Hundreds Place):
1. Extract the hundreds digit and distribute to buckets
2. After redistribution and local sorting:
   - Numbers are now fully sorted

### Result Collection:
- Each process has part of the sorted array
- After `MPI_Gatherv`, process 0 has the complete sorted array:
  `[9, 47, 78, 125, 212, 329, 355, 436, 457, 657, 720, 839]`

## 5. Performance Considerations

1. **Communication Overhead**
   - The algorithm requires multiple rounds of all-to-all communication
   - For each digit position, every process communicates with every other process
   - This can be a performance bottleneck for large process counts

2. **Load Balancing**
   - The distribution of numbers can lead to imbalanced workloads
   - Some digit values may be more common than others
   - The local counting sort helps mitigate this issue for small process counts

3. **Memory Usage**
   - Each process needs memory for:
     - Its local partition
     - Buckets for redistribution
     - Send and receive buffers
   - Overall memory usage is O(n + p), where n is the data size and p is the process count

4. **Scalability**
   - The algorithm works well for moderate process counts
   - Performance may degrade for very large process counts due to communication overhead
   - Best suited for situations where the digit range (BASE) is larger than the process count

5. **Optimization Opportunities**
   - Hybrid approach with OpenMP for local sorting
   - Using non-blocking communication to overlap computation and communication
   - Implementing a hierarchical approach for large process counts
   - Custom distribution strategies for skewed data distributions

## 6. Complexity Analysis

### Time Complexity
- **Sequential Radix Sort**: O(d × n) where:
  - n is the array size
  - d is the number of digits in the maximum number
- **Parallel Radix Sort**: O(d × n/p + d × p) where:
  - p is the number of processes
  - Local processing: O(d × n/p) per process
  - All-to-all communication: O(d × p) per process

### Space Complexity
- **Sequential**: O(n) for auxiliary arrays in counting sort
- **Parallel**: O(n/p) per process for local data storage
  - Additional O(p) space per process for communication buffers
  - O(BASE) space for digit counts (constant as BASE=10)
  - Root process: O(n) for final result array

### Communication Costs
- **Per Digit Iteration**:
  - All-to-all counts: O(p²) small messages
  - All-to-all data: O(n) total data transferred
- **Total Communication Volume**: O(d × n) over all iterations
- **Latency Impact**: O(d × p) startup costs across all iterations
- **Data Exchange Pattern**: Non-uniform, dependent on data distribution

