#include <stdio.h>
#include <string.h>
#include "utils.h"


int main() {
    Buffer b;
    const char* s1 = "hello,";
    b.write(s1, strlen(s1));
    char s2[10];
    char s3[10];
    int n = b.consume(s2, 100);
    printf("%d %s\n", n, s2);
    n = b.peek(s3, 100);
    printf("%d %s\n", n, s3);
    return 0;
}
