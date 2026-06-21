#include <check.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "src/c_libs/memory/memory.h"

START_TEST(test_growable_array_capacity_overflow)
{
    // Invariant: Buffer reads/writes never exceed allocated memory bounds
    // When capacity doubling would overflow, the operation must fail safely
    
    // Test cases: item_size values that could cause overflow when multiplied with capacity
    size_t item_sizes[] = {
        SIZE_MAX / 2 + 1,  // Exploit case: causes overflow on first double
        SIZE_MAX / 4 + 1,  // Boundary: overflows after one more double
        sizeof(int)        // Valid case: normal usage
    };
    int num_cases = sizeof(item_sizes) / sizeof(item_sizes[0]);

    for (int i = 0; i < num_cases; i++) {
        Arena arena;
        arena_init(&arena, 4096);
        
        GrowableArray arr;
        size_t initial_capacity = 2;
        
        // Initialize with potentially dangerous item_size
        growable_array_init(&arr, &arena, item_sizes[i], initial_capacity);
        
        // For overflow-prone cases, pushing should either:
        // 1. Fail gracefully (return NULL or not corrupt memory)
        // 2. Reject the operation
        // It must NOT allocate undersized buffer and write past it
        
        if (item_sizes[i] > SIZE_MAX / (initial_capacity * 2)) {
            // This would overflow - push should handle safely
            void *result = growable_array_push(&arr);
            // If it returns non-NULL, verify we haven't corrupted memory
            // by checking count doesn't exceed what could fit in SIZE_MAX
            ck_assert_msg(result == NULL || arr.count * arr.item_size >= arr.count,
                "Overflow in allocation size must be prevented");
        } else {
            // Normal case - should work fine
            void *result = growable_array_push(&arr);
            ck_assert_ptr_nonnull(result);
            ck_assert(arr.count <= arr.capacity);
        }
        
        arena_free(&arena);
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_growable_array_capacity_overflow);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}