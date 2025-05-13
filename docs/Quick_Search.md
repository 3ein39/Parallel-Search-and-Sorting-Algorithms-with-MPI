## 1. Core Functions Explanation

### `ParallelQuickSearch` Class Overview
This class implements a parallel search algorithm that distributes a dataset across multiple processes and uses binary search to efficiently locate target elements.

### `partition` Function
```cpp
int partition(vector<int>& arr, int low, int high) {
    if (low >= high) return low;

    int pivot = arr[high];  // Choose the last element as pivot
    int i = low - 1;  // Index of smaller element

    for (int j = low; j < high; j++) {
        if (arr[j] <= pivot) {
            i++;
            swap(arr[i], arr[j]);
        }
    }
    swap(arr[i + 1], arr[high]);
    return i + 1;
}
```
This function implements the partition step of the quicksort algorithm:
- Takes the last element as the pivot
- Places the pivot in its correct position in the sorted array
- Places all smaller elements to the left of the pivot and all greater elements to the right
- Returns the pivot's index after partitioning

### `quickSort` Function
```cpp
void quickSort(vector<int>& arr, int low, int high) {
    if (low < high) {
        int pivot_index = partition(arr, low, high);
        quickSort(arr, low, pivot_index - 1);
        quickSort(arr, pivot_index + 1, high);
    }
}
```
This is a recursive implementation of the quicksort algorithm:
- Partitions the array around a pivot element
- Recursively sorts the left and right subarrays
- Used to sort the local dataset on each process

### `binarySearch` Function
```cpp
int binarySearch(const vector<int>& arr, int target) {
    int left = 0, right = arr.size() - 1;
    while (left <= right) {
        int mid = left + (right - left) / 2;
        if (arr[mid] == target) {
            return mid;  // Found the target
        } else if (arr[mid] < target) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    return -1;  // Target not found
}
```
This function implements the binary search algorithm:
- Takes a sorted array and searches for the target element
- Returns the index of the target if found, or -1 if not found
- Works in O(log n) time complexity, making it efficient for large datasets

### `partitionDataAcrossProcesses` Function
```cpp
void partitionDataAcrossProcesses() {
    int total_elements = global_dataset.size();
    int base_partition_size = total_elements / num_processes;
    int remainder = total_elements % num_processes;

    // Calculate send counts and displacements for MPI_Scatterv
    vector<int> send_counts(num_processes);
    vector<int> displacements(num_processes);
    int start_index = 0;

    for (int p = 0; p < num_processes; ++p) {
        send_counts[p] = (p < remainder) ? (base_partition_size + 1) : base_partition_size;
        displacements[p] = start_index;
        start_index += send_counts[p];
    }

    // Resize local dataset
    local_dataset.resize(send_counts[rank]);

    // Scatter the dataset to processes
    MPI_Scatterv(global_dataset.data(), send_counts.data(), displacements.data(), MPI_INT,
                 local_dataset.data(), send_counts[rank], MPI_INT, 0, comm);

    // Sort the local dataset
    if (!local_dataset.empty()) {
        quickSort(local_dataset, 0, local_dataset.size() - 1);
    }
}
```
This function:
- Calculates how to divide the data evenly across all processes
- Handles the case where the data size is not evenly divisible by the number of processes
- Uses MPI_Scatterv to distribute the data efficiently
- Sorts each process's local data chunk using quicksort

### `searchLocalDataset` Function
```cpp
int searchLocalDataset(int target) {
    if (local_dataset.empty()) return -1;

    // Use binary search to find the target
    int local_index = binarySearch(local_dataset, target);
    if (local_index == -1) return -1;

    // Calculate global index
    int base_index = 0;
    for (int p = 0; p < rank; ++p) {
        base_index += (p < (global_dataset.size() % num_processes)) ?
            (global_dataset.size() / num_processes + 1) :
            (global_dataset.size() / num_processes);
    }
    return base_index + local_index;
}
```
This function:
- Searches for the target in the local dataset using binary search
- If found, converts the local index to a global index
- Returns -1 if the target is not found in the local dataset
- The global index calculation accounts for the data distribution pattern

