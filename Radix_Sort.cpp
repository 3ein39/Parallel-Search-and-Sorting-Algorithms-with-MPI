#include <mpi.h>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>

const int BASE = 10; // Using base-10 for radix sort

// Get the largest value in the array
int get_largest(const std::vector<int>& numbers) {
    if (numbers.empty()) return 0;
    return *std::max_element(numbers.begin(), numbers.end());
}

// Calculate number of digits in a number
int digit_count(int value) {
    if (value == 0) return 1;
    int digits = 0;
    while (value > 0) {
        value /= 10;
        digits++;
    }
    return digits;
}

// Helper function to distribute numbers based on digit
void distribute_by_digit(const std::vector<int>& input, int divisor, int size,
                         std::vector<std::vector<int>>& buckets, std::vector<int>& counts) {
    std::fill(counts.begin(), counts.end(), 0);
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

// Wrapper function to be called from source.cpp
bool runRadixSort(const char* inputFile, const char* outputFile, int rank, int size, MPI_Comm comm) {
    std::vector<int> input_array;
    int array_size = 0;

    // Process 0 reads input from file
    if (rank == 0) {
        std::ifstream input_file(inputFile);
        if (!input_file.is_open()) {
            std::cerr << "Error: Unable to open " << inputFile << std::endl;
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
    std::vector<int> partition(partition_size);

    // Prepare for scattering data
    std::vector<int> counts_to_send(size), offsets(size);
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

    std::vector<int> counts_to_send_proc(size);
    std::vector<int> counts_to_recv(size);

    // Process each digit
    for (int digit_pos = 0; digit_pos < max_digits; ++digit_pos) {
        int divisor = 1;
        for (int i = 0; i < digit_pos; ++i) divisor *= 10;

        // Distribute numbers to buckets
        std::vector<std::vector<int>> buckets(size);
        distribute_by_digit(partition, divisor, size, buckets, counts_to_send_proc);

        // Share send counts
        MPI_Alltoall(counts_to_send_proc.data(), 1, MPI_INT, counts_to_recv.data(), 1, MPI_INT, comm);

        // Calculate displacements
        std::vector<int> send_offsets(size), recv_offsets(size);
        send_offsets[0] = recv_offsets[0] = 0;
        for (int i = 1; i < size; ++i) {
            send_offsets[i] = send_offsets[i - 1] + counts_to_send_proc[i - 1];
            recv_offsets[i] = recv_offsets[i - 1] + counts_to_recv[i - 1];
        }

        // Prepare send buffer
        int total_to_send = send_offsets[size - 1] + counts_to_send_proc[size - 1];
        std::vector<int> send_data(total_to_send);
        int pos = 0;
        for (int i = 0; i < size; ++i) {
            std::copy(buckets[i].begin(), buckets[i].end(), send_data.begin() + pos);
            pos += buckets[i].size();
        }

        // Update partition size
        partition_size = recv_offsets[size - 1] + counts_to_recv[size - 1];
        std::vector<int> recv_data(partition_size);

        // Exchange data between processes
        MPI_Alltoallv(send_data.data(), counts_to_send_proc.data(), send_offsets.data(), MPI_INT,
                      recv_data.data(), counts_to_recv.data(), recv_offsets.data(), MPI_INT,
                      comm);

        partition = std::move(recv_data);

        // Perform local counting sort if needed
        if (size < BASE) {
            std::vector<int> digit_counts(BASE, 0);
            std::vector<int> sorted(partition_size);
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
            partition = std::move(sorted);
        }
    }

    // Gather partition sizes
    std::vector<int> final_counts(size);
    MPI_Allgather(&partition_size, 1, MPI_INT, final_counts.data(), 1, MPI_INT, comm);

    std::vector<int> final_offsets(size);
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
        std::ofstream output_file(outputFile);
        if (!output_file.is_open()) {
            std::cerr << "Error: Unable to open " << outputFile << std::endl;
            return false;
        } else {
            for (auto num : input_array) {
                output_file << num << " ";
            }
            output_file << std::endl;
            output_file.close();
        }
    }

    return true;
}

// Keeping the main function for standalone testing
#ifdef RADIX_SORT_MAIN
int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);

    int proc_id, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &proc_id);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    bool success = runRadixSort("in.txt", "out.txt", proc_id, num_procs, MPI_COMM_WORLD);

    MPI_Finalize();
    return success ? 0 : 1;
}
#endif