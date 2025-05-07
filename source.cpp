#include <iostream>
#include <fstream>
#include <vector>
#include <mpi.h>

using namespace std;

extern void parallelPrimeSearch(int start, int end);

// Function to read array data for Sorting and searching algorithms
vector<int> readArrayData(const char *filename)
{
    ifstream file(filename);
    int size;
    file >> size;
    vector<int> data(size);
    for (int i = 0; i < size; ++i)
    {
        file >> data[i];
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

    double start_time, end_time;
    char tryAnother = 'y';

    while (tryAnother == 'y' || tryAnother == 'Y')
    {
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

        start_time = MPI_Wtime();

        switch (choice)
        {
        case 1:
        {
            if (rank == 0)
            {
                int target;
                cout << "Enter Search Target: ";
                cin >> target;
                cout << "Running Quick Search...\n";
                // Read array data from file using the function readArrayData
                // Call quick search with the data
            }
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
            if (rank == 0)
            {
                cout << "Running Bitonic Sort...\n";
                
                // Read array data from file using the function readArrayData
                // Call bitonic sort function here
            }
            break;
        }

        case 4:
        {
            if (rank == 0)
            {
                cout << "Running Radix Sort...\n";
                // Read array data from file using the function readArrayData
                // Call radix sort function here
            }
            break;
        }

        case 5:
        {
            if (rank == 0)
            {
                cout << "Running Sample Sort...\n";
                // Read array data from file using the function readArrayData
                // Call sample sort function here
            }
            break;
        }

        default:
            if (rank == 0)
            {
                cout << "Invalid choice! Please try again.\n";
            }
            break;
        }

        // Record end time and calculate execution time
        end_time = MPI_Wtime();

        if (rank == 0 && choice > 0 && choice <= 5)
        {
            cout << "Algorithm completed successfully!\n";
            cout << "Execution time: " << (end_time - start_time) * 1000 << " ms\n";
            cout << "Results written to out.txt\n";

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