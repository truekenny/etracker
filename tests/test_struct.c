#include <stdio.h>

struct test1 {
    unsigned long f1;
};
struct test2 {
    unsigned long f1;
    unsigned int f2;
    unsigned long f3;
};
struct test2_packed {
    unsigned long f1;
    unsigned int f2;
    unsigned long f3;
} __attribute__((packed));

int main() {
    printf("sizeof(struct test1)        = %lu\n", sizeof(struct test1));
//  sizeof(struct test1)        = 8
    printf("sizeof(struct test2)        = %lu\n", sizeof(struct test2));
//  sizeof(struct test2)        = 24
    printf("sizeof(struct test2_packed) = %lu\n", sizeof(struct test2_packed));
//  sizeof(struct test2_packed) = 20

    char * input = "1234567890abcdefghijklmn\0";
    struct test2 *test2 = (struct test2 *) input;
    struct test2_packed *test2_packed = (struct test2_packed *) input;

    printf("test2:        %lu %d %lu %s\n", test2->f1, test2->f2, test2->f3, &test2->f3);
//  test2:        4050765991979987505 1650536505 7957135325236127847 ghijklmn
    printf("test2_packed: %lu %d %lu %s\n", test2_packed->f1, test2_packed->f2, test2_packed->f3, &test2_packed->f3);
//  test2_packed: 4050765991979987505 1650536505 7667774633883821155 cdefghijklmn
}
