#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <limits.h>

#define ARRAY_SIZE 50

// Function to generate random numbers
void generate_random(int *arr, int size)
{
    for (int i = 0; i < size; i++)
    {
        arr[i] = rand() % 100;
    }
}

// Function to perform local quicksort
void quicksort(int *arr, int low, int high)
{
    if (low < high)
    {
        int pivot = arr[high];
        int i = low - 1;
        for (int j = low; j < high; j++)
        {
            if (arr[j] <= pivot)
            {
                i++;
                int temp = arr[i];
                arr[i] = arr[j];
                arr[j] = temp;
            }
        }
        int temp = arr[i + 1];
        arr[i + 1] = arr[high];
        arr[high] = temp;
        quicksort(arr, low, i);
        quicksort(arr, i + 2, high);
    }
}

int main(int argc, char *argv[])
{
    int rank, size;
    int *array = NULL;
    int *local_array = NULL;
    int local_size, remainder;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Initialize random seed
    srand(time(NULL) + rank);

    // Root process generates the array
    if (rank == 0)
    {
        array = (int *)malloc(ARRAY_SIZE * sizeof(int));
        generate_random(array, ARRAY_SIZE);
    }

    // Calculate local array size, handling remainder
    local_size = ARRAY_SIZE / size;
    remainder = ARRAY_SIZE % size;
    int *send_counts = (int *)malloc(size * sizeof(int));
    int *send_displs = (int *)malloc(size * sizeof(int));
    for (int i = 0; i < size; i++)
    {
        send_counts[i] = local_size + (i < remainder ? 1 : 0);
        send_displs[i] = (i == 0) ? 0 : send_displs[i - 1] + send_counts[i - 1];
    }
    local_size = send_counts[rank];
    local_array = (int *)malloc(local_size * sizeof(int));

    // Scatter the array to all processes
    MPI_Scatterv(array, send_counts, send_displs, MPI_INT, local_array, local_size, MPI_INT, 0, MPI_COMM_WORLD);

    // Local sorting
    quicksort(local_array, 0, local_size - 1);

    // Select samples
    int sample_size = size - 1;
    int *samples = NULL;
    if (rank == 0)
    {
        samples = (int *)malloc(sample_size * size * sizeof(int));
    }
    int *local_samples = (int *)malloc(sample_size * sizeof(int));
    for (int i = 0; i < sample_size; i++)
    {
        local_samples[i] = local_array[(i + 1) * local_size / (sample_size + 1)];
    }

    // Gather samples to root
    MPI_Gather(local_samples, sample_size, MPI_INT, samples, sample_size, MPI_INT, 0, MPI_COMM_WORLD);

    // Root process selects splitters
    int *splitters = NULL;
    if (rank == 0)
    {
        quicksort(samples, 0, sample_size * size - 1);
        splitters = (int *)malloc(size * sizeof(int));
        for (int i = 0; i < size - 1; i++)
        {
            splitters[i] = samples[(i + 1) * sample_size];
        }
        splitters[size - 1] = INT_MAX;
    }

    // Broadcast splitters
    splitters = (int *)malloc(size * sizeof(int));
    MPI_Bcast(splitters, size, MPI_INT, 0, MPI_COMM_WORLD);

    // Partition local array based on splitters
    int *partition_counts = (int *)calloc(size, sizeof(int));
    for (int i = 0; i < local_size; i++)
    {
        int j = 0;
        while (j < size - 1 && local_array[i] > splitters[j])
        {
            j++;
        }
        partition_counts[j]++;
    }

    // Prepare send buffer
    int *send_buf = (int *)malloc(local_size * sizeof(int));
    int *send_displs_local = (int *)calloc(size, sizeof(int));
    for (int i = 1; i < size; i++)
    {
        send_displs_local[i] = send_displs_local[i - 1] + partition_counts[i - 1];
    }
    int *temp_displs = (int *)calloc(size, sizeof(int));
    for (int i = 0; i < size; i++)
    {
        temp_displs[i] = send_displs_local[i];
    }
    for (int i = 0; i < local_size; i++)
    {
        int j = 0;
        while (j < size - 1 && local_array[i] > splitters[j])
        {
            j++;
        }
        send_buf[temp_displs[j]++] = local_array[i];
    }

    // Exchange data
    int *recv_counts = (int *)malloc(size * sizeof(int));
    MPI_Alltoall(partition_counts, 1, MPI_INT, recv_counts, 1, MPI_INT, MPI_COMM_WORLD);

    int recv_size = 0;
    for (int i = 0; i < size; i++)
    {
        recv_size += recv_counts[i];
    }
    int *recv_buf = (int *)malloc(recv_size * sizeof(int));
    int *recv_displs = (int *)calloc(size, sizeof(int));
    for (int i = 1; i < size; i++)
    {
        recv_displs[i] = recv_displs[i - 1] + recv_counts[i - 1];
    }

    MPI_Alltoallv(send_buf, partition_counts, send_displs_local, MPI_INT,
                  recv_buf, recv_counts, recv_displs, MPI_INT, MPI_COMM_WORLD);

    // Final local sort
    quicksort(recv_buf, 0, recv_size - 1);

    // Gather sorted sizes
    int *all_sizes = NULL;
    if (rank == 0)
    {
        all_sizes = (int *)malloc(size * sizeof(int));
    }
    MPI_Gather(&recv_size, 1, MPI_INT, all_sizes, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Gather sorted data
    int *displs = NULL;
    if (rank == 0)
    {
        displs = (int *)malloc(size * sizeof(int));
        displs[0] = 0;
        for (int i = 1; i < size; i++)
        {
            displs[i] = displs[i - 1] + all_sizes[i - 1];
        }
        if (!array)
        {
            array = (int *)malloc(ARRAY_SIZE * sizeof(int));
        }
    }

    MPI_Gatherv(recv_buf, recv_size, MPI_INT, array, all_sizes, displs, MPI_INT, 0, MPI_COMM_WORLD);

    // Print small portion of sorted array
    if (rank == 0)
    {
        printf("sorted array:\n");
        for (int i = 0; i < ARRAY_SIZE; i++)
        {
            printf("%d ", array[i]);
        }
        printf("\n");
    }

    // Cleanup
    if (rank == 0)
    {
        free(array);
    }
    free(local_array);
    free(local_samples);
    free(samples);
    free(splitters);
    free(partition_counts);
    free(send_buf);
    free(send_displs_local);
    free(temp_displs);
    free(recv_counts);
    free(recv_buf);
    free(recv_displs);
    free(all_sizes);
    free(displs);
    free(send_counts);
    free(send_displs);

    MPI_Finalize();
    return 0;
}