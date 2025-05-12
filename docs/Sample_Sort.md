## 1. Core Functions Explanation

### `choose_pivot` Function
```cpp
int choose_pivot(int *arr, int low, int high)
{
    // Find the middle element
    int mid = low + (high - low) / 2;

    // Sort low, mid, high elements (ensures median is in the middle)
    if (arr[low] > arr[mid])
        swap(arr[low], arr[mid]);
    if (arr[mid] > arr[high])
        swap(arr[mid], arr[high]);
    if (arr[low] > arr[mid])
        swap(arr[low], arr[mid]);

    // Move the pivot to the high position
    swap(arr[mid], arr[high]);

    // Return the pivot value
    return arr[high];
}
```
This function implements the median-of-three pivot selection strategy:
- Identifies the middle element between `low` and `high`
- Sorts the elements at low, mid, and high positions to find the median
- Places the pivot (median value) at the high position
- Returns the pivot value
- This strategy helps avoid worst-case performance in quicksort

### `quicksort` Function
```cpp
void quicksort(int *arr, int low, int high)
{
    if (low < high)
    {
        // Choose a better pivot using median-of-three strategy
        int pivot = choose_pivot(arr, low, high);
        int i = low - 1;

        for (int j = low; j < high; j++)
        {
            if (arr[j] <= pivot)
            {
                i++;
                swap(arr[i], arr[j]);
            }
        }
        swap(arr[i + 1], arr[high]);

        int pivot_pos = i + 1;
        quicksort(arr, low, pivot_pos - 1);
        quicksort(arr, pivot_pos + 1, high);
    }
}
```
This function implements the quicksort algorithm with median-of-three pivot selection:
- Uses `choose_pivot` to find a good pivot element
- Partitions the array around the pivot
- Recursively sorts the subarrays before and after the pivot
- The median-of-three strategy improves performance for partially sorted inputs

### `calculate_counts_and_displs` Function
```cpp
void calculate_counts_and_displs(int *send_counts, int *send_displs, int size, int total_size)
{
    int base = total_size / size, rem = total_size % size;
    for (int i = 0; i < size; i++)
    {
        send_counts[i] = base + (i < rem ? 1 : 0);
        send_displs[i] = (i == 0) ? 0 : send_displs[i - 1] + send_counts[i - 1];
    }
}
```
This utility function calculates:
- How many elements to send to each process (`send_counts`)
- Where in the array each process's data starts (`send_displs`)
- Handles the case where the total size is not evenly divisible by the number of processes
- Used for both initial data distribution and final result gathering

### `select_local_samples` Function
```cpp
void select_local_samples(int *local_array, int local_size, int *local_samples, int sample_size)
{
    for (int i = 0; i < sample_size; i++)
    {
        local_samples[i] = local_array[(i + 1) * local_size / (sample_size + 1)];
    }
}
```
This function:
- Selects representative sample elements from the local array
- Uses a regular sampling strategy to pick elements at approximately equal distances
- These samples will be used to determine the global splitters

### `select_splitters` Function
```cpp
void select_splitters(int *samples, int total_samples, int *splitters, int size)
{
    quicksort(samples, 0, total_samples - 1);
    for (int i = 0; i < size - 1; i++)
    {
        splitters[i] = samples[(i + 1) * (total_samples / size)];
    }
    splitters[size - 1] = INT_MAX;
}
```
This function:
- Sorts all collected samples
- Selects `size-1` splitters at regular intervals from the sorted samples
- Sets the last splitter to INT_MAX to ensure all remaining elements go to the last bucket
- These splitters will partition the data range into `size` approximately equal-sized buckets

### `partition_data` Function
```cpp
void partition_data(int *local_array, int local_size, int *splitters, int size,
                    int *partition_counts, int *send_buf, int *send_displs)
{
    int *temp_displs = (int *)calloc(size, sizeof(int));
    for (int i = 0; i < local_size; i++)
    {
        int j = 0;
        while (j < size - 1 && local_array[i] > splitters[j])
            j++;
        partition_counts[j]++;
    }

    send_displs[0] = 0;
    for (int i = 1; i < size; i++)
        send_displs[i] = send_displs[i - 1] + partition_counts[i - 1];

    for (int i = 0; i < size; i++)
        temp_displs[i] = send_displs[i];

    for (int i = 0; i < local_size; i++)
    {
        int j = 0;
        while (j < size - 1 && local_array[i] > splitters[j])
            j++;
        send_buf[temp_displs[j]++] = local_array[i];
    }
    free(temp_displs);
}
```
This function:
1. **First pass**: Counts how many elements will go to each process based on the splitters
2. **Calculates displacements**: Determines where in the send buffer to place elements for each process
3. **Second pass**: Places elements in the send buffer according to which process they'll be sent to
4. Uses a temporary displacement array to track current positions while filling the send buffer

