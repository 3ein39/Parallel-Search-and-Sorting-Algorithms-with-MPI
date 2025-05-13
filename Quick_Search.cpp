#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <mpi.h>
#include <sstream>

using namespace std;

class ParallelQuickSearch {
private:
    vector<int> global_dataset;
    vector<int> local_dataset;
    int rank;
    int num_processes;
    MPI_Comm comm;

    int partition(vector<int>& arr, int low, int high) {
        if (low >= high) return low;

        int pivot = arr[high];
        int i = low - 1;

        for (int j = low; j < high; j++) {
            if (arr[j] <= pivot) {
                i++;
                swap(arr[i], arr[j]);
            }
        }
        swap(arr[i + 1], arr[high]);
        return i + 1;
    }

    void quickSort(vector<int>& arr, int low, int high) {
        if (low < high) {
            int pivot_index = partition(arr, low, high);
            quickSort(arr, low, pivot_index - 1);
            quickSort(arr, pivot_index + 1, high);
        }
    }

    int binarySearch(const vector<int>& arr, int target) {
        int left = 0, right = arr.size() - 1;
        while (left <= right) {
            int mid = left + (right - left) / 2;
            if (arr[mid] == target) {
                return mid;
            }
            else if (arr[mid] < target) {
                left = mid + 1;
            }
            else {
                right = mid - 1;
            }
        }
        return -1;
    }

    void partitionDataAcrossProcesses() {
        int total_elements = global_dataset.size();
        int base_partition_size = total_elements / num_processes;
        int remainder = total_elements % num_processes;
        vector<int> send_counts(num_processes);
        vector<int> displacements(num_processes);
        int start_index = 0;

        for (int p = 0; p < num_processes; ++p) {
            send_counts[p] = (p < remainder) ? (base_partition_size + 1) : base_partition_size;
            displacements[p] = start_index;
            start_index += send_counts[p];
        }

        local_dataset.resize(send_counts[rank]);
        MPI_Scatterv(global_dataset.data(), send_counts.data(), displacements.data(), MPI_INT,
            local_dataset.data(), send_counts[rank], MPI_INT, 0, comm);

        if (!local_dataset.empty()) {
            quickSort(local_dataset, 0, local_dataset.size() - 1);
        }
    }


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

    int searchLocalDataset(int target) {
        if (local_dataset.empty()) return -1;

        int local_index = binarySearch(local_dataset, target);
        if (local_index == -1) return -1;

        int base_index = 0;
        for (int p = 0; p < rank; ++p) {
            base_index += (p < (global_dataset.size() % num_processes)) ?
                (global_dataset.size() / num_processes + 1) :
                (global_dataset.size() / num_processes);
        }
        return base_index + local_index;
    }

public:
    ParallelQuickSearch(MPI_Comm comm_world) :
        rank(0), num_processes(1), comm(comm_world) {
        MPI_Comm_rank(comm_world, &rank);
        MPI_Comm_size(comm_world, &num_processes);
    }

    int findElement(const string& filename, int target) {

        if (rank == 0) {
            if (!readDatasetFromFile(filename)) {
                return -2;  // read error
            }
        }

        int total_size = global_dataset.size();
        MPI_Bcast(&total_size, 1, MPI_INT, 0, comm);
        if (rank != 0) {
            global_dataset.resize(total_size);
        }


        partitionDataAcrossProcesses();
        int local_result = searchLocalDataset(target);
        int global_result = -1;
        MPI_Reduce(&local_result, &global_result, 1, MPI_INT, MPI_MAX, 0, comm);

        return (rank == 0) ? global_result : -1;
    }
};

bool runQuickSearch(const char* inputFile, const char* outputFile, int target, int rank, int size, MPI_Comm comm) {

    MPI_Barrier(comm);
    double start_time = MPI_Wtime();


    ParallelQuickSearch pqs(comm);


    int result = pqs.findElement(inputFile, target);

    double end_time = MPI_Wtime();
    MPI_Barrier(comm);


    if (rank == 0) {

        double duration = (end_time - start_time) * 1000;
        ofstream outFile(outputFile);
        outFile << "Quick Search Results:\n";

        if (result == -2) {
            outFile << "Error: Could not open input file\n";
            cout << "Error: Could not open input file\n";
            return false;
        }
        else if (result == -1) {
            outFile << "Target " << target << " not found in dataset\n";
            cout << "Target " << target << " not found in dataset\n";
        }
        else {
            outFile << "Target " << target << " found at position " << result << "\n";
            cout << "Target " << target << " found at position " << result << "\n";
        }

        outFile << "Execution time: " << duration << " ms\n";
        cout << "Quick Search execution time: " << duration << " ms\n";

        outFile.close();
    }

    return (result >= -1);
}