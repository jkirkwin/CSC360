// an infinite loop to help test signal handling in kapish
#include <unistd.h>
int main() {
    while(1) {
        sleep(1);
    }
}