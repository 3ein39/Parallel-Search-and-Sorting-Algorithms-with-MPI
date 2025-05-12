#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <mpi.h>
#include <sstream>
#include <cmath>

using namespace std;

// Partition function for Quick Search within a process
int partition(vector<int>& arr, int low, int high) {
    if (low >= high) return low;

    int pivot = arr[high];  // Choose the last element as pivot
    int i = low - 1;  // Index of smaller element

    for (int j = low; j < high; j++) {
        // If current element is smaller than or equal to pivot
        if (arr[j] <= pivot) {
            i++;  // Increment index of smaller element
            swap(arr[i], arr[j]);
        }
    }
    swap(arr[i + 1], arr[high]);
    return i + 1;
}

class ParallelQuickSearch {
private:
    vector<int> global_dataset;
    vector<int> local_dataset;
    int rank;
    int num_processes;
    MPI_Comm comm;

    // Recursive quick search within a process
    void quickSearchRecursive(vector<int>& arr, int low, int high) {
        if (low < high) {
            int pivot_index = partition(arr, low, high);
            quickSearchRecursive(arr, low, pivot_index - 1);
            quickSearchRecursive(arr, pivot_index + 1, high);
        }
    }

    // Explicit data partitioning strategy
    void partitionDataAcrossProcesses() {
        // Total number of elements
        int total_elements = global_dataset.size();
        
        // Calculate basic partition size
        int base_partition_size = total_elements / num_processes;
        int remainder = total_elements % num_processes;

        // Calculate start and end indices for this process
        int start_index, end_index, partition_size;

        if (rank < remainder) {
            // First 'remainder' processes get one extra element
            partition_size = base_partition_size + 1;
            start_index = rank * partition_size;
        } else {
            // Remaining processes get base partition size
            partition_size = base_partition_size;
            start_index = remainder * (partition_size + 1) + 
                          (rank - remainder) * partition_size;
        }

        // Determine end index
        end_index = start_index + partition_size;

        // Extract local dataset for this process
        local_dataset.clear();
        local_dataset.insert(
            local_dataset.begin(), 
            global_dataset.begin() + start_index, 
            global_dataset.begin() + end_index
        );

        // Perform local recursive quick search
        if (!local_dataset.empty()) {
            quickSearchRecursive(local_dataset, 0, local_dataset.size() - 1);
        }
    }

    // Read dataset from file (only on root process)
    bool readDatasetFromFile(const string& filename) {
        if (rank != 0) return true;

        ifstream file(filename);
        if (!file.is_open()) {
            return false;
        }

        global_dataset.clear();
        string line;
        while (getline(file, line)) {
            istringstream iss(line);
            int num;
            while (iss >> num) {
                global_dataset.push_back(num);
            }
        }

        file.close();
        return !global_dataset.empty();
    }

    // Search for target in local dataset
    int searchLocalDataset(int target) {
        for (size_t i = 0; i < local_dataset.size(); ++i) {
            if (local_dataset[i] == target) {
                // Calculate global index
                int base_index = 0;
                for (int p = 0; p < rank; ++p) {
                    base_index += (p < (global_dataset.size() % num_processes)) ? 
                        (global_dataset.size() / num_processes + 1) : 
                        (global_dataset.size() / num_processes);
                }
                return base_index + i;
            }
        }
        return -1;
    }

public:
    ParallelQuickSearch(MPI_Comm comm_world) : 
        rank(0), num_processes(1), comm(comm_world) {
        MPI_Comm_rank(comm_world, &rank);
        MPI_Comm_size(comm_world, &num_processes);
    }

    // Find target element
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

        // Broadcast entire dataset
        MPI_Bcast(global_dataset.data(), total_size, MPI_INT, 0, comm);

        // Partition data across processes
        partitionDataAcrossProcesses();

        // Search local dataset
        int local_result = searchLocalDataset(target);

        // Reduce to find global result
        int global_result = -1;
        MPI_Reduce(&local_result, &global_result, 1, MPI_INT, MPI_MAX, 0, comm);

        return (rank == 0) ? global_result : -1;
    }
};

int main(int argc, char** argv) {
    // Initialize MPI
    MPI_Init(&argc, &argv);

    // Check command-line arguments
    if (argc != 3) {
        MPI_Finalize();
        return 1;
    }

    // Parse input
    string input_file = argv[1];
    int target = stoi(argv[2]);

    // Create Parallel Quick Search object
    ParallelQuickSearch pqs(MPI_COMM_WORLD);

    // Perform parallel quick search
    int result = pqs.findElement(input_file, target);

    // Finalize MPI
    MPI_Finalize();

    return result;
}