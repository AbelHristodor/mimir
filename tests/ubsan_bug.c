/* Bug class: signed integer overflow (undefined behavior). UBSan (ubsan) catches this. */
#include <limits.h>

int main(int argc, char **argv) {
    (void)argv;
    volatile int x = INT_MAX; /* volatile so the compiler cannot fold it away */
    x = x + argc;             /* argc >= 1, so INT_MAX + argc overflows */
    return (int)x;
}
