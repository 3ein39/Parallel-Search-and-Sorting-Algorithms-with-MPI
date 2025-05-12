#include <iostream>
#include <vector>
#include <mpi.h>
#include <cmath>
#include <fstream>
#include <algorithm>
#include <climits>

using namespace std;

// Function to compare and swap elements locally based on direction
void compareAndSwap(vector<int>& arr, int i, int j, bool dir) {
    if (dir == (arr[i] > arr[j])) {
        swap(arr[i], arr[j]);
    }
}

// Function for bitonic merge - performs the compare-split operation
// on local data after receiving from partner
void bitonicMerge(vector<int>& local_data, vector<int>& recv_data, bool dir) {
    int n = local_data.size();
    
    // Perform compare-split operation directly on local_data and recv_data
    for (int i = 0; i < n; i++) {
        // If local_data[i] and recv_data[i] are out of order according to dir, swap them
        if (dir == (local_data[i] > recv_data[i])) {
            swap(local_data[i], recv_data[i]);
        }
    }
}

// Function for parallel bitonic sort using MPI
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

// Wrapper function for bitonic sort
bool runBitonicSort(const char* inputFile, const char* outputFile, int rank, int size, MPI_Comm comm) {
    vector<int> global_array;
    int n = 0;
    
    // Root process reads the input file
    if (rank == 0) {
        ifstream file(inputFile);
        int el;
        while (file >> el) {
            global_array.push_back(el);
        }
        file.close();
        n = global_array.size();
        
        // Check if the number of processes is a power of 2
        if ((size & (size - 1)) != 0) {
            cout << "Error: Number of processes must be a power of 2 for bitonic sort.\n";
            n = -1;  // Signal error
        }
        
        // Print the unsorted array
        if (n > 0) {
            ofstream outFile(outputFile);
            outFile << "Unsorted array: ";
            for (int i = 0; i < min(n, 100); i++) {  // Only print first 100 elements
                outFile << global_array[i] << " ";
            }
            if (n > 100) outFile << "...";
            outFile << endl;
            outFile.close();
        }
    }
    
    // Broadcast array size to all processes
    MPI_Bcast(&n, 1, MPI_INT, 0, comm);
    
    // If error occurred, return false
    if (n <= 0) {
        return false;
    }
    
    // For optimal bitonic sort with p processes:
    // 1. Number of processes must be a power of 2
    // 2. Each process should have equal number of elements
    
    // Calculate elements per process (ceil division to ensure all elements are processed)
    int elements_per_process = (n + size - 1) / size;
    
    // Make elements_per_process a power of 2 if it's not already
    int power_of_two = 1;
    while (power_of_two < elements_per_process) {
        power_of_two *= 2;
    }
    elements_per_process = power_of_two;
    
    // Calculate padded size to make total elements a multiple of elements_per_process * size
    int padded_size = elements_per_process * size;
    
    // Allocate local array with padding
    vector<int> local_data(elements_per_process, INT_MAX);
    
    // Distribute data from root
    if (rank == 0) {
        // Pad the global array if needed
        global_array.resize(padded_size, INT_MAX);
    }
    
    // Scatter data to all processes
    MPI_Scatter(rank == 0 ? global_array.data() : nullptr,
               elements_per_process, MPI_INT,
               local_data.data(), elements_per_process, MPI_INT,
               0, comm);
    
    // Start timing
    MPI_Barrier(comm);
    double start_time = MPI_Wtime();
    
    // Perform parallel bitonic sort
    bitonicSortParallel(local_data, padded_size, rank, size, comm);
    
    // End timing
    double end_time = MPI_Wtime();
    
    // Calculate the max time across all processes
    double local_time = end_time - start_time;
    double max_time;
    MPI_Reduce(&local_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, comm);
    
    // Gather results back to root
    vector<int> result;
    if (rank == 0) {
        result.resize(padded_size);
    }
    
    MPI_Gather(local_data.data(), elements_per_process, MPI_INT,
              result.data(), elements_per_process, MPI_INT,
              0, comm);
    
    // Root process finalizes the sort and writes output
    if (rank == 0) {
        // Remove padding
        result.erase(
            remove_if(result.begin(), result.end(),
                     [](int x) { return x == INT_MAX; }),
            result.end());
        
        // Verify the result is sorted
        bool is_sorted = std::is_sorted(result.begin(), result.end());
        
        // Print execution time
        double duration = max_time * 1000; // Convert to milliseconds
        cout << "Bitonic Sort execution time: " << duration << " ms\n";
        if (!is_sorted) {
            cout << "Warning: Final array is not sorted correctly.\n";
        }
        
        // Write sorted array to output file
        ofstream outFile(outputFile, ios::app);
        outFile << "Sorted array: ";
        for (int i = 0; i < min((int)result.size(), 100); i++) {  // Only print first 100 elements
            outFile << result[i] << " ";
        }
        if (result.size() > 100) outFile << "...";
        outFile << endl;
        outFile.close();
    }
    
    return true;
}