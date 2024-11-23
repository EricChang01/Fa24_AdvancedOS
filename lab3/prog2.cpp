#include <iostream>
#include <chrono>
#include <string>

using namespace std;
using namespace chrono;

#define ARRSIZE 2000000
int main(int argc, char* argv[]){
    int arr[ARRSIZE];
    int t = stoi(argv[1]);
    int pos = 0;
    auto start = high_resolution_clock::now();
    switch (t) {
        case 0:
        for (int i=0; i<ARRSIZE; i++)
            arr[i] = i;
        break;

        case 1:
        for (int i=0; i<ARRSIZE; i++){
            arr[pos] = pos;
            pos += 1024;
            pos %= ARRSIZE;
        }
        break;

        case 2:
        for (int i=0; i<ARRSIZE; i++) {
            int r = rand() % ARRSIZE;
            arr[r] = r;
        }
        break;
    }

    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    cout << "Execution time: " << duration.count() << " microseconds" << endl;
}