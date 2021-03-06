//
// Created by caw on 11/29/16.
//

#include "../fib_merged_filter.h"

#include <LongBow/testing.h>

#include <parc/algol/parc_Memory.h>
#include <parc/algol/parc_SafeMemory.h>

#include <parc/testing/parc_MemoryTesting.h>

#include "test_fib.c"

LONGBOW_TEST_RUNNER(merged_bloom)
{
    LONGBOW_RUN_TEST_FIXTURE(Core);
}

// The Test Runner calls this function once before any Test Fixtures are run.
LONGBOW_TEST_RUNNER_SETUP(merged_bloom)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

// The Test Runner calls this function once after all the Test Fixtures are run.
LONGBOW_TEST_RUNNER_TEARDOWN(merged_bloom)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE(Core)
{
    LONGBOW_RUN_TEST_CASE(Core, fibMergedBloom_LookupSimple);
    LONGBOW_RUN_TEST_CASE(Core, fibMergedBloom_LookupHashed);
}

LONGBOW_TEST_FIXTURE_SETUP(Core)
{
    parcMemory_SetInterface(&PARCSafeMemoryAsPARCMemory);
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE_TEARDOWN(Core)
{
    if (!parcMemoryTesting_ExpectedOutstanding(0, "%s mismanaged memory.", longBowTestCase_GetFullName(testCase))) {
        return LONGBOW_STATUS_MEMORYLEAK;
    }
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_CASE(Core, fibMergedBloom_LookupSimple)
{
    FIBMergedFilter *filter = fibMergedFilter_Create(128, 128, 3);
    assertNotNull(filter, "Expected a non-NULL FIBCisco to be created");

    FIB *fib = fib_Create(filter, MergedFilterFIBAsFIB);
    assertNotNull(fib, "Expected non-NULL FIB");

    test_fib_lookup(fib);

    fib_Destroy(&fib);
}

LONGBOW_TEST_CASE(Core, fibMergedBloom_LookupHashed)
{
    FIBMergedFilter *filter = fibMergedFilter_Create(128, 128, 3);
    assertNotNull(filter, "Expected a non-NULL FIBCisco to be created");

    FIB *fib = fib_Create(filter, MergedFilterFIBAsFIB);
    assertNotNull(fib, "Expected non-NULL FIB");

    test_fib_hash_lookup(fib);

    fib_Destroy(&fib);
}

int
main(int argc, char *argv[argc])
{
    LongBowRunner *testRunner = LONGBOW_TEST_RUNNER_CREATE(merged_bloom);
    int exitStatus = longBowMain(argc, argv, testRunner, NULL);
    longBowTestRunner_Destroy(&testRunner);
    exit(exitStatus);
}
