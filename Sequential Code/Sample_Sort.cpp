#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <limits.h>

#define ARRAY_SIZE 50
#define NUM_BUCKETS 4

// Function to generate random numbers
void generate_random(int *arr, int size)
{
    for (int i = 0; i < size; i++)
    {
        arr[i] = rand() % 100;
    }
}

// Function to perform quicksort
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

// Function to select samples
int *select_samples(int *array, int array_size, int num_buckets, int *sample_size_out)
{
    int sample_size = (num_buckets - 1) * num_buckets;
    int *samples = (int *)malloc(sample_size * sizeof(int));
    if (!samples)
    {
        fprintf(stderr, "Error: Failed to allocate memory for samples\n");
        exit(1);
    }

    // Select samples at regular intervals
    int sample_step = array_size / (sample_size + 1);
    for (int i = 0; i < sample_size; i++)
    {
        samples[i] = array[(i + 1) * sample_step];
    }

    // Sort samples
    quicksort(samples, 0, sample_size - 1);
    *sample_size_out = sample_size;
    return samples;
}

// Function to select splitters
int *select_splitters(int *samples, int sample_size, int num_buckets)
{
    int *splitters = (int *)malloc(num_buckets * sizeof(int));
    if (!splitters)
    {
        fprintf(stderr, "Error: Failed to allocate memory for splitters\n");
        exit(1);
    }

    for (int i = 0; i < num_buckets - 1; i++)
    {
        splitters[i] = samples[(i + 1) * sample_size / num_buckets];
    }
    splitters[num_buckets - 1] = INT_MAX;
    return splitters;
}

// Function to partition array into buckets
void partition_into_buckets(int *array, int array_size, int *splitters, int num_buckets,
                            int ***buckets_out, int **bucket_sizes_out)
{
    int *bucket_sizes = (int *)calloc(num_buckets, sizeof(int));
    if (!bucket_sizes)
    {
        fprintf(stderr, "Error: Failed to allocate memory for bucket sizes\n");
        exit(1);
    }

    // Count elements per bucket
    for (int i = 0; i < array_size; i++)
    {
        int j = 0;
        while (j < num_buckets - 1 && array[i] > splitters[j])
        {
            j++;
        }
        bucket_sizes[j]++;
    }

    // Allocate buckets
    int **buckets = (int **)malloc(num_buckets * sizeof(int *));
    if (!buckets)
    {
        fprintf(stderr, "Error: Failed to allocate memory for buckets\n");
        exit(1);
    }
    for (int i = 0; i < num_buckets; i++)
    {
        buckets[i] = (int *)malloc(bucket_sizes[i] * sizeof(int));
        if (!buckets[i])
        {
            fprintf(stderr, "Error: Failed to allocate memory for bucket %d\n", i);
            exit(1);
        }
    }

    // Distribute elements to buckets
    int *bucket_indices = (int *)calloc(num_buckets, sizeof(int));
    if (!bucket_indices)
    {
        fprintf(stderr, "Error: Failed to allocate memory for bucket indices\n");
        exit(1);
    }
    for (int i = 0; i < array_size; i++)
    {
        int j = 0;
        while (j < num_buckets - 1 && array[i] > splitters[j])
        {
            j++;
        }
        buckets[j][bucket_indices[j]++] = array[i];
    }
    free(bucket_indices);

    *buckets_out = buckets;
    *bucket_sizes_out = bucket_sizes;
}

// Function to merge buckets back into array
void merge_buckets(int *array, int **buckets, int *bucket_sizes, int num_buckets)
{
    int output_index = 0;
    for (int i = 0; i < num_buckets; i++)
    {
        for (int j = 0; j < bucket_sizes[i]; j++)
        {
            array[output_index++] = buckets[i][j];
        }
    }
}

int main()
{
    // Initialize random seed
    srand(time(NULL));

    // Allocate main array
    int *array = (int *)malloc(ARRAY_SIZE * sizeof(int));
    if (!array)
    {
        fprintf(stderr, "Error: Failed to allocate memory for array\n");
        return 1;
    }
    generate_random(array, ARRAY_SIZE);

    // Print unsorted array
    printf("Unsorted array:\n");
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        printf("%d ", array[i]);
    }
    printf("\n");

    // Step 1: Select samples
    int sample_size;
    int *samples = select_samples(array, ARRAY_SIZE, NUM_BUCKETS, &sample_size);

    // Step 2: Select splitters
    int *splitters = select_splitters(samples, sample_size, NUM_BUCKETS);

    // Step 3: Partition into buckets
    int **buckets;
    int *bucket_sizes;
    partition_into_buckets(array, ARRAY_SIZE, splitters, NUM_BUCKETS, &buckets, &bucket_sizes);

    // Step 4: Sort each bucket
    for (int i = 0; i < NUM_BUCKETS; i++)
    {
        quicksort(buckets[i], 0, bucket_sizes[i] - 1);
    }

    // Step 5: Merge buckets
    merge_buckets(array, buckets, bucket_sizes, NUM_BUCKETS);

    // Print sorted array
    printf("Sorted array:\n");
    for (int i = 0; i < ARRAY_SIZE; i++)
    {
        printf("%d ", array[i]);
    }
    printf("\n");

    // Cleanup
    free(array);
    free(samples);
    free(splitters);
    for (int i = 0; i < NUM_BUCKETS; i++)
    {
        free(buckets[i]);
    }
    free(buckets);
    free(bucket_sizes);

    return 0;
}