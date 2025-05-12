#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <time.h>
#include <limits.h>

#define ARRAY_SIZE 50

void generate_random(int *arr, int size)
{
    for (int i = 0; i < size; i++)
        arr[i] = rand() % 100;
}

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

void parallel_sort(int rank, int size)
{
    int *array = NULL;
    int *send_counts = (int *)malloc(size * sizeof(int));
    int *send_displs = (int *)malloc(size * sizeof(int));

    if (rank == 0)
    {
        array = (int *)malloc(ARRAY_SIZE * sizeof(int));
        generate_random(array, ARRAY_SIZE);
    }

    calculate_counts_and_displs(send_counts, send_displs, size, ARRAY_SIZE);

    int local_size = send_counts[rank];
    int *local_array = (int *)malloc(local_size * sizeof(int));

    MPI_Scatterv(array, send_counts, send_displs, MPI_INT,
                 local_array, local_size, MPI_INT, 0, MPI_COMM_WORLD);

    quicksort(local_array, 0, local_size - 1);

    int sample_size = size - 1;
    int *local_samples = (int *)malloc(sample_size * sizeof(int));
    select_local_samples(local_array, local_size, local_samples, sample_size);

    int *samples = (rank == 0) ? (int *)malloc(sample_size * size * sizeof(int)) : NULL;
    MPI_Gather(local_samples, sample_size, MPI_INT,
               samples, sample_size, MPI_INT, 0, MPI_COMM_WORLD);

    int *splitters = (int *)malloc(size * sizeof(int));
    if (rank == 0)
        select_splitters(samples, sample_size * size, splitters, size);

    MPI_Bcast(splitters, size, MPI_INT, 0, MPI_COMM_WORLD);

    int *partition_counts = (int *)calloc(size, sizeof(int));
    int *send_buf = (int *)malloc(local_size * sizeof(int));
    int *send_displs_local = (int *)calloc(size, sizeof(int));

    partition_data(local_array, local_size, splitters, size,
                   partition_counts, send_buf, send_displs_local);

    int *recv_counts = (int *)malloc(size * sizeof(int));
    MPI_Alltoall(partition_counts, 1, MPI_INT,
                 recv_counts, 1, MPI_INT, MPI_COMM_WORLD);

    int recv_size = 0;
    for (int i = 0; i < size; i++)
        recv_size += recv_counts[i];

    int *recv_buf = (int *)malloc(recv_size * sizeof(int));
    int *recv_displs = (int *)calloc(size, sizeof(int));
    for (int i = 1; i < size; i++)
        recv_displs[i] = recv_displs[i - 1] + recv_counts[i - 1];

    MPI_Alltoallv(send_buf, partition_counts, send_displs_local, MPI_INT,
                  recv_buf, recv_counts, recv_displs, MPI_INT, MPI_COMM_WORLD);

    quicksort(recv_buf, 0, recv_size - 1);

    int *all_sizes = (rank == 0) ? (int *)malloc(size * sizeof(int)) : NULL;
    int *displs = (rank == 0) ? (int *)malloc(size * sizeof(int)) : NULL;

    if (rank == 0 && array == NULL)
        array = (int *)malloc(ARRAY_SIZE * sizeof(int));

    gather_sorted_data(recv_buf, recv_size, rank, size,
                       array, all_sizes, displs);

    if (rank == 0)
    {
        printf("Sorted array:\n");
        for (int i = 0; i < ARRAY_SIZE; i++)
            printf("%d ", array[i]);
        printf("\n");
        free(array);
        free(all_sizes);
        free(displs);
    }

    // Free common allocations
    free(send_counts);
    free(send_displs);
    free(local_array);
    free(local_samples);
    free(samples);
    free(splitters);
    free(partition_counts);
    free(send_buf);
    free(send_displs_local);
    free(recv_counts);
    free(recv_buf);
    free(recv_displs);
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    srand(time(NULL) + rank); // Unique seed per process
    parallel_sort(rank, size);

    MPI_Finalize();
    return 0;
}
