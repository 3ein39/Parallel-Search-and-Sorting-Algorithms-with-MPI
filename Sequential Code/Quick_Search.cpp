#include <iostream>
#include <vector>
#include <random>
#include <algorithm>

// Function to partition the array around a pivot
int partition(std::vector<int>& arr, int low, int high) {
    int pivot = arr[high];
    int i = low - 1;
    for (int j = low; j < high; j++) {
        if (arr[j] <= pivot) {
            i++;
            std::swap(arr[i], arr[j]);
        }
    }
    std::swap(arr[i + 1], arr[high]);
    return i + 1;
}

// Sequential Quick Search
bool sequentialQuickSearch(std::vector<int>& arr, int target, int low, int high) {
    if (low > high) {
        return false;
    }
    int pivot_idx = partition(arr, low, high);
    if (arr[pivot_idx] == target) {
        return true;
    }
    else if (arr[pivot_idx] > target) {
        return sequentialQuickSearch(arr, target, low, pivot_idx - 1);
    }
    else {
        return sequentialQuickSearch(arr, target, pivot_idx + 1, high);
    }
}

int main(int argc, char* argv[]) {
    // Default values
    int n = 10000; // Array size

    // Handle dynamic input
    if (argc > 1) {
        n = std::atoi(argv[1]);
    }
    if (n <= 0) {
        std::cerr << "Invalid array size\n";
        return 1;
    }

    // Generate data
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(1, 1000);

    std::vector<int> arr(n);
    for (int i = 0; i < n; i++) {
        arr[i] = dis(gen); // Random numbers from 1 to 1000
    }
    int target = arr[dis(gen) % n]; // Random target from array

    // Perform search
    bool found = sequentialQuickSearch(arr, target, 0, n - 1);

    // Output results
    std::cout << "Sequential Quick Search (Target: " << target << "): "
              << (found ? "Found" : "Not Found") << "\n";

    return 0;
}