### `findElement` Function
```cpp
int findElement(const string& filename, int target) {
    // Read dataset from file (only on root process)
    if (rank == 0) {
        if (!readDatasetFromFile(filename)) {
            return -2;  // Indicate file read error
        }
    }

    // Broadcast dataset size
    int total_size = global_dataset.size();
    MPI_Bcast(&total_size, 1, MPI_INT, 0, comm);

    // Resize global dataset for non-root processes
    if (rank != 0) {
        global_dataset.resize(total_size);
    }

    // Partition data across processes
    partitionDataAcrossProcesses();

    // Search local dataset
    int local_result = searchLocalDataset(target);

    // Reduce to find global result
    int global_result = -1;
    MPI_Reduce(&local_result, &global_result, 1, MPI_INT, MPI_MAX, 0, comm);

    return (rank == 0) ? global_result : -1;
}
```
This is the main search function that orchestrates the entire process:
- Process 0 reads the dataset from the input file
- The dataset size is broadcasted to all processes
- The dataset is distributed across all processes
- Each process searches its local dataset for the target
- Results are combined using MPI_Reduce with MPI_MAX operation (since successful searches return positive indices)
- The function returns the global index of the target if found, or -1 if not found

## 2. The Wrapper Function `runQuickSearch`

```cpp
bool runQuickSearch(const char* inputFile, const char* outputFile, int target, int rank, int size, MPI_Comm comm) {
    // Start timing
    MPI_Barrier(comm);
    double start_time = MPI_Wtime();
    
    // Create Parallel Quick Search object
    ParallelQuickSearch pqs(comm);
    
    // Perform the search
    int result = pqs.findElement(inputFile, target);
    
    // End timing
    double end_time = MPI_Wtime();
    MPI_Barrier(comm);
    
    // Only the root process will write results
    if (rank == 0) {
        // Calculate duration
        double duration = (end_time - start_time) * 1000; // milliseconds
        
        // Write results to output file
        ofstream outFile(outputFile);
        outFile << "Quick Search Results:\n";
        
        if (result == -2) {
            outFile << "Error: Could not open input file\n";
            cout << "Error: Could not open input file\n";
            return false;
        } else if (result == -1) {
            outFile << "Target " << target << " not found in dataset\n";
            cout << "Target " << target << " not found in dataset\n";
        } else {
            outFile << "Target " << target << " found at position " << result << "\n";
            cout << "Target " << target << " found at position " << result << "\n";
        }
        
        outFile << "Execution time: " << duration << " ms\n";
        cout << "Quick Search execution time: " << duration << " ms\n";
        
        outFile.close();
    }
    
    return (result >= -1);  // Return true if search was performed, even if target not found
}
```

This wrapper function:
1. **Timing & Synchronization**: 
   - Uses MPI_Barrier to synchronize all processes
   - Records the start and end times for performance measurement

2. **Search Process**:
   - Creates a ParallelQuickSearch object
   - Calls the findElement method with the specified target

3. **Result Reporting**:
   - Process 0 writes the results to the specified output file
   - Handles different result codes (-2 for file error, -1 for not found)
   - Reports the execution time

## 3. Key MPI Functions In More Detail

### MPI_Scatterv
```cpp
MPI_Scatterv(global_dataset.data(), send_counts.data(), displacements.data(), MPI_INT,
             local_dataset.data(), send_counts[rank], MPI_INT, 0, comm);
```
This function:
- Distributes different portions of the global dataset to each process
- `send_counts` specifies how many elements each process should receive
- `displacements` indicates where in the global array each process's data starts
- Handles uneven distributions gracefully

