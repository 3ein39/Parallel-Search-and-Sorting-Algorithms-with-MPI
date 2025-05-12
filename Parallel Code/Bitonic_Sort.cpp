#include <iostream>
#include <vector>
#include <mpi/mpi.h>
#include <cmath>
#include <random>
#include <fstream>

using namespace std;

// Function to compare and swap elements locally based on direction
void compareAndSwap(vector<int>& arr, int i, int j, bool dir) {
    if (dir == (arr[i] > arr[j])) {
        swap(arr[i], arr[j]);
    }
}

// Function for bitonically merging locally
void bitonicMergeLocal(vector<int>& arr, int low, int cnt, bool dir) {
    if (cnt > 1) {
        int k = cnt / 2;
        for (int i = low; i < low + k; i++) {
            compareAndSwap(arr, i, i + k, dir);
        }
        bitonicMergeLocal(arr, low, k, dir);
        bitonicMergeLocal(arr, low + k, k, dir);
    }
}

// Function for parallel bitonic sort using MPI - this is the main exported function
void bitonicSortParallel(vector<int>& local_data, int local_n, int total_n, int rank, int size, MPI_Comm comm) {
    // For each stage of the bitonic sort
    for (int step = 2; step <= total_n; step *= 2) {
        // For each substep within the stage
        for (int substep = step / 2; substep > 0; substep /= 2) {
            // Determine partner for the exchange
            int partner = rank ^ substep;
            
            // Skip if the partner is outside the valid range
            if (partner >= size) continue;
            
            // Buffer for receiving data from partner
            vector<int> recv_buffer(local_n);
            
            // Direction of comparison based on the step and rank
            bool dir = ((rank / step) % 2 == 0);
            
            // Exchange data with partner
            MPI_Sendrecv(local_data.data(), local_n, MPI_INT, partner, 0,
                        recv_buffer.data(), local_n, MPI_INT, partner, 0,
                        comm, MPI_STATUS_IGNORE);
            
            // Merge the received data with local data
            vector<int> merged(2 * local_n);
            for (int i = 0; i < local_n; i++) {
                merged[i] = local_data[i];
                merged[i + local_n] = recv_buffer[i];
            }
            
            // Sort the merged array bitonically
            bitonicMergeLocal(merged, 0, 2 * local_n, dir);
            
            // Keep the appropriate half based on whether this rank is smaller or larger than partner
            if (rank < partner) {
                for (int i = 0; i < local_n; i++) {
                    local_data[i] = merged[i];
                }
            } else {
                for (int i = 0; i < local_n; i++) {
                    local_data[i] = merged[i + local_n];
                }
            }
        }
    }
}

// Original main function kept for reference, but commented out
/*
int main(int argc, char** argv) {
    // Initialize MPI
    MPI_Init(&argc, &argv);
    
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    // The total number of elements must be a power of 2
    // and evenly divisible by the number of processes
    const int n = 16; // Must be a power of 2
    
    if (n % size != 0) {
        if (rank == 0) {
            cout << "Error: Array size must be divisible by number of processes" << endl;
        }
        MPI_Finalize();
        return 1;
    }
    
    // Calculate local array size
    int local_n = n / size;
    vector<int> local_array(local_n);
    vector<int> global_array;
    
    // Seed for random number generation
    std::mt19937 gen(time(nullptr) + rank);
    std::uniform_int_distribution<> distrib(1, 100);
    
    // Generate random numbers for the local array
    for (int i = 0; i < local_n; i++) {
        local_array[i] = distrib(gen);
    }
    
    // Gather the initial unsorted array at root
    if (rank == 0) {
        global_array.resize(n);
    }
    
    MPI_Gather(local_array.data(), local_n, MPI_INT,
               global_array.data(), local_n, MPI_INT, 0, MPI_COMM_WORLD);
    
    // Print unsorted array (rank 0 only)
    if (rank == 0) {
        cout << "Unsorted array: ";
        for (int i = 0; i < n; i++) {
            cout << global_array[i] << " ";
        }
        cout << endl;
    }
    
    // Start timing using MPI timing
    MPI_Barrier(MPI_COMM_WORLD);
    double start_time = MPI_Wtime();
    
    // Perform the parallel bitonic sort
    bitonicSortParallel(local_array, local_n, n, rank, size, MPI_COMM_WORLD);
    
    // End timing
    double end_time = MPI_Wtime();
    MPI_Barrier(MPI_COMM_WORLD);
    
    // Gather the sorted array at root
    MPI_Gather(local_array.data(), local_n, MPI_INT,
               global_array.data(), local_n, MPI_INT, 0, MPI_COMM_WORLD);
    
    // Calculate duration and print results (rank 0 only)
    if (rank == 0) {
        double duration = (end_time - start_time) * 1000000; // Convert to microseconds
        
        cout << "Sorted array: ";
        for (int i = 0; i < n; i++) {
            cout << global_array[i] << " ";
        }
        cout << endl;
        
        cout << "Execution time: " << duration << " microseconds" << endl;
        cout << endl;
    }
    
    // Finalize MPI
    MPI_Finalize();
    return 0;
}
*/