### `gather_sorted_data` Function
```cpp
void gather_sorted_data(int *recv_buf, int recv_size, int rank, int size,
                        int *final_array, int *all_sizes, int *displs)
{
    MPI_Gather(&recv_size, 1, MPI_INT, all_sizes, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0)
    {
        displs[0] = 0;
        for (int i = 1; i < size; i++)
            displs[i] = displs[i - 1] + all_sizes[i - 1];
    }

    MPI_Gatherv(recv_buf, recv_size, MPI_INT,
                final_array, all_sizes, displs, MPI_INT,
                0, MPI_COMM_WORLD);
}
```
This function:
- Gathers the sizes of all sorted partitions using `MPI_Gather`
- Calculates displacement array for the final gather operation
- Uses `MPI_Gatherv` to collect all sorted data into one array on process 0

## 2. The Main Function `runSampleSort`

```cpp
bool runSampleSort(const char *inputFile, const char *outputFile, int rank, int size, MPI_Comm comm)
{
    // Variables
    int *array = NULL;
    int array_size = 0;
    bool is_error = false;

    // Root process reads input file
    if (rank == 0)
    {
        // Read data from input file
        ifstream file(inputFile);
        vector<int> data;
        int el;
        while (file >> el)
        {
            data.push_back(el);
        }
        file.close();

        // Set array size
        array_size = data.size();

        // Check if size is valid
        if (array_size <= 0)
        {
            cout << "Error: Invalid input array size\n";
            is_error = true;
        }

        // Allocate array and copy data
        if (!is_error)
        {
            array = (int *)malloc(array_size * sizeof(int));
            if (!array)
            {
                cout << "Error: Memory allocation failed\n";
                is_error = true;
            }
            else
            {
                for (int i = 0; i < array_size; i++)
                {
                    array[i] = data[i];
                }

                // Also write to output file
                ofstream outFile(outputFile);
                outFile << "Unsorted array: ";
                for (int i = 0; i < array_size; i++)
                {
                    outFile << array[i] << " ";
                }
                outFile << endl;
                outFile.close();
            }
        }
    }

    // Broadcast error status
    MPI_Bcast(&is_error, 1, MPI_C_BOOL, 0, comm);

    if (is_error)
    {
        return false;
    }

    // Broadcast array size
    MPI_Bcast(&array_size, 1, MPI_INT, 0, comm);

    // Start timing
    MPI_Barrier(comm);
    double start_time = MPI_Wtime();

    // Calculate counts and displacements for scattering data
    int *send_counts = (int *)malloc(size * sizeof(int));
    int *send_displs = (int *)malloc(size * sizeof(int));
    calculate_counts_and_displs(send_counts, send_displs, size, array_size);

    // Allocate local array for each process
    int local_size = send_counts[rank];
    int *local_array = (int *)malloc(local_size * sizeof(int));

    // Scatter data to all processes
    MPI_Scatterv(array, send_counts, send_displs, MPI_INT,
                 local_array, local_size, MPI_INT, 0, comm);

    // Sort local data
    quicksort(local_array, 0, local_size - 1);
    
    // Sample selection
    int samples_per_process = std::max(1, (int)log2(size));
    int sample_size = samples_per_process * size;
    int *local_samples = (int *)malloc(sample_size * sizeof(int));
    select_local_samples(local_array, local_size, local_samples, sample_size);

    int *samples = NULL;
    if (rank == 0)
    {
        samples = (int *)malloc(sample_size * size * sizeof(int));
    }
    MPI_Gather(local_samples, sample_size, MPI_INT,
               samples, sample_size, MPI_INT, 0, comm);

    // Select and broadcast splitters
    int *splitters = (int *)malloc(size * sizeof(int));
    if (rank == 0)
    {
        select_splitters(samples, sample_size * size, splitters, size);
    }
    MPI_Bcast(splitters, size, MPI_INT, 0, comm);

    // Partition data based on splitters
    int *partition_counts = (int *)calloc(size, sizeof(int));
    int *send_buf = (int *)malloc(local_size * sizeof(int));
    int *send_displs_local = (int *)calloc(size, sizeof(int));
    partition_data(local_array, local_size, splitters, size,
                   partition_counts, send_buf, send_displs_local);
    
    // Exchange partition counts
    int *recv_counts = (int *)malloc(size * sizeof(int));
    MPI_Alltoall(partition_counts, 1, MPI_INT,
                 recv_counts, 1, MPI_INT, comm);

    // Calculate total data to receive
    int recv_size = 0;
    for (int i = 0; i < size; i++)
    {
        recv_size += recv_counts[i];
    }

    // Prepare buffers for receiving data
    int *recv_buf = (int *)malloc(recv_size * sizeof(int));
    int *recv_displs = (int *)calloc(size, sizeof(int));
    for (int i = 1; i < size; i++)
    {
        recv_displs[i] = recv_displs[i - 1] + recv_counts[i - 1];
    }

    // Exchange partitioned data
    MPI_Alltoallv(send_buf, partition_counts, send_displs_local, MPI_INT,
                  recv_buf, recv_counts, recv_displs, MPI_INT, comm);

    // Sort received data
    quicksort(recv_buf, 0, recv_size - 1);

    // Gather final sorted data
    int *all_sizes = NULL;
    int *displs = NULL;
    if (rank == 0)
    {
        all_sizes = (int *)malloc(size * sizeof(int));
        displs = (int *)malloc(size * sizeof(int));

        // Ensure array is allocated for final sorted data
        if (array == NULL)
        {
            array = (int *)malloc(array_size * sizeof(int));
        }
    }

    gather_sorted_data(recv_buf, recv_size, rank, size, array, all_sizes, displs);

    // End timing
    double end_time = MPI_Wtime();
    MPI_Barrier(comm);

    // Write results to file
    if (rank == 0)
    {
        // Calculate execution time
        double duration = (end_time - start_time) * 1000; // Convert to milliseconds
        cout << "Sample Sort execution time: " << duration << " ms\n";

        // Write to output file
        ofstream outFile(outputFile, ios::app);
        outFile << "Sorted array: ";
        for (int i = 0; i < array_size; i++)
        {
            outFile << array[i] << " ";
        }
        outFile << endl;
        outFile.close();
    }

    // Clean up
    // (Memory cleanup code)

    return true;
}
```

