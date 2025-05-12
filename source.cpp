#include <iostream>
#include <fstream>
#include <vector>
#include <mpi.h>

using namespace std;

extern void parallelPrimeSearch(int start, int end);
extern void bitonicSortParallel(vector<int>& local_data, int local_n, int total_n, int rank, int size, MPI_Comm comm);
extern bool runBitonicSort(const char* inputFile, const char* outputFile, int rank, int size, MPI_Comm comm);
extern bool runSampleSort(const char* inputFile, const char* outputFile, int rank, int size, MPI_Comm comm);
extern bool runQuickSearch(const char* inputFile, const char* outputFile, int target, int rank, int size, MPI_Comm comm);
extern bool runRadixSort(const char* inputFile, const char* outputFile, int rank, int size, MPI_Comm comm); // Add Radix Sort declaration


// Function to read array data for Sorting and searching algorithms
vector<int> readArrayData(const char *filename)
{
    ifstream file(filename);
    vector<int> data;
    while (!file.eof()) {
        int el;
        file >> el;
        data.push_back(el);
    }
    file.close();
    return data;
}

// Function to read range data for Prime Search
pair<int, int> readRangeData(const char *filename)
{
    ifstream file(filename);
    int start, end;
    file >> start >> end;
    file.close();
    return {start, end};
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    int rank, size, choice = -1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    char tryAnother = 'y';

    while (tryAnother == 'y' || tryAnother == 'Y')
    {

        bool is_error = false;
        if (rank == 0)
        {
            cout << "\n========================================\n";
            cout << "Parallel Algorithms with MPI\n";
            cout << "========================================\n";
            cout << "0. Exit\n";
            cout << "1. Quick Search\n";
            cout << "2. Prime Number Search\n";
            cout << "3. Bitonic Sort\n";
            cout << "4. Radix Sort\n";
            cout << "5. Sample Sort\n";
            cout << "Enter choice: ";
            cin >> choice;
        }

        MPI_Bcast(&choice, 1, MPI_INT, 0, MPI_COMM_WORLD);

        if (choice == 0)
        {
            if (rank == 0)
            {
                cout << "Exiting program...\n";
            }
            break;
        }


        switch (choice)
        {
        case 1:
        {
            int target = 0;
            if (rank == 0) {
                cout << "Enter Search Target: ";
                cin >> target;
                cout << "Running Quick Search...\n";
            }
            
            // Broadcast the target value to all processes
            MPI_Bcast(&target, 1, MPI_INT, 0, MPI_COMM_WORLD);
            
            // Call the wrapper function that handles everything
            bool success = runQuickSearch("Performance_Results/inputs/search_large.txt", "out.txt", target, rank, size, MPI_COMM_WORLD);
            
            // Set error flag if search failed
            is_error = !success;
            break;
        }

        case 2:
        {
            if (rank == 0)
            {
                cout << "Running Prime Number Search...\n";
            }

            int start = 0, end = 0;
            if (rank == 0)
            {
                auto range = readRangeData("in.txt");
                start = range.first;
                end = range.second;
            }

            MPI_Bcast(&start, 1, MPI_INT, 0, MPI_COMM_WORLD);
            MPI_Bcast(&end, 1, MPI_INT, 0, MPI_COMM_WORLD);

            parallelPrimeSearch(start, end);
            break;
        }
        case 3:
        {
            if (rank == 0) {
                cout << "Running Bitonic Sort...\n";
            }
            
            // Call the wrapper function that handles everything
            bool success = runBitonicSort("in.txt", "out.txt", rank, size, MPI_COMM_WORLD);
            
            // Set error flag if sorting failed
            is_error = !success;
            break;
        }

        case 4:
        {
            if (rank == 0) {
                cout << "Running Radix Sort...\n";
            }
            
            // Call the wrapper function that handles everything
            bool success = runRadixSort("Performance_Results/inputs/prime_small.txt", "out.txt", rank, size, MPI_COMM_WORLD);
            
            // Set error flag if sorting failed
            is_error = !success;
            break;
        }

        case 5:
        {
            if (rank == 0) {
                cout << "Running Sample Sort...\n";
            }
            
            // Call the wrapper function that handles everything
            bool success = runSampleSort("Performance_Results/inputs/small_random.txt", "out.txt", rank, size, MPI_COMM_WORLD);

            // Set error flag if sorting failed
            is_error = !success;
            break;
        }

        default:
            if (rank == 0)
            {
                cout << "Invalid choice! Please try again.\n";
            }
            break;
        }


        if (rank == 0 && choice > 0 && choice <= 5 && !is_error)
        {
            cout << "Algorithm completed successfully!\n";
            cout << "Results written to out.txt\n";

        }

        is_error = false; 

        if (rank == 0)
        {
            // Ask if user wants to try another algorithm
            cout << "\nWant to try another algorithm? (y/n): ";
            cin >> tryAnother;
        }

        // Broadcast the user's choice to all processes
        MPI_Bcast(&tryAnother, 1, MPI_CHAR, 0, MPI_COMM_WORLD);

        // Synchronize all processes before next iteration
        MPI_Barrier(MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
}