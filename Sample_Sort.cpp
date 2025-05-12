#include <iostream>
#include <vector>
#include <cstdlib>
#include <mpi.h>
#include <ctime>
#include <climits>
#include <fstream>

using namespace std;


void quicksort(int *arr, int low, int high)
{
    if (low < high)
    {
        int pivot = arr[high], i = low - 1;
        for (int j = low; j < high; j++)
        {
            if (arr[j] <= pivot)
            {
                i++;
                int tmp = arr[i];
                arr[i] = arr[j];
                arr[j] = tmp;
            }
        }
        int tmp = arr[i + 1];
        arr[i + 1] = arr[high];
        arr[high] = tmp;
        quicksort(arr, low, i);
        quicksort(arr, i + 2, high);
    }
}

void calculate_counts_and_displs(int *send_counts, int *send_displs, int size, int total_size)
{
    int base = total_size / size, rem = total_size % size;
    for (int i = 0; i < size; i++)
    {
        send_counts[i] = base + (i < rem ? 1 : 0);
        send_displs[i] = (i == 0) ? 0 : send_displs[i - 1] + send_counts[i - 1];
    }
}

void select_local_samples(int *local_array, int local_size, int *local_samples, int sample_size)
{
    for (int i = 0; i < sample_size; i++)
    {
        local_samples[i] = local_array[(i + 1) * local_size / (sample_size + 1)];
    }
}

void select_splitters(int *samples, int total_samples, int *splitters, int size)
{
    quicksort(samples, 0, total_samples - 1);
    for (int i = 0; i < size - 1; i++)
    {
        splitters[i] = samples[(i + 1) * (total_samples / size)];
    }
    splitters[size - 1] = INT_MAX;
}

void partition_data(int *local_array, int local_size, int *splitters, int size,
                    int *partition_counts, int *send_buf, int *send_displs)
{
    int *temp_displs = (int *)calloc(size, sizeof(int));
    for (int i = 0; i < local_size; i++)
    {
        int j = 0;
        while (j < size - 1 && local_array[i] > splitters[j])
            j++;
        partition_counts[j]++;
    }

    send_displs[0] = 0;
    for (int i = 1; i < size; i++)
        send_displs[i] = send_displs[i - 1] + partition_counts[i - 1];

    for (int i = 0; i < size; i++)
        temp_displs[i] = send_displs[i];

    for (int i = 0; i < local_size; i++)
    {
        int j = 0;
        while (j < size - 1 && local_array[i] > splitters[j])
            j++;
        send_buf[temp_displs[j]++] = local_array[i];
    }
    free(temp_displs);
}

void gather_sorted_data(int *recv_buf, int recv_size, int rank, int size,
                        int *final_array, int *all_sizes, int *displs)
{
    MPI_Gather(&recv_size, 1, MPI_INT, all_sizes, 1, MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0)
    {
        displs[0] = 0;
        for (int i = 1; i < size; i++)
            displs[i] = displs[i - 1] + all_sizes[i - 1];
    }

    MPI_Gatherv(recv_buf, recv_size, MPI_INT,
                final_array, all_sizes, displs, MPI_INT,
                0, MPI_COMM_WORLD);
}

