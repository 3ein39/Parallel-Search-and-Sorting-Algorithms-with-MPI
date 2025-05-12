#include<iostream>
#include<vector>
using namespace std;

int getMax(const vector<int> &arr){
    int maxElement = arr[0];
    for(int i=1;i<arr.size();i++){
        if(arr[i]>maxElement)
        maxElement=arr[i];
    }
    return maxElement;
}
void countingSort(vector<int> & arr, int pos){
int n = arr.size();
vector<int> output(n);
int count[10]={0};

for(int i=0; i<n;i++){
    int digit=(arr[i]/pos)%10;
    count[digit]++;
}
for(int i=1 ; i<10;i++){
   count[i]+=count[i-1]; 
}
for(int i=n-1; i>=0; i--){
    int digit=(arr[i]/pos)%10;
    output[count[digit]-1]=arr[i];
    count[digit]--;
}
for(int i=0;i<n;i++){
    arr[i]=output[i];
}
}
void RadixSort(vector<int> &arr){
    int maxElement =getMax(arr);
    for(int pos=1;maxElement/pos>0;pos*=10){
        countingSort(arr,pos);
    }
}
int main(){
    vector<int> arr={423,7,21,8,184,688,0,245};
    RadixSort(arr);
    for(int i=0; i<arr.size();i++){
        cout<<arr[i]<<" ";
    }
}
