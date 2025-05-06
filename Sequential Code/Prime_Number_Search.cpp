#include <iostream>
#include <vector>

using namespace std;
bool isPrime(int n)
{
    if (n <= 1)
        return false;
    for (int i = 2; i * i <= n; i++)
    {
        if (n % i == 0)
            return false;
    }
    return true;
}

void findPrimesInRange(int start, int end)
{
    cout << "Prime numbers between " << start << " and " << end << " are: ";
    for (int i = start; i <= end; i++)
    {
        if (isPrime(i))
        {
            cout << i << " ";
        }
    }
    cout << endl;
}

int main()
{
    int start, end;
    start = 10; // Example start value
    end = 50;   // Example end value
    findPrimesInRange(start, end);

    return 0;
}