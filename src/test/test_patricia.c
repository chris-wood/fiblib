#include "../patricia.h"

#include <stdio.h>

#include <LongBow/testing.h>
#include <LongBow/debugging.h>

#include <parc/algol/parc_Memory.h>
#include <parc/algol/parc_SafeMemory.h>

#include <parc/testing/parc_MemoryTesting.h>

LONGBOW_TEST_RUNNER(patricia)
{
    LONGBOW_RUN_TEST_FIXTURE(Core);
}

// The Test Runner calls this function once before any Test Fixtures are run.
LONGBOW_TEST_RUNNER_SETUP(patricia)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

// The Test Runner calls this function once after all the Test Fixtures are run.
LONGBOW_TEST_RUNNER_TEARDOWN(patricia)
{
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE(Core)
{
    LONGBOW_RUN_TEST_CASE(Core, patricia_Create);
    LONGBOW_RUN_TEST_CASE(Core, patricia_Insert_Empty);
    LONGBOW_RUN_TEST_CASE(Core, patricia_Insert_Equal);
    LONGBOW_RUN_TEST_CASE(Core, patricia_Insert_Sibling);
    LONGBOW_RUN_TEST_CASE(Core, patricia_Insert_Longer);
    LONGBOW_RUN_TEST_CASE(Core, patricia_Insert_Split);
    LONGBOW_RUN_TEST_CASE(Core, patricia_Insert_LongSplit);
}

LONGBOW_TEST_FIXTURE_SETUP(Core)
{
    parcMemory_SetInterface(&PARCSafeMemoryAsPARCMemory);
    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_FIXTURE_TEARDOWN(Core)
{
    if (!parcMemoryTesting_ExpectedOutstanding(0, "%s mismanaged memory.", longBowTestCase_GetFullName(testCase))) {
        parcSafeMemory_ReportAllocation(STDOUT_FILENO);
        return LONGBOW_STATUS_MEMORYLEAK;
    }

    return LONGBOW_STATUS_SUCCEEDED;
}

LONGBOW_TEST_CASE(Core, patricia_Create)
{
    Patricia *trie = patricia_Create(NULL);
    assertNotNull(trie, "Expected a non-NULL trie to be created");
    patricia_Destroy(&trie);
    assertNull(trie, "Expected a NULL trie after map_Destroy");
}

LONGBOW_TEST_CASE(Core, patricia_Insert_Empty)
{
    Patricia *trie = patricia_Create(NULL);
    assertNotNull(trie, "Expected a non-NULL trie to be created");

    PARCBuffer *key = parcBuffer_AllocateCString("abc");
    PARCBuffer *value = parcBuffer_AllocateCString("hello");
   
    patricia_Insert(trie, key, (void *) value);
    PARCBuffer *actual = (PARCBuffer *) patricia_Get(trie, key);

    assertNotNull(actual, "Expected to get something back");
    assertTrue(parcBuffer_Equals(actual, value), "Expected to get the same value back");
    
    parcBuffer_Release(&key);
    parcBuffer_Release(&value);
    patricia_Destroy(&trie);
}

LONGBOW_TEST_CASE(Core, patricia_Insert_LongSplit)
{
    Patricia *trie = patricia_Create(NULL);
    assertNotNull(trie, "Expected a non-NULL trie to be created");

    PARCBuffer *key1 = parcBuffer_AllocateCString("001");
    PARCBuffer *key2 = parcBuffer_AllocateCString("002");
    PARCBuffer *key3 = parcBuffer_AllocateCString("003");
    PARCBuffer *key4 = parcBuffer_AllocateCString("004");

    PARCBuffer *value1 = parcBuffer_AllocateCString("hello1");
    PARCBuffer *value2 = parcBuffer_AllocateCString("hello2");
    PARCBuffer *value3 = parcBuffer_AllocateCString("hello3");
    PARCBuffer *value4 = parcBuffer_AllocateCString("hello4");

    patricia_Insert(trie, key1, (void *) value1);
    patricia_Insert(trie, key2, (void *) value2);
//    patricia_Insert(trie, key3, (void *) value3);
//    patricia_Insert(trie, key4, (void *) value4);

    PARCBuffer *actual1 = (PARCBuffer *) patricia_Get(trie, key1);
    assertNotNull(actual1, "Expected to get something back");
    assertTrue(parcBuffer_Equals(actual1, value1), "Expected to get the same value back");

    PARCBuffer *actual2 = (PARCBuffer *) patricia_Get(trie, key2);
    assertNotNull(actual2, "Expected to get something back");
    assertTrue(parcBuffer_Equals(actual2, value2), "Expected to get the same value back");

//    PARCBuffer *actual3 = (PARCBuffer *) patricia_Get(trie, key3);
//    assertNotNull(actual3, "Expected to get something back");
//    assertTrue(parcBuffer_Equals(actual3, value3), "Expected to get the same value back");
//
//    PARCBuffer *actual4 = (PARCBuffer *) patricia_Get(trie, key4);
//    assertNotNull(actual4, "Expected to get something back");
//    assertTrue(parcBuffer_Equals(actual4, value4), "Expected to get the same value back");

    parcBuffer_Release(&key1);
    parcBuffer_Release(&value1);
    parcBuffer_Release(&key2);
    parcBuffer_Release(&value2);
    parcBuffer_Release(&key3);
    parcBuffer_Release(&value3);
    parcBuffer_Release(&key4);
    parcBuffer_Release(&value4);
    patricia_Destroy(&trie);
}

LONGBOW_TEST_CASE(Core, patricia_Insert_Equal)
{
    Patricia *trie = patricia_Create(NULL);
    assertNotNull(trie, "Expected a non-NULL trie to be created");

    PARCBuffer *key1 = parcBuffer_AllocateCString("/a/b/c/d");
    PARCBuffer *value1 = parcBuffer_AllocateCString("hello1");

    patricia_Insert(trie, key1, (void *) value1);
    PARCBuffer *actual = (PARCBuffer *) patricia_Get(trie, key1);

    assertNotNull(actual, "Expected to get something back");
    assertTrue(parcBuffer_Equals(actual, value1), "Expected to get the same value back");

    parcBuffer_Release(&key1);
    parcBuffer_Release(&value1);
    patricia_Destroy(&trie);
}

LONGBOW_TEST_CASE(Core, patricia_Insert_Sibling)
{
    Patricia *trie = patricia_Create(NULL);
    assertNotNull(trie, "Expected a non-NULL trie to be created");

    PARCBuffer *key1 = parcBuffer_AllocateCString("abc");
    PARCBuffer *value1 = parcBuffer_AllocateCString("hello1");
    PARCBuffer *key2 = parcBuffer_AllocateCString("def");
    PARCBuffer *value2 = parcBuffer_AllocateCString("value2");
   
    patricia_Insert(trie, key1, (void *) value1);
    PARCBuffer *actual = (PARCBuffer *) patricia_Get(trie, key1);

    assertNotNull(actual, "Expected to get something back");
    assertTrue(parcBuffer_Equals(actual, value1), "Expected to get the same value back");

    patricia_Insert(trie, key2, (void *) value2);
    actual = (PARCBuffer *) patricia_Get(trie, key2);

    assertNotNull(actual, "Expected to get something back");
    assertTrue(parcBuffer_Equals(actual, value2), "Expected to get the same value back");
    
    parcBuffer_Release(&key1);
    parcBuffer_Release(&value1);
    parcBuffer_Release(&key2);
    parcBuffer_Release(&value2);
    patricia_Destroy(&trie);
}

LONGBOW_TEST_CASE(Core, patricia_Insert_Longer)
{
    Patricia *trie = patricia_Create(NULL);
    assertNotNull(trie, "Expected a non-NULL trie to be created");

    PARCBuffer *key1 = parcBuffer_AllocateCString("abc");
    PARCBuffer *value1 = parcBuffer_AllocateCString("hello1");
    PARCBuffer *key2 = parcBuffer_AllocateCString("abcdef");
    PARCBuffer *value2 = parcBuffer_AllocateCString("value2");

    patricia_Insert(trie, key1, (void *) value1);
    PARCBuffer *actual = (PARCBuffer *) patricia_Get(trie, key1);

    assertNotNull(actual, "Expected to get something back");
    assertTrue(parcBuffer_Equals(actual, value1), "Expected to get the same value back");

    patricia_Insert(trie, key2, (void *) value2);
    actual = (PARCBuffer *) patricia_Get(trie, key2);

    assertNotNull(actual, "Expected to get something back");
    assertTrue(parcBuffer_Equals(actual, value2), "Expected to get the same value back");

    parcBuffer_Release(&key1);
    parcBuffer_Release(&value1);
    parcBuffer_Release(&key2);
    parcBuffer_Release(&value2);
    patricia_Destroy(&trie);
}

LONGBOW_TEST_CASE(Core, patricia_Insert_Split)
{
    Patricia *trie = patricia_Create(NULL);
    assertNotNull(trie, "Expected a non-NULL trie to be created");

    PARCBuffer *key1 = parcBuffer_AllocateCString("abc");
    PARCBuffer *value1 = parcBuffer_AllocateCString("hello1");
    PARCBuffer *key2 = parcBuffer_AllocateCString("abd");
    PARCBuffer *value2 = parcBuffer_AllocateCString("value2");
   
    patricia_Insert(trie, key1, (void *) value1);
    PARCBuffer *actual = (PARCBuffer *) patricia_Get(trie, key1);

    assertNotNull(actual, "Expected to get something back");
    assertTrue(parcBuffer_Equals(actual, value1), "Expected to get the same value before the split");

    patricia_Insert(trie, key2, (void *) value2);
    actual = (PARCBuffer *) patricia_Get(trie, key2);

    assertNotNull(actual, "Expected to get something back");
    assertTrue(parcBuffer_Equals(actual, value2), "Expected to get the same value back after the split");
    
    parcBuffer_Release(&key1);
    parcBuffer_Release(&value1);
    parcBuffer_Release(&key2);
    parcBuffer_Release(&value2);
    patricia_Destroy(&trie);
}

int
main(int argc, char *argv[argc])
{
    LongBowRunner *testRunner = LONGBOW_TEST_RUNNER_CREATE(patricia);
    int exitStatus = longBowMain(argc, argv, testRunner, NULL);
    longBowTestRunner_Destroy(&testRunner);
    exit(exitStatus);
}
