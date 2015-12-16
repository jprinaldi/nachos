#include "syscall.h"

void sleep(int n) {
    int i, j;
    for (i = 0; i < n; i++)
        for (j = 0; j < n; j++)
            continue;
}

int main() {
    while (1) {
        sleep(512);
        Write("B", 1, ConsoleOutput);
    }
}