This function implements the complete parallel sample sort algorithm:

### Phase 1: Initialization and Data Distribution
- Process 0 reads input data from file
- The data size is broadcasted to all processes
- Initial data is distributed to all processes using `MPI_Scatterv`
- Each process sorts its local data using quicksort

### Phase 2: Sample Collection and Splitter Selection
- Each process selects representative samples from its local sorted data
- The samples are gathered at process 0 using `MPI_Gather`
- Process 0 selects splitter values from the collected samples
- The splitters are broadcasted to all processes

### Phase 3: Data Exchange Based on Splitters
- Each process partitions its data according to the splitters
- Partition counts are exchanged using `MPI_Alltoall`
- The actual data is exchanged using `MPI_Alltoallv`
- Now each process has all values in its assigned range

### Phase 4: Local Sorting and Result Collection
- Each process sorts its received data
- The sorted blocks are gathered at process 0 using `MPI_Gatherv`
- Process 0 writes the final sorted array to the output file

## 3. Key MPI Functions In More Detail

### MPI_Gather for Sample Collection
```cpp
MPI_Gather(local_samples, sample_size, MPI_INT,
           samples, sample_size, MPI_INT, 0, comm);
```
This operation:
- Collects local samples from each process
- Each process contributes exactly `sample_size` integers
- Process 0 receives all samples (total of `sample_size * size` integers)
- Critical for determining the global data distribution

### MPI_Alltoall for Count Exchange
```cpp
MPI_Alltoall(partition_counts, 1, MPI_INT,
             recv_counts, 1, MPI_INT, comm);
```
This operation:
- Every process tells every other process how many elements it will send
- Each process sends exactly one integer (count) to each process
- Prepares for the subsequent `MPI_Alltoallv` operation
- Enables each process to allocate the correct amount of memory for receiving

