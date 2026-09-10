
#include <iostream>
#include <fstream> // ofstream para escribir en archivo
#include "containers/vector.h"
#include "Demos.h"
using namespace std;

void DemoVector() {
    Vector<TX> vec;
    for (TX i = 0; i < 10; ++i) {
        vec.push_back(i);
    }

    cout << "Vector using print: ";
    vec.print(cout);
    cout << endl;

    ofstream of("vector.txt");
    vec.print(of);
    of.close();

    cout << "Vector using cout directly: ";
    cout << vec << endl;

    vec.apply([](auto i) { cout << (i + 1); });
}