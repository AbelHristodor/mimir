/* Bug class: heap-buffer-overflow. AddressSanitizer (asan) catches this. */
#include <stdlib.h>

int main(int argc, char **argv) {
    (void)argv;
    /* volatile + a runtime value (argc) so the optimizer cannot fold the
       accesses away; the out-of-bounds store/load must actually be emitted. */
    volatile int *a = malloc(4 * sizeof(int)); /* valid indices are 0..3 */
    a[4] = argc;                               /* one past the end: OOB write */
    return (int)a[4];                          /* OOB read */
}
