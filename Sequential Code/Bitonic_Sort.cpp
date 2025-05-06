#include <iostream>
#include <vector>
#include <chrono> 

using namespace std;
void bitonicMerge(vector<int> & arr, int low, int cnt, bool dir) {
    if (cnt <= 1) 
        return;

    int k = cnt / 2;
    for (int i = low; i < low + k; i++) {
        if (dir == (arr[i] > arr[i + k])) {
            swap(arr[i], arr[i + k]);
        }
    }
    bitonicMerge(arr, low, k, dir);
    bitonicMerge(arr, low + k, k, dir);
}

void bitonicSort(vector<int>& arr, int low, int cnt, bool dir) {
    if (cnt <= 1) 
        return;

    int k = cnt / 2;
    
    bitonicSort(arr, low, k, true);
    bitonicSort(arr, low + k, k, false);

    bitonicMerge(arr, low, cnt, dir);
}



int main() {
    
    // test 2^4 vector 
    int n = 16;
    vector<int> arr(n);
    for (int i = 0; i < n; i++) {
        arr[i] = rand() % 100;
    }

    cout << "Unsorted array: ";
    for (int i = 0; i < n; i++) {
        cout << arr[i] << " ";
    }
    cout << endl;

    // Start timing
    auto start_time = chrono::high_resolution_clock::now();
    
    // Sort the array in ascending order
    bitonicSort(arr, 0, n, true);
    
    // End timing
    auto end_time = chrono::high_resolution_clock::now();
    
    // Calculate duration
    auto duration = chrono::duration_cast<chrono::microseconds>(end_time - start_time);

    cout << "Sorted array: ";
    for (int i = 0; i < n; i++) {
        cout << arr[i] << " ";
    }
    cout << endl;

    // Display execution time
    cout << "Execution time: " << duration.count() << " microseconds" << endl
         << endl
    ;

    return 0;
}