#include <iostream>

using namespace std;
#define ARRSIZE 100000
int main(){
    int arr[ARRSIZE];
    for(int i=0; i<ARRSIZE; i++){
        arr[i] = arr[i] + 1;
    }
}