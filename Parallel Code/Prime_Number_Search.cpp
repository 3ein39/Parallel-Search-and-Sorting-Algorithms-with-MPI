#include <iostream>
#include <vector>
#include <ctime>
#include <chrono>
#include <mpi.h>

using namespace std;
using namespace std::chrono;

bool isPrime(int n)
{
    if (n <= 1)
        return false;
    for (int i = 2; i * i <= n; i++)
    {
        if (n % i == 0)
            return false;
    }
    return true;
}

vector<int> findPrimesInRange(int start, int end)
{
    vector<int> primes;
    for (int i = start; i <= end; i++)
    {
        if (isPrime(i))
        {
            primes.push_back(i);
        }
    }
    return primes;
}

int main(int argc, char **argv)
{
    int rank, size;
    int start = 2;    // Default start value
    int end = 100000; // Default end value
    int local_start, local_end;
    int total_primes = 0;
    double start_time, end_time;

    // Initialize MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Process command line arguments if provided
    if (argc >= 3 && rank == 0)
    {
        start = atoi(argv[1]);
        end = atoi(argv[2]);
    }

    // Broadcast the range to all processes
    MPI_Bcast(&start, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&end, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Calculate the range for each process
    int range_size = end - start + 1;
    int chunk_size = range_size / size;
    int remainder = range_size % size;

    // Distribute the work evenly
    if (rank < remainder)
    {
        local_start = start + rank * (chunk_size + 1);
        local_end = local_start + chunk_size;
    }
    else
    {
        local_start = start + rank * chunk_size + remainder;
        local_end = local_start + chunk_size - 1;
    }

    // Start timing
    start_time = MPI_Wtime();

    // Each process finds primes in its local range
    vector<int> local_primes = findPrimesInRange(local_start, local_end);
    int local_count = local_primes.size();

    // Gather the counts of primes found by each process
    vector<int> all_counts(size);
    MPI_Gather(&local_count, 1, MPI_INT, all_counts.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Calculate displacements for gathering primes
    vector<int> displacements(size, 0);
    if (rank == 0)
    {
        for (int i = 1; i < size; i++)
        {
            displacements[i] = displacements[i - 1] + all_counts[i - 1];
        }
        total_primes = displacements[size - 1] + all_counts[size - 1];
    }

    // Broadcast total count to all processes
    MPI_Bcast(&total_primes, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Gather all primes to root process
    vector<int> all_primes;
    if (rank == 0)
    {
        all_primes.resize(total_primes);
    }

    // Use Gatherv to collect primes of variable length from each process
    MPI_Gatherv(local_primes.data(), local_count, MPI_INT,
                all_primes.data(), all_counts.data(), displacements.data(),
                MPI_INT, 0, MPI_COMM_WORLD);

    // Stop timing
    end_time = MPI_Wtime();

    // Print results at root process
    if (rank == 0)
    {
        cout << "Found " << total_primes << " prime numbers between " << start << " and " << end << endl;
        cout << "Execution time: " << (end_time - start_time) * 1000 << " ms" << endl;

        // Print a sample of first few primes if the array is not too large
        if (total_primes <= 100)
        {
            cout << "Prime numbers: ";
            for (int prime : all_primes)
            {
                cout << prime << " ";
            }
            cout << endl;
        }
        else
        {
            cout << "First 10 primes: ";
            for (int i = 0; i < min(10, total_primes); i++)
            {
                cout << all_primes[i] << " ";
            }
            cout << endl;
        }
    }

    MPI_Finalize();
    return 0;
}