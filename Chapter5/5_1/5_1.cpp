#include <iostream>

using namespace std;

bool big_endian()
{
    union {
        short int_val;
        char str_val[sizeof(int_val)];
    } test;
    test.int_val = 0x0102;
    if (test.str_val[0] == 1 && test.str_val[1] == 2) {
        return true;
    } else {
        return false;
    }
}

int main()
{
    ios::sync_with_stdio(false);

    if (big_endian()) {
        cout << "big endian" << "\n";
    } else {
        cout << "little endian" << "\n";
    }

    return 0;
}