### MPI_Reduce
```cpp
MPI_Reduce(&local_result, &global_result, 1, MPI_INT, MPI_MAX, 0, comm);
```
This function:
- Combines the search results from all processes using the MAX operation
- Only successful searches return non-negative indices
- If any process finds the target, its index will be selected due to MAX operation
- The final result is stored in `global_result` on process 0

### MPI_Bcast
```cpp
MPI_Bcast(&total_size, 1, MPI_INT, 0, comm);
```
This function:
- Broadcasts the dataset size from process 0 to all other processes
- Ensures all processes know how big the global dataset is
- Used for proper data distribution calculation

## 4. Step-by-Step Example

Let's trace through the algorithm with a simple example:

### Initial Setup:
- Input array: `[23, 14, 5, 67, 32, 91, 8, 44, 18, 50]`
- 4 processes (ranks 0-3)
- Target to search for: 32

### Data Distribution Phase:
After `MPI_Scatterv`:
- Process 0 has: `[23, 14, 5]` (3 elements)
- Process 1 has: `[67, 32, 91]` (3 elements)
- Process 2 has: `[8, 44]` (2 elements)
- Process 3 has: `[18, 50]` (2 elements)

### Local Sorting:
After quicksort within each process:
- Process 0 has: `[5, 14, 23]`
- Process 1 has: `[32, 67, 91]`
- Process 2 has: `[8, 44]`
- Process 3 has: `[18, 50]`

### Local Search:
- Process 0: Searches for 32 in `[5, 14, 23]` → Not found, returns -1
- Process 1: Searches for 32 in `[32, 67, 91]` → Found at local index 0
  - Calculates global index: 3 + 0 = 3
  - Returns 3
- Process 2: Searches for 32 in `[8, 44]` → Not found, returns -1
- Process 3: Searches for 32 in `[18, 50]` → Not found, returns -1

### Result Reduction:
- MPI_Reduce with MPI_MAX combines: [-1, 3, -1, -1] → 3
- Process 0 receives the global index 3

### Final Output:
- "Target 32 found at position 3"
- Reports the execution time

## 5. Performance Considerations

1. **Data Distribution**
   - The algorithm distributes data evenly, accounting for remainder elements
   - This ensures balanced workload across all processes

2. **Search Efficiency**
   - Uses binary search (O(log n)) within each process
   - Each process only searches its local portion of the data
   - The overall complexity is O(log(n/p)) where p is the number of processes

3. **Communication Costs**
   - Minimal communication overhead:
     - One broadcast operation for the dataset size
     - One scatter operation for data distribution
     - One reduce operation for result collection

4. **Scalability**
   - Adding more processes reduces the local dataset size
   - For very large datasets, this leads to significant speedup
   - For small datasets, communication overhead might outweigh computational benefits

5. **Memory Usage**
   - Each process only needs to store its local portion of the dataset
   - This allows for searching in datasets that wouldn't fit in a single machine's memory

6. **Optimization Opportunities**
   - The algorithm could be enhanced with an initial sampling step to improve data distribution
   - For extremely large datasets, a hierarchical approach might reduce communication overhead

## 6. Complexity Analysis

### Time Complexity
- **Sequential Search**: O(n log n) for sorting + O(log n) for binary search
  - Where n is the dataset size
- **Parallel Quick Search**: O((n/p) log(n/p)) + O(log(n/p))
  - Local data sorting: O((n/p) log(n/p)) per process
  - Binary search: O(log(n/p)) per process
  - Communication overhead: O(log p)

### Space Complexity
- **Sequential**: O(n) for storing the dataset
- **Parallel**: O(n/p) per process for local dataset storage
  - Each process only stores its portion of the dataset
  - Additional O(1) space for search result communication

### Communication Costs
- **Broadcast Operation**: O(log p) time to broadcast dataset size
- **Scatter Operation**: O(n/p) data + O(log p) time
  - One-time distribution of dataset across processes
- **Reduce Operation**: O(log p) time for combining search results
  - Minimal data (single integer) per process
- **Total Communication Volume**: O(n) + O(p log p)
