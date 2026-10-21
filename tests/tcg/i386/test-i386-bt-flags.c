/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Check that BT, BTS, BTR and BTC leave OF, SF, ZF, AF and PF untouched.
 *
 * The SDM documents OF, SF, AF and PF as undefined after the bit test
 * instructions, but every implementation leaves them unchanged and
 * generated code depends on it, e.g. "dec; bt; lahf; jg".
 */

#include <stdio.h>
#include <stdlib.h>

#define CF 0x001
#define PF 0x004
#define AF 0x010
#define ZF 0x040
#define SF 0x080
#define OF 0x800
#define PRESERVED (PF | AF | ZF | SF | OF)

static int failed;

#define CHECK(name, before, after)                                          \
    do {                                                                    \
        if (((before) & PRESERVED) != ((after) & PRESERVED)) {              \
            printf("%s: flags 0x%lx -> 0x%lx (preserved bits differ)\n",      \
                   name, (unsigned long)(before), (unsigned long)(after));  \
            failed = 1;                                                     \
        }                                                                   \
    } while (0)

/*
 * ARITH is executed in the same basic block as the bit test, so that a
 * translator which tracks flags lazily sees the arithmetic result when
 * it translates the bit test.
 */
#define TEST(arith, bitop)                                                  \
    do {                                                                    \
        unsigned long before, after;                                        \
        unsigned int v;                                                     \
        v = value;                                                          \
        asm volatile(arith " %[v]\n\t"                                      \
                     "pushf\n\t"                                            \
                     "pop %[f]"                                             \
                     : [v] "+r" (v), [f] "=r" (before) : : "cc");           \
        v = value;                                                          \
        asm volatile(arith " %[v]\n\t"                                      \
                     bitop " $8, %[v]\n\t"                                  \
                     "pushf\n\t"                                            \
                     "pop %[f]"                                             \
                     : [v] "+r" (v), [f] "=r" (after) : : "cc");            \
        CHECK(arith " + " bitop " $8", before, after);                      \
        v = value;                                                          \
        asm volatile(arith " %[v]\n\t"                                      \
                     bitop " %[b], %[v]\n\t"                                \
                     "pushf\n\t"                                            \
                     "pop %[f]"                                             \
                     : [v] "+r" (v), [f] "=r" (after)                       \
                     : [b] "r" (bit) : "cc");                               \
        CHECK(arith " + " bitop " reg", before, after);                     \
    } while (0)

static void test_value(unsigned int value, unsigned int bit)
{
    TEST("decl", "btl");
    TEST("decl", "btsl");
    TEST("decl", "btrl");
    TEST("decl", "btcl");
    TEST("incl", "btl");
    TEST("negl", "btl");
    TEST("notl", "btl");    /* no flags change: previous op is a logic op */
    TEST("shll", "btl");
}

int main(void)
{
    static const unsigned int values[] = {
        0, 1, 2, 0x100, 0x7fffffff, 0x80000000, 0x80000001, 0xffffffff,
    };
    unsigned int i, bit;

    for (i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        for (bit = 0; bit < 32; bit += 7) {
            test_value(values[i], bit);
        }
    }
    return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
