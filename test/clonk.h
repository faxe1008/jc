#ifndef CLONK_H__
#define CLONK_H__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif
/******** Type definitions ********/
typedef enum {
    RESULT_SUCCESS,
    RESULT_FAIL,
    RESULT_SKIPPED
} ClonkTestCaseResult;

typedef ClonkTestCaseResult (*TestCaseFunction)(const char*);

typedef enum {
    TYPE_TEST = 0,
    TYPE_BENCHMARK
} ClonkTestType;

typedef struct ClonkTestCaseEntry ClonkTestCaseEntry;
struct ClonkTestCaseEntry {
    ClonkTestType type;
    TestCaseFunction test_func;
    const char* test_name;
    ClonkTestCaseEntry* next;
};

typedef struct {
    ClonkTestCaseEntry* test_cases;
    int success_count;
    int fail_count;
    int skip_count;
} TestSuite;

/******** Global statics ********/
static TestSuite s_test_suite = { 0 };

/******** External Settings ********/
#ifndef CLONK_NO_COLOR
#    define ESCP_FAILED "\x1b[31m"
#    define ESCP_SUCCESS "\x1b[32m"
#    define ESCP_SKIPPED "\x1b[33m"
#    define ESCP_INFO "\x1b[34m"
#    define ESCP_EXIT "\x1b[0m"
#else
#    define ESCP_FAILED ""
#    define ESCP_SUCCESS ""
#    define ESCP_SKIPPED ""
#    define ESCP_INFO ""
#    define ESCP_EXIT ""
#endif

#ifndef CLONK_BENCHMARK_CYCLES
#    define CLONK_BENCHMARK_CYCLES 1000000
#endif

/******** Internal Settings & Helpers ********/
#define TEST_CASE_NAME(BASE_NAME) test_##BASE_NAME
#define BENCHMARK_NAME(BASE_NAME) benchmark_##BASE_NAME
#define SOURCE_CODE_POSITON_FORMATTER "(%s:%d)\n"

