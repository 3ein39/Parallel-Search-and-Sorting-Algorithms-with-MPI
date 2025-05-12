#include <iostream>
#include <vector>
#include <random>
#include <mpi.h>

// Parallel linear search on local data
bool local_search(const std::vector<int>& local_data, int target) {
    for (int x : local_data) {
        if (x == target) {
            return true;
        }
    }
    return false;
}

int main(int argc, char** argv) {
    // Initialize MPI
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int n = 10000; // Default array size
    int target = 0;
    std::vector<int> data;

    // Master process: Handle input and generate data
    if (rank == 0) {
        if (argc > 1) {
            n = std::atoi(argv[1]);
        }
        if (n <= 0) {
            std::cerr << "Invalid array size\n";
            MPI_Finalize();
            return 1;
        }

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(1, 1000);

        data.resize(n);
        for (int i = 0; i < n; i++) {
            data[i] = dis(gen); // Random numbers from 1 to 1000
        }
        target = data[dis(gen) % n]; // Random target from array
    }

    // Broadcast array size and target
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&target, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Compute local data sizes and displacements
    std::vector<int> sendcounts(size), displs(size);
    int base_size = n / size;
    int remainder = n % size;
    int offset = 0;
    for (int i = 0; i < size; ++i) {
        sendcounts[i] = base_size + (i < remainder ? 1 : 0);
        displs[i] = offset;
        offset += sendcounts[i];
    }

    // Allocate local data
    int local_n = sendcounts[rank];
    std::vector<int> local_data(local_n);

    // Distribute data to all processes
    MPI_Scatterv(data.data(), sendcounts.data(), displs.data(), MPI_INT,
                 local_data.data(), local_n, MPI_INT, 0, MPI_COMM_WORLD);

    // Parallel search
    bool local_result = local_search(local_data, target);

    // Collect results
    bool global_result = false;
    MPI_Reduce(&local_result, &global_result, 1, MPI_CXX_BOOL, MPI_LOR, 0, MPI_COMM_WORLD);

    // Root process prints results
    if (rank == 0) {
        std::cout << "Parallel Quick Search (Target: " << target << "): "
                  << (global_result ? "Found" : "Not Found") << "\n";
    }

    // Finalize MPI
    MPI_Finalize();
    return 0;
}