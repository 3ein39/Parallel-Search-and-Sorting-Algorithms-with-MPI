#include <iostream>
#include <vector>
#include <mpi.h>
using namespace std;

int getMax(const vector<int>& arr) {
    if (arr.empty()) return 0;
    int maxElement = arr[0];
    for (int i = 1; i < arr.size(); i++) {
        if (arr[i] > maxElement)
            maxElement = arr[i];
    }
    return maxElement;
}

void countingSort(vector<int>& arr, int exp) {
    if (arr.empty()) return;
    
    int n = arr.size();
    vector<int> output(n);
    vector<int> count(10, 0);

    // Count occurrences
    for (int i = 0; i < n; i++) {
        count[(arr[i] / exp) % 10]++;
    }

    // Calculate cumulative count
    for (int i = 1; i < 10; i++) {
        count[i] += count[i - 1];
    }

    // Build output array
    for (int i = n - 1; i >= 0; i--) {
        int digit = (arr[i] / exp) % 10;
        output[count[digit] - 1] = arr[i];
        count[digit]--;
    }

    arr = output;
}

void radixSortParallel(vector<int>& local_arr, int maxElement, int rank, int size) {
    for (int exp = 1; maxElement / exp > 0; exp *= 10) {
        // Sort locally based on current digit
        countingSort(local_arr, exp);

        // Count elements for each digit locally
        vector<int> local_count(10, 0);
        for (int num : local_arr) {
            local_count[(num / exp) % 10]++;
        }

        // Get global counts
        vector<int> global_count(10, 0);
        MPI_Allreduce(local_count.data(), global_count.data(), 10, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

        // Calculate boundaries for each digit
        vector<int> boundaries(11, 0);
        for (int i = 1; i <= 10; i++) {
            boundaries[i] = boundaries[i-1] + global_count[i-1];
        }

        // Calculate target processes for each element
        vector<int> send_counts(size, 0);
        for (int num : local_arr) {
            int digit = (num / exp) % 10;
            int pos = boundaries[digit];
            int target = (pos * size) / boundaries[10];
            if (target >= size) target = size - 1;
            send_counts[target]++;
        }

        // Exchange counts
        vector<int> recv_counts(size);
        MPI_Alltoall(send_counts.data(), 1, MPI_INT, recv_counts.data(), 1, MPI_INT, MPI_COMM_WORLD);

        // Calculate displacements
        vector<int> send_displs(size, 0);
        vector<int> recv_displs(size, 0);
        for (int i = 1; i < size; i++) {
            send_displs[i] = send_displs[i-1] + send_counts[i-1];
            recv_displs[i] = recv_displs[i-1] + recv_counts[i-1];
        }

        // Prepare send buffer
        vector<int> send_buf(local_arr.size());
        vector<int> next_pos = send_displs;

        for (int num : local_arr) {
            int digit = (num / exp) % 10;
            int pos = boundaries[digit];
            int target = (pos * size) / boundaries[10];
            if (target >= size) target = size - 1;
            send_buf[next_pos[target]++] = num;
        }

        // Exchange data
        int total_recv = 0;
        for (int count : recv_counts) total_recv += count;
        
        vector<int> recv_buf(total_recv);
        MPI_Alltoallv(send_buf.data(), send_counts.data(), send_displs.data(), MPI_INT,
                      recv_buf.data(), recv_counts.data(), recv_displs.data(), MPI_INT,
                      MPI_COMM_WORLD);

        local_arr = recv_buf;
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Test array
    vector<int> arr;
    int n;

    if (rank == 0) {
        arr = {423, 7, 21, 8, 184, 688, 0, 245};
        n = arr.size();
        
        cout << "Input array: ";
        for (int x : arr) cout << x << " ";
        cout << endl;
    }

    // Broadcast array size
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Calculate local array size
    int local_size = n / size;
    if (rank < n % size) local_size++;

    // Distribute data
    vector<int> local_arr(local_size);
    vector<int> send_counts(size, n / size);
    vector<int> send_displs(size, 0);

    for (int i = 0; i < n % size; i++) {
        send_counts[i]++;
    }
    for (int i = 1; i < size; i++) {
        send_displs[i] = send_displs[i-1] + send_counts[i-1];
    }

    MPI_Scatterv(arr.data(), send_counts.data(), send_displs.data(), MPI_INT,
                 local_arr.data(), local_size, MPI_INT, 0, MPI_COMM_WORLD);

    // Find global maximum
    int local_max = local_arr.empty() ? 0 : getMax(local_arr);
    int global_max;
    MPI_Allreduce(&local_max, &global_max, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);

    // Sort
    radixSortParallel(local_arr, global_max, rank, size);

    // Gather results
    vector<int> result(n);
    MPI_Gatherv(local_arr.data(), local_size, MPI_INT,
                result.data(), send_counts.data(), send_displs.data(), MPI_INT, 0, MPI_COMM_WORLD);

    if (rank == 0) {
        cout << "Sorted array: ";
        for (int x : result) cout << x << " ";
        cout << endl;
    }

    MPI_Finalize();
    return 0;
}