#define PRINT_TEST_RESULT(RESULT, MESSAGE, ...)                 \
    printf(RESULT "[%s]" MESSAGE SOURCE_CODE_POSITON_FORMATTER, \
        clonk_test_case_base_name, ##__VA_ARGS__, __FILE__, __LINE__)

/******** Helper functions ********/
void clonk_add_test(const char* name, TestCaseFunction func, ClonkTestType test_type)
{
    if (!s_test_suite.test_cases)
        s_test_suite.test_cases = calloc(1, sizeof(ClonkTestCaseEntry));
    ClonkTestCaseEntry* last_entry = s_test_suite.test_cases;
    for (; last_entry && last_entry->next; last_entry = last_entry->next) {
    }
    last_entry->test_name = name;
    last_entry->type = test_type;
    last_entry->test_func = func;
    last_entry->next = calloc(1, sizeof(ClonkTestCaseEntry));
}

void clonk_free_suite()
{
    ClonkTestCaseEntry* current = s_test_suite.test_cases;
    while (current) {
        ClonkTestCaseEntry* next = current->next;
        free(current);
        current = next;
    }
}

#define FOR_EACH_MATCHING_TEST(CONDITION, BODY)                       \
    for (ClonkTestCaseEntry* test_case = s_test_suite.test_cases;     \
         test_case && test_case->next; test_case = test_case->next) { \
        if (CONDITION) {                                              \
            BODY;                                                     \
        }                                                             \
    }

#define RUN_MATCHING_TESTS(CONDITION) \
    FOR_EACH_MATCHING_TEST(CONDITION, test_case->test_func(test_case->test_name))

void clonk_show_usage(char** argv)
{
    fprintf(stderr, "Usage: %s [-tblh] [TEST_NAME]...\n", argv[0]);
    fprintf(stderr, "clonk based test/benchmark executable.\n");
    fprintf(stderr, "Options: \n");
    fprintf(stderr, "  -b    Run benchmarks\n");
    fprintf(stderr, "  -t    Run tests\n");
    fprintf(stderr, "  -l    List available tests\n");
    fprintf(stderr, "  -h    Show this help message\n");
}

int clonk_run_test_suite(int argc, char** argv)
{
    int opt;
    int run_all_tests = 1;
    while ((opt = getopt(argc, argv, "tblh")) != -1) {
        run_all_tests = 0;
        switch (opt) {
        case 't':
            RUN_MATCHING_TESTS(test_case->type == TYPE_TEST);
            break;
        case 'b':
            RUN_MATCHING_TESTS(test_case->type == TYPE_BENCHMARK);
            break;
        case 'l':
            fprintf(stderr, "Tests: \n");
            FOR_EACH_MATCHING_TEST(test_case->type == TYPE_TEST, fprintf(stderr, "  %s\n", test_case->test_name));
            fprintf(stderr, "Benchmarks: \n");
            FOR_EACH_MATCHING_TEST(test_case->type == TYPE_BENCHMARK, fprintf(stderr, "  %s\n", test_case->test_name));
            return 0;
        case 'h':
            clonk_show_usage(argv);
            return 0;
        default:
            clonk_show_usage(argv);
            return 1;
        }
    }

    char** positionals = &argv[optind];
    if (run_all_tests && !*positionals) {
        RUN_MATCHING_TESTS(1);
    } else {
        for (; *positionals; positionals++) {
            RUN_MATCHING_TESTS(strcmp(test_case->test_name, *positionals) == 0);
        }
    }

    printf("Result: " ESCP_SUCCESS "%d Success" ESCP_EXIT ", " ESCP_FAILED
           "%d Failed" ESCP_EXIT ", " ESCP_SKIPPED "%d Skipped" ESCP_EXIT "\n",
        s_test_suite.success_count, s_test_suite.fail_count,
        s_test_suite.skip_count);
    clonk_free_suite();
    return s_test_suite.fail_count;
}

/******** Public Clonk Interface ********/
#define TEST_CASE(BASE_NAME, BODY)                               \
    ClonkTestCaseResult TEST_CASE_NAME(BASE_NAME)(               \
        const char* clonk_test_case_base_name)                   \
    {                                                            \
        BODY s_test_suite.success_count++;                       \
        PRINT_TEST_RESULT(ESCP_SUCCESS "SUCCESS" ESCP_EXIT, ""); \
        return RESULT_SUCCESS;                                   \
    }

#define BENCHMARK(BASE_NAME, BODY)                                                                                               \
    ClonkTestCaseResult BENCHMARK_NAME(BASE_NAME)(                                                                               \
        const char* clonk_test_case_base_name)                                                                                   \
    {                                                                                                                            \
        struct timeval tval_before, tval_after, tval_result;                                                                     \
        gettimeofday(&tval_before, NULL);                                                                                        \
        for (volatile int cbi = 0; cbi < CLONK_BENCHMARK_CYCLES; cbi++) {                                                        \
            BODY                                                                                                                 \
        }                                                                                                                        \
        gettimeofday(&tval_after, NULL);                                                                                         \
        timersub(&tval_after, &tval_before, &tval_result);                                                                       \
        uint64_t elapsed_ms = (tval_result.tv_sec * (uint64_t)1000) + (tval_result.tv_usec / 1000);                              \
        PRINT_TEST_RESULT(ESCP_INFO "BENCHMARK" ESCP_EXIT, ": took %lu ms (%d iterations)", elapsed_ms, CLONK_BENCHMARK_CYCLES); \
        return RESULT_SUCCESS;                                                                                                   \
    }

#define VERIFY(EXPRESSION)                                  \
    do {                                                    \
        if (!(EXPRESSION)) {                                \
            PRINT_TEST_RESULT(ESCP_FAILED "FAIL" ESCP_EXIT, \
                ": VERIFY " #EXPRESSION " ");               \
            s_test_suite.fail_count++;                      \
            return RESULT_FAIL;                             \
        }                                                   \
    } while (0)

#define VERIFY_NOT_REACHED()                                                 \
    PRINT_TEST_RESULT(ESCP_FAILED "FAIL" ESCP_EXIT, " VERIFY_NOT_REACHED "); \
    s_test_suite.fail_count++;                                               \
    return RESULT_FAIL;

#define SKIP()                                            \
    PRINT_TEST_RESULT(ESCP_SKIPPED "SKIP" ESCP_EXIT, ""); \
    s_test_suite.skip_count++;                            \
    return RESULT_SKIPPED

#define REGISTER_TEST_CASE(BASE_NAME) \
    clonk_add_test(#BASE_NAME, TEST_CASE_NAME(BASE_NAME), TYPE_TEST)

#define REGISTER_BENCHMARK(BASE_NAME) \
    clonk_add_test(#BASE_NAME, BENCHMARK_NAME(BASE_NAME), TYPE_BENCHMARK)

#define RUN_TEST_SUITE(ARGC, ARGV) return clonk_run_test_suite(ARGC, ARGV)

#ifdef __cplusplus
}
#endif

#endif