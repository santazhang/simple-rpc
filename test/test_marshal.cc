#include <iostream>

#include <stdio.h>

#include "marshal.h"

using namespace std;

int main(int argc, char* argv[]) {
    Marshal m, mcopy;
    m << 1 << 2 << 3 << 3.14 << "hello";
    mcopy = m;
    Marshal u(m);
    i32 a, b, c;
    double f;
    string s;
    u >> a >> b >> c >> f >> s;
    printf("%d %d %d %lf\n", a, b, c, f);
    cout << s << endl;
    mcopy >> a;
    Marshal u2;
    u2 = u2 = mcopy;
    u2 >> b >> c >> f >> s;
    printf("%d %d %lf\n", b, c, f);
    cout << s << endl;
    cout << u2.size() << endl;
    return 0;
}

