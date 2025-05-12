#include <vector>
#include <cstdio>
#include <iostream>
#include <mpi.h>

using namespace std;

bool isPrime(int n)
{
    if (n <= 1)
        return false;
    for (int i = 2; i * i <= n; ++i)
        if (n % i == 0)
            return false;
    return true;
}

vector<int> findPrimes(int start, int end)
{
    vector<int> primes;
    for (int i = start; i <= end; ++i)
        if (isPrime(i))
            primes.push_back(i);
    return primes;
}

void parallelPrimeSearch(int start, int end)
{
    double start_time, end_time;
    start_time = MPI_Wtime();

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int range = end - start + 1;
    int chunk = range / size;
    int remainder = range % size;
    int local_start = start + rank * chunk + (rank < remainder ? rank : remainder);
    int local_end = local_start + chunk - 1 + (rank < remainder ? 1 : 0);

    vector<int> local_primes = findPrimes(local_start, local_end);
    int local_count = local_primes.size();

    vector<int> counts(size);
    MPI_Gather(&local_count, 1, MPI_INT, counts.data(), 1, MPI_INT, 0, MPI_COMM_WORLD);

    vector<int> displs(size);
    int total = 0;
    if (rank == 0)
    {
        for (int i = 0; i < size; ++i)
        {
            displs[i] = total;
            total += counts[i];
        }
    }

    vector<int> all_primes(total);
    MPI_Gatherv(local_primes.data(), local_count, MPI_INT, all_primes.data(), counts.data(), displs.data(), MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0)
    {
        end_time = MPI_Wtime();

        cout << "Execution time: " << (end_time - start_time) * 1000 << " ms\n";
        
        FILE *out = fopen("out.txt", "w");
        fprintf(out, "Prime Number Search Results:\n");
        fprintf(out, "Found %d primes between %d and %d\n", total, start, end);
        for (int i = 0; i < (total > 100 ? 10 : total); ++i)
            fprintf(out, "%d ", all_primes[i]);
        if (total > 100)
            fprintf(out, "... (and %d more)", total - 10);
        fclose(out);
    }
}