### MPI_Alltoallv for Data Redistribution
```cpp
MPI_Alltoallv(send_buf, partition_counts, send_displs_local, MPI_INT,
              recv_buf, recv_counts, recv_displs, MPI_INT, comm);
```
This is the heart of the sample sort algorithm:
- Redistributes data so each process gets elements in its assigned range
- `partition_counts` and `send_displs_local` specify how much data to send to each process
- `recv_counts` and `recv_displs` specify how much data to receive from each process
- After this operation, each process has all elements in its assigned global range

## 4. Step-by-Step Example

Let's trace through a simple example with 4 processes sorting an array of 16 integers:

### Initial Setup:
- Input array: `[42, 17, 93, 31, 8, 72, 56, 29, 5, 61, 88, 11, 23, 67, 49, 37]`
- 4 processes (ranks 0-3)

### Data Distribution (Phase 1):
After `MPI_Scatterv`:
- Process 0 has: `[42, 17, 93, 31]`
- Process 1 has: `[8, 72, 56, 29]`
- Process 2 has: `[5, 61, 88, 11]`
- Process 3 has: `[23, 67, 49, 37]`

### Local Sorting:
After quicksort:
- Process 0 has: `[17, 31, 42, 93]`
- Process 1 has: `[8, 29, 56, 72]`
- Process 2 has: `[5, 11, 61, 88]`
- Process 3 has: `[23, 37, 49, 67]`

### Sample Selection (Phase 2):
Assuming 2 samples per process:
- Process 0 samples: `[17, 42]` (positions ~1/3 and ~2/3 through local data)
- Process 1 samples: `[8, 56]`
- Process 2 samples: `[5, 61]`
- Process 3 samples: `[23, 49]`

### Splitter Selection:
- All samples gathered at process 0: `[17, 42, 8, 56, 5, 61, 23, 49]`
- Sorted samples: `[5, 8, 17, 23, 42, 49, 56, 61]`
- Selected splitters (3 for 4 processes): `[17, 42, INT_MAX]`
- These splitters divide the data range into 4 parts

### Data Partitioning (Phase 3):
After applying splitters:
- Elements ≤ 17 go to process 0
- Elements between 17 and 42 go to process 1
- Elements between 42 and INT_MAX go to process 2 and 3
- Process 3 handles the largest values

### Data Redistribution:
After `MPI_Alltoallv`:
- Process 0 receives: `[5, 8, 11, 17]` (all values ≤ 17)
- Process 1 receives: `[23, 29, 31, 37]` (values between 17 and 42)
- Process 2 receives: `[42, 49, 56, 61]` (values between 42 and 56)
- Process 3 receives: `[67, 72, 88, 93]` (values > 56)

### Final Local Sorting:
- Each process sorts its received data (if needed)
- The data is already mostly sorted due to the previous quicksort

### Result Collection (Phase 4):
- Process 0 gathers all sorted segments
- Final sorted array: `[5, 8, 11, 17, 23, 29, 31, 37, 42, 49, 56, 61, 67, 72, 88, 93]`

## 5. Performance Considerations

1. **Sample Quality**
   - Sample selection significantly impacts load balancing
   - Using log₂(p) samples per process improves splitter quality
   - Better samples lead to more evenly sized partitions

2. **Communication Costs**
   - The algorithm has three major communication phases:
     1. Initial data distribution (`MPI_Scatterv`)
     2. Sample gathering and splitter broadcasting (`MPI_Gather` and `MPI_Bcast`)
     3. All-to-all data exchange (`MPI_Alltoallv`)
   - The all-to-all exchange is the most expensive communication step

3. **Load Balancing**
   - Sample sort generally achieves good load balancing when data is randomly distributed
   - However, it may struggle with skewed data distributions
   - The number and quality of samples directly impacts load balance

4. **Memory Usage**
   - Each process needs memory for:
     - Local data array
     - Local samples
     - Send and receive buffers for all-to-all exchange
   - The algorithm requires additional temporary memory during the redistribution phase

5. **Scalability**
   - Sample sort scales well with increasing process counts
   - Communication complexity is O(n/p + p log p), which is better than many other parallel sorting algorithms
   - For very large p, the log₂(p) samples per process strategy helps maintain good splitter quality

6. **Optimization Opportunities**
   - More sophisticated sampling strategies for better load balancing
   - Using non-blocking communication to overlap computation and communication
   - Hybrid parallelization with OpenMP for multi-core nodes
   - Specialized handling for skewed data distributions