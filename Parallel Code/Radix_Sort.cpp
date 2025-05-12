#include <iostream>
#include <vector>
#include <mpi.h>
using namespace std;

// دالة لإيجاد أكبر عنصر في المصفوفة (نفس اللي في الكود الـ sequential)
int getMax(const vector<int>& arr) {
    int maxElement = arr[0];
    for (int i = 1; i < arr.size(); i++) {
        if (arr[i] > maxElement)
            maxElement = arr[i];
    }
    return maxElement;
}

// دالة Counting Sort محلية بناءً على الرقم (digit)
void countingSort(vector<int>& arr, int pos, int* count) {
    int n = arr.size();
    vector<int> output(n);
    fill(count, count + 10, 0); // تهيئة مصفوفة العد

    // عد الأرقام بناءً على الرقم الحالي
    for (int i = 0; i < n; i++) {
        int digit = (arr[i] / pos) % 10;
        count[digit]++;
    }

    // تراكم العد
    for (int i = 1; i < 10; i++) {
        count[i] += count[i - 1];
    }

    // ترتيب العناصر في المصفوفة المؤقتة
    for (int i = n - 1; i >= 0; i--) {
        int digit = (arr[i] / pos) % 10;
        output[count[digit] - 1] = arr[i];
        count[digit]--;
    }

    // نسخ النتيجة إلى المصفوفة الأصلية
    for (int i = 0; i < n; i++) {
        arr[i] = output[i];
    }
}

// دالة Radix Sort الرئيسية لكل عملية
void radixSortParallel(vector<int>& local_arr, int maxElement, int rank, int size) {
    int count[10];

    // كل مرحلة بناءً على الرقم (digit)
    for (int pos = 1; maxElement / pos > 0; pos *= 10) {
        // الخطوة 1: طبق Counting Sort محليًا
        countingSort(local_arr, pos, count);

        // الخطوة 2: جمع مصفوفات العد من كل العمليات
        int global_count[10];
        MPI_Allreduce(count, global_count, 10, MPI_INT, MPI_SUM, MPI_COMM_WORLD);

        // الخطوة 3: حساب النطاقات (ranges) لكل عملية
        int local_size = local_arr.size();
        vector<int> send_counts(size, 0);
        vector<int> send_displs(size, 0);
        vector<int> digit_counts(10, 0);

        // إعادة عد الأرقام بناءً على الرقم الحالي
        for (int i = 0; i < local_size; i++) {
            int digit = (local_arr[i] / pos) % 10;
            digit_counts[digit]++;
        }

        // حساب عدد العناصر التي سترسل إلى كل عملية
        int total = 0;
        for (int i = 0; i < 10; i++) {
            int start = total;
            total += global_count[i];
            if (total == 0) continue;
            int per_proc = total / size;
            int extra = total % size;
            for (int j = 0; j < size; j++) {
                int count = per_proc + (j < extra ? 1 : 0);
                if (i == 9 && j == size - 1)
                    count = total - start - accumulate(send_counts.begin(), send_counts.end(), 0);
                send_counts[j] += digit_counts[i] * count / (total - start);
            }
        }

        // حساب الإزاحات (displacements)
        send_displs[0] = 0;
        for (int i = 1; i < size; i++) {
            send_displs[i] = send_displs[i - 1] + send_counts[i - 1];
        }

        // الخطوة 4: تبادل البيانات بين العمليات
        vector<int> recv_buf(local_size);
        MPI_Alltoallv(local_arr.data(), send_counts.data(), send_displs.data(), MPI_INT,
                      recv_buf.data(), send_counts.data(), send_displs.data(), MPI_INT, MPI_COMM_WORLD);

        // تحديث المصفوفة المحلية
        local_arr = recv_buf;
    }
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    vector<int> arr;
    int n;

    // العملية 0 تقرأ البيانات
    if (rank == 0) {
        arr = {423, 7, 21, 8, 184, 688, 0, 245}; // مثال للمصفوفة
        n = arr.size();
    }

    // بث حجم المصفوفة إلى كل العمليات
    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // حساب حجم الجزء المحلي لكل عملية
    int local_size = n / size;
    if (rank < n % size) local_size++;

    vector<int> local_arr(local_size);

    // توزيع البيانات
    vector<int> send_counts(size, n / size);
    vector<int> send_displs(size, 0);
    for (int i = 0; i < n % size; i++) send_counts[i]++;
    for (int i = 1; i < size; i++) {
        send_displs[i] = send_displs[i - 1] + send_counts[i - 1];
    }

    MPI_Scatterv(arr.data(), send_counts.data(), send_displs.data(), MPI_INT,
                 local_arr.data(), local_size, MPI_INT, 0, MPI_COMM_WORLD);

    // إيجاد أكبر عنصر محليًا
    int local_max = local_arr.empty() ? 0 : getMax(local_arr);
    int global_max;

    // جمع أكبر عنصر من كل العمليات
    MPI_Reduce(&local_max, &global_max, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Bcast(&global_max, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // تنفيذ Radix Sort المتوازي
    radixSortParallel(local_arr, global_max, rank, size);

    // جمع النتائج في العملية 0
    vector<int> final_arr(n);
    MPI_Gatherv(local_arr.data(), local_size, MPI_INT,
                final_arr.data(), send_counts.data(), send_displs.data(), MPI_INT, 0, MPI_COMM_WORLD);

    // طباعة النتيجة
    if (rank == 0) {
        cout << "Sorted array: ";
        for (int i = 0; i < n; i++) {
            cout << final_arr[i] << " ";
        }
        cout << endl;
    }

    MPI_Finalize();
    return 0;
}