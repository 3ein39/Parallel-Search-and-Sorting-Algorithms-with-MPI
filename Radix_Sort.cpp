#include <iostream>
#include <vector>
#include <fstream>
#include <mpi.h>
#include <cmath>
#include <algorithm>
#include <climits>

using namespace std;

// Function to find the maximum element in an array
int getMax(const vector<int>& arr) {
    return *max_element(arr.begin(), arr.end());
}

// A utility function to do counting sort based on a specific digit position
void countingSortByDigit(vector<int>& arr, int exp) {
    int n = arr.size();
    vector<int> output(n, 0);
    vector<int> count(10, 0);  // Count array for digits 0-9
    
    // Store count of occurrences of current digit
    for (int i = 0; i < n; i++) {
        count[(arr[i] / exp) % 10]++;
    }
    
    // Change count[i] so that it contains the actual
    // position of this digit in output[]
    for (int i = 1; i < 10; i++) {
        count[i] += count[i - 1];
    }
    
    // Build the output array
    for (int i = n - 1; i >= 0; i--) {
        output[count[(arr[i] / exp) % 10] - 1] = arr[i];
        count[(arr[i] / exp) % 10]--;
    }
    
    // Copy the output array to arr[]
    for (int i = 0; i < n; i++) {
        arr[i] = output[i];
    }
}

// Function to perform radix sort on a local array
void radixSortLocal(vector<int>& arr) {
    // Handle empty arrays gracefully
    if (arr.empty()) return;
    
    // Find the maximum number to know the number of digits
    int max_val = getMax(arr);
    
    // Do counting sort for every digit
    for (int exp = 1; max_val / exp > 0; exp *= 10) {
        countingSortByDigit(arr, exp);
    }
}

// Simplified parallel radix sort without the overly complex redistribution
void parallelRadixSort(vector<int>& local_data, int rank, int size, MPI_Comm comm) {
    if (local_data.empty()) {
        return;
    }
    
    // Sort local data first
    radixSortLocal(local_data);
    
    // Now gather all sorted data to rank 0
    int local_size = local_data.size();
    vector<int> sizes(size);
    
    // Exchange sizes
    MPI_Allgather(&local_size, 1, MPI_INT, sizes.data(), 1, MPI_INT, comm);
    
    // Calculate displacements
    vector<int> displs(size, 0);
    for (int i = 1; i < size; i++) {
        displs[i] = displs[i-1] + sizes[i-1];
    }
    
    // Calculate total size
    int total_size = 0;
    for (int i = 0; i < size; i++) {
        total_size += sizes[i];
    }
    
    // Gather all data to all processes
    vector<int> all_data(total_size);
    MPI_Allgatherv(local_data.data(), local_size, MPI_INT,
                  all_data.data(), sizes.data(), displs.data(), MPI_INT, comm);
    
    // Sort the gathered data (this is a sequential operation but done by all processes)
    sort(all_data.begin(), all_data.end());
    
    // Redistribute the sorted data back to processes
    int elements_per_proc = total_size / size;
    int remainder = total_size % size;
    
    // Calculate how many elements this process should have
    int my_elements = elements_per_proc + (rank < remainder ? 1 : 0);
    int my_start = rank * elements_per_proc + min(rank, remainder);
    
    // Copy my portion of the sorted data
    local_data.resize(my_elements);
    for (int i = 0; i < my_elements; i++) {
        local_data[i] = all_data[my_start + i];
    }
}

// Wrapper function to handle data distribution, sorting, and collection
bool runRadixSort(const char* inputFile, const char* outputFile, int rank, int size, MPI_Comm comm) {
    vector<int> global_array;
    int array_size = 0;
    bool is_error = false;
    
    // Root process reads the input file
    if (rank == 0) {
        ifstream file(inputFile);
        if (!file.is_open()) {
            cout << "Error: Unable to open input file " << inputFile << endl;
            is_error = true;
        } else {
            int num;
            while (file >> num) {
                global_array.push_back(num);
            }
            file.close();
            
            array_size = global_array.size();
            
            if (array_size <= 0) {
                cout << "Error: Input array is empty" << endl;
                is_error = true;
            } else {
                // Print the unsorted array
                cout << "Unsorted array: ";
                for (int i = 0; i < array_size; i++) {
                    cout << global_array[i] << " ";
                }
                cout << endl;
                
                // Also write to output file
                ofstream outFile(outputFile);
                outFile << "Unsorted array: ";
                for (int i = 0; i < array_size; i++) {
                    outFile << global_array[i] << " ";
                }
                outFile << endl;
                outFile.close();
            }
        }
    }
    
    // Broadcast error status
    MPI_Bcast(&is_error, 1, MPI_C_BOOL, 0, comm);
    
    if (is_error) {
        return false;
    }
    
    // Broadcast array size
    MPI_Bcast(&array_size, 1, MPI_INT, 0, comm);
    
    // Calculate local array size and offset
    int local_size = array_size / size;
    int remainder = array_size % size;
    int my_local_size = local_size + (rank < remainder ? 1 : 0);
    int my_offset = rank * local_size + min(rank, remainder);
    
    // Allocate local array
    vector<int> local_array(my_local_size);
    
    // Root process distributes data
    if (rank == 0) {
        // Copy data for rank 0
        for (int i = 0; i < my_local_size; i++) {
            local_array[i] = global_array[i];
        }
        
        // Send data to other processes
        int current_offset = my_local_size;
        for (int i = 1; i < size; i++) {
            int proc_local_size = local_size + (i < remainder ? 1 : 0);
            MPI_Send(&global_array[current_offset], proc_local_size, MPI_INT, i, 0, comm);
            current_offset += proc_local_size;
        }
    } else {
        // Receive local chunk from root process
        MPI_Recv(local_array.data(), my_local_size, MPI_INT, 0, 0, comm, MPI_STATUS_IGNORE);
    }
    
    // Start timing
    MPI_Barrier(comm);
    double start_time = MPI_Wtime();
    
    // Perform parallel radix sort
    parallelRadixSort(local_array, rank, size, comm);
    
    // End timing
    double end_time = MPI_Wtime();
    MPI_Barrier(comm);
    
    // Calculate timing statistics
    double local_time = end_time - start_time;
    double max_time = 0.0;
    MPI_Reduce(&local_time, &max_time, 1, MPI_DOUBLE, MPI_MAX, 0, comm);
    
    // Gather all data back to root
    vector<int> recv_counts(size);
    vector<int> displs(size);
    
    for (int i = 0; i < size; i++) {
        recv_counts[i] = local_size + (i < remainder ? 1 : 0);
        displs[i] = i * local_size + min(i, remainder);
    }
    
    // Resize global array on root process
    if (rank == 0) {
        global_array.resize(array_size);
    }
    
    // Gather sorted data
    MPI_Gatherv(local_array.data(), my_local_size, MPI_INT,
               global_array.data(), recv_counts.data(), displs.data(),
               MPI_INT, 0, comm);
    
    // Root writes results to file
    if (rank == 0) {
        // Write sorted array to the output file
        ofstream outFile(outputFile, ios::app);
        outFile << "Sorted array: ";
        for (int i = 0; i < array_size; i++) {
            outFile << global_array[i] << " ";
        }
        outFile << endl;
        
        // Print sorted array and timing information
        cout << "Sorted array: ";
        for (int i = 0; i < array_size; i++) {
            cout << global_array[i] << " ";
        }
        cout << endl;
        
        cout << "Radix Sort execution time: " << max_time * 1000 << " ms" << endl;
    }
    
    return true;
}