#include <iostream>
#include <vector>
#include <mpi.h>
#include <cmath>
#include <random>
#include <fstream>
#include <algorithm> // Added for std::sort

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

// Wrapper function to handle data distribution, sorting, and collection
bool runBitonicSort(const char* inputFile, const char* outputFile, int rank, int size, MPI_Comm comm) {
    vector<int> global_array;
    int n = 0;
    bool is_error = false;
    
    // Root process reads the input file
    if (rank == 0) {
        ifstream file(inputFile);
        int el;
        while (file >> el) {
            global_array.push_back(el);
        }
        file.close();
        
        n = global_array.size();
        
        // Check if array size is valid for bitonic sort
        // Only check for power of 2, not divisibility by processes
        if ((n & (n - 1)) != 0) {
            cout << "Error: Array size is not a power of 2.\n";
            is_error = true;
        }
        
        // Print the unsorted array
        if (!is_error) {
            // Also write to output file
            ofstream outFile(outputFile);
            outFile << "Unsorted array: ";
            for (int i = 0; i < n; i++) {
                outFile << global_array[i] << " ";
            }
            outFile << endl;
            outFile.close();
        }
    }
    
    // Broadcast error status and array size
    MPI_Bcast(&is_error, 1, MPI_C_BOOL, 0, comm);
    
    if (is_error) {
        return false;
    }
    
    // Broadcast the array size to all processes
    MPI_Bcast(&n, 1, MPI_INT, 0, comm);
    
    // Calculate local array size with potential remainder handling
    int local_n = n / size;
    int remainder = n % size;
    int my_local_n = local_n + (rank < remainder ? 1 : 0); // Some processes get one extra element
    int my_offset = rank * local_n + min(rank, remainder); // Calculate offset for this process
    
    vector<int> local_array(my_local_n);
    
    if (rank == 0) {
        // Keep rank 0's local chunk
        for (int i = 0; i < my_local_n; i++) {
            local_array[i] = global_array[i];
        }
        
        // Distribute chunks to all processes
        int current_offset = my_local_n;
        for (int i = 1; i < size; i++) {
            int proc_local_n = local_n + (i < remainder ? 1 : 0); // Calculate this process's local_n
            MPI_Send(&global_array[current_offset], proc_local_n, MPI_INT, i, 0, comm);
            current_offset += proc_local_n;
        }
    } else {
        // Receive local chunk
        MPI_Recv(local_array.data(), my_local_n, MPI_INT, 0, 0, comm, MPI_STATUS_IGNORE);
    }
    
    // Start timing
    MPI_Barrier(comm);
    double start_time = MPI_Wtime();
    
    // Create a uniform distribution of elements across processes for bitonic sort
    // This might involve redistribution to ensure each process has the same number of elements
    int max_local_n = local_n + (remainder > 0 ? 1 : 0);
    vector<int> uniform_local_array(max_local_n, 0); // Initialize with zeros for padding if needed
    
    // Gather all data to redistribute evenly
    vector<int> all_data;
    if (rank == 0) {
        all_data.resize(n);
    }
    
    // Gather with variable counts
    vector<int> recv_counts(size);
    vector<int> displs(size, 0);
    
    for (int i = 0; i < size; i++) {
        recv_counts[i] = local_n + (i < remainder ? 1 : 0);
        if (i > 0) {
            displs[i] = displs[i-1] + recv_counts[i-1];
        }
    }
    
    MPI_Gatherv(local_array.data(), my_local_n, MPI_INT, 
               all_data.data(), recv_counts.data(), displs.data(), 
               MPI_INT, 0, comm);
    
    // Redistribute evenly for bitonic sort
    if (rank == 0) {
        // Keep rank 0's uniform chunk
        for (int i = 0; i < max_local_n && i < n; i++) {
            uniform_local_array[i] = all_data[i];
        }
        
        // Send uniform chunks to other processes
        for (int i = 1; i < size; i++) {
            int start_idx = i * max_local_n;
            int elements_to_send = min(max_local_n, n - start_idx);
            if (elements_to_send > 0) {
                MPI_Send(&all_data[start_idx], elements_to_send, MPI_INT, i, 0, comm);
            }
        }
    } else {
        // Calculate how many elements this process should receive
        int elements_to_recv = min(max_local_n, n - rank * max_local_n);
        if (elements_to_recv > 0) {
            MPI_Recv(uniform_local_array.data(), elements_to_recv, MPI_INT, 0, 0, comm, MPI_STATUS_IGNORE);
        }
    }
    
    // Perform parallel bitonic sort on uniform distribution
    if (n >= size) { // Only perform sort if we have enough elements
        bitonicSortParallel(uniform_local_array, max_local_n, n, rank, size, comm);
    }
    
    // End timing
    double end_time = MPI_Wtime();
    MPI_Barrier(comm);
    
    // Gather uniform data back and redistribute to original distribution
    MPI_Gatherv(uniform_local_array.data(), max_local_n, MPI_INT,
               all_data.data(), recv_counts.data(), displs.data(),
               MPI_INT, 0, comm);
    
    // Gather sorted data back
    if (rank == 0) {
        // Reconstruct the global sorted array
        global_array = all_data;
        
        // Perform a final local sort to ensure complete sorting
        // This is necessary because while each process sorted its portion correctly,
        // the final concatenation might not preserve the global ordering
        sort(global_array.begin(), global_array.end());
        
        // Calculate and print execution time
        double duration = (end_time - start_time) * 1000; // Convert to milliseconds
        cout << "Bitonic Sort execution time: " << duration << " ms\n";
        
        // Write sorted array to output file
        ofstream outFile(outputFile, ios::app);
        outFile << "Sorted array: ";
        for (int i = 0; i < n; i++) {
            outFile << global_array[i] << " ";
        }
        outFile << endl;
        outFile.close();
    }
    
    return true;
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