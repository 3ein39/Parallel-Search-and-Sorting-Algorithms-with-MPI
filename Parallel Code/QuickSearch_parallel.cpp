#include <mpi.h>
#include <iostream>
#include <vector>
#include <random>
#include <algorithm>

using namespace std;

// Function to partition the array around a pivot and return the pivot's final index
int partition(vector<int>& arr, int left, int right, int pivotIndex) {
    int pivotValue = arr[pivotIndex];
    swap(arr[pivotIndex], arr[right]); // Move pivot to end
    int storeIndex = left;

    for (int i = left; i < right; i++) {
        if (arr[i] <= pivotValue) {
            swap(arr[i], arr[storeIndex]);
            storeIndex++;
        }
    }

    swap(arr[storeIndex], arr[right]); // Move pivot to its final place
    return storeIndex;
}

// Recursive quick search function that returns global index (or -1 if not found)
int quickSearch(vector<int>& arr, int left, int right, int target, int global_offset, mt19937& gen) {
    if (left > right) {
        return -1; // Base case: subarray is empty
    }

    // Choose a random pivot
    uniform_int_distribution<> dist(left, right);
    int pivotIndex = dist(gen);
    int pivotValue = arr[pivotIndex];

    // Partition the array around the pivot
    pivotIndex = partition(arr, left, right, pivotIndex);

    // Check if pivot is the target
    if (arr[pivotIndex] == target) {
        return global_offset + pivotIndex; // Return global index
    }

    // Recursively search the appropriate partition(s)
    if (target < pivotValue) {
        // Search left partition
        return quickSearch(arr, left, pivotIndex - 1, target, global_offset, gen);
    } else {
        // Search right partition
        return quickSearch(arr, pivotIndex + 1, right, target, global_offset, gen);
    }
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int n = 100; // Array size
    int target;
    vector<int> data;
    random_device rd;
    mt19937 gen(rd()); // Random number generator for pivot selection

    // Generate data in root process
    if (rank == 0) {
        uniform_int_distribution<> dist(0, 999);
        data.resize(n);
        for (int i = 0; i < n; i++) {
            data[i] = dist(gen);
        }
        target = data[dist(gen) % n]; // Random target from array
        cout << "Target: " << target << endl;
    }

    // Broadcast target
    MPI_Bcast(&target, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Compute chunk sizes and displacements for MPI_Scatterv
    vector<int> sendcounts(size), displs(size);
    int chunk = n / size;
    int remainder = n % size;
    int global_offset = 0;

    for (int i = 0; i < size; i++) {
        sendcounts[i] = (i < remainder) ? chunk + 1 : chunk;
        displs[i] = global_offset;
        global_offset += sendcounts[i];
    }

    // Allocate local data
    int local_size = sendcounts[rank];
    vector<int> local_data(local_size);

    // Distribute data using MPI_Scatterv
    MPI_Scatterv(data.data(), sendcounts.data(), displs.data(), MPI_INT,
                 local_data.data(), local_size, MPI_INT, 0, MPI_COMM_WORLD);

    // Perform recursive quick search on local data
    int local_result = quickSearch(local_data, 0, local_size - 1, target, displs[rank], gen);

    // Gather results from all processes
    vector<int> all_results(size);
    MPI_Gather(&local_result, 1, MPI_INT, all_results.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Process results in root
    if (rank == 0) {
        int final_result = -1;
        for (int i = 0; i < size; i++) {
            if (all_results[i] != -1) {
                final_result = all_results[i];
                break; // Take first valid result
            }
        }
        if (final_result != -1) {
            cout << "Found at index " << final_result << endl;
        } else {
            cout << "Not found" << endl;
        }
    }

    MPI_Finalize();
    return 0;
}