// Wrapper function to handle file I/O, data distribution, and sample sort execution
bool runSampleSort(const char* inputFile, const char* outputFile, int rank, int size, MPI_Comm comm) {
    // Variables
    int *array = NULL;
    int array_size = 0;
    bool is_error = false;
    
    // Root process reads input file
    if (rank == 0) {
        // Read data from input file
        ifstream file(inputFile);
        vector<int> data;
        int el;
        while (file >> el) {
            data.push_back(el);
        }
        file.close();
        
        // Set array size
        array_size = data.size();
        
        // Check if size is valid
        if (array_size <= 0) {
            cout << "Error: Invalid input array size\n";
            is_error = true;
        }
        
        // Allocate array and copy data
        if (!is_error) {
            array = (int *)malloc(array_size * sizeof(int));
            if (!array) {
                cout << "Error: Memory allocation failed\n";
                is_error = true;
            } else {
                for (int i = 0; i < array_size; i++) {
                    array[i] = data[i];
                }
                
                
                // Also write to output file
                ofstream outFile(outputFile);
                outFile << "Unsorted array: ";
                for (int i = 0; i < array_size; i++) {
                    outFile << array[i] << " ";
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
    
    // Start timing
    MPI_Barrier(comm);
    double start_time = MPI_Wtime();
    
    // Calculate counts and displacements for scattering data
    int *send_counts = (int *)malloc(size * sizeof(int));
    int *send_displs = (int *)malloc(size * sizeof(int));
    calculate_counts_and_displs(send_counts, send_displs, size, array_size);
    
    // Allocate local array for each process
    int local_size = send_counts[rank];
    int *local_array = (int *)malloc(local_size * sizeof(int));
    
    // Scatter data to all processes
    MPI_Scatterv(array, send_counts, send_displs, MPI_INT,
                local_array, local_size, MPI_INT, 0, comm);
    
    // Sort local data
    quicksort(local_array, 0, local_size - 1);
    
    // Select and gather samples
    int sample_size = size - 1;
    int *local_samples = (int *)malloc(sample_size * sizeof(int));
    select_local_samples(local_array, local_size, local_samples, sample_size);
    
    int *samples = NULL;
    if (rank == 0) {
        samples = (int *)malloc(sample_size * size * sizeof(int));
    }
    MPI_Gather(local_samples, sample_size, MPI_INT,
              samples, sample_size, MPI_INT, 0, comm);
    
    // Select and broadcast splitters
    int *splitters = (int *)malloc(size * sizeof(int));
    if (rank == 0) {
        select_splitters(samples, sample_size * size, splitters, size);
    }
    MPI_Bcast(splitters, size, MPI_INT, 0, comm);
    
    // Partition data based on splitters
    int *partition_counts = (int *)calloc(size, sizeof(int));
    int *send_buf = (int *)malloc(local_size * sizeof(int));
    int *send_displs_local = (int *)calloc(size, sizeof(int));
    partition_data(local_array, local_size, splitters, size,
                  partition_counts, send_buf, send_displs_local);
    
    // Exchange data between processes
    int *recv_counts = (int *)malloc(size * sizeof(int));
    MPI_Alltoall(partition_counts, 1, MPI_INT,
                recv_counts, 1, MPI_INT, comm);
    
    // Calculate total data to receive
    int recv_size = 0;
    for (int i = 0; i < size; i++) {
        recv_size += recv_counts[i];
    }
    
    // Prepare buffers for receiving data
    int *recv_buf = (int *)malloc(recv_size * sizeof(int));
    int *recv_displs = (int *)calloc(size, sizeof(int));
    for (int i = 1; i < size; i++) {
        recv_displs[i] = recv_displs[i - 1] + recv_counts[i - 1];
    }
    
    // Exchange partitioned data
    MPI_Alltoallv(send_buf, partition_counts, send_displs_local, MPI_INT,
                 recv_buf, recv_counts, recv_displs, MPI_INT, comm);
    
    // Sort received data
    quicksort(recv_buf, 0, recv_size - 1);
    
    // Gather final sorted data
    int *all_sizes = NULL;
    int *displs = NULL;
    if (rank == 0) {
        all_sizes = (int *)malloc(size * sizeof(int));
        displs = (int *)malloc(size * sizeof(int));
        
        // Ensure array is allocated for final sorted data
        if (array == NULL) {
            array = (int *)malloc(array_size * sizeof(int));
        }
    }
    
    gather_sorted_data(recv_buf, recv_size, rank, size, array, all_sizes, displs);
    
    // End timing
    double end_time = MPI_Wtime();
    MPI_Barrier(comm);
    
    // Write results to file
    if (rank == 0) {
        // Calculate execution time
        double duration = (end_time - start_time) * 1000; // Convert to milliseconds
        cout << "Sample Sort execution time: " << duration << " ms\n";
        
        
        // Write to output file
        ofstream outFile(outputFile, ios::app);
        outFile << "Sorted array: ";
        for (int i = 0; i < array_size; i++) {
            outFile << array[i] << " ";
        }
        outFile << endl;
        outFile.close();
        
        // Clean up root process allocations
        free(array);
        free(all_sizes);
        free(displs);
    }
    
    // Clean up common allocations
    free(send_counts);
    free(send_displs);
    free(local_array);
    free(local_samples);
    if (rank == 0) free(samples);
    free(splitters);
    free(partition_counts);
    free(send_buf);
    free(send_displs_local);
    free(recv_counts);
    free(recv_buf);
    free(recv_displs);
    
    return true;
}

// int main(int argc, char *argv[])
// {
//     MPI_Init(&argc, &argv);
//     int rank, size;
//     MPI_Comm_rank(MPI_COMM_WORLD, &rank);
//     MPI_Comm_size(MPI_COMM_WORLD, &size);

//     srand(time(NULL) + rank); // Unique seed per process

//     if (argc != 3) {
//         if (rank == 0) {
//             cout << "Usage: " << argv[0] << " <input_file> <output_file>\n";
//         }
//         MPI_Finalize();
//         return 1;
//     }

//     const char* inputFile = argv[1];
//     const char* outputFile = argv[2];

//     bool success = runSampleSort(inputFile, outputFile, rank, size, MPI_COMM_WORLD);

//     MPI_Finalize();
//     return success ? 0 : 1;
// }
