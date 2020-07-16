/*
** File: sbn_coveragetest_common.h
**
** Purpose:
** Common definitions for all sbn coverage tests
*/


#ifndef _sbn_coveragetest_common_h_
#define _sbn_coveragetest_common_h_

/*
 * Includes
 */

#include <utassert.h>
#include <uttest.h>
#include <utstubs.h>

#include <cfe.h>

#include "sbn_interfaces.h"

/*
 * Macro to call a function and check its int32 return code
 */
#define UT_TEST_FUNCTION_RC(func,exp)           \
{                                               \
    int32 rcexp = exp;                          \
    int32 rcact = func;                         \
    UtAssert_True(rcact == rcexp, "%s (%ld) == %s (%ld)",   \
        #func, (long)rcact, #exp, (long)rcexp);             \
}

/*
 * Macro to add a test case to the list of tests to execute
 */
#define ADD_TEST(test) UtTest_Add((Test_ ## test),UT_Setup,UT_TearDown, #test)

/*
 * Setup function prior to every test
 */
void UT_Setup(void);

/*
 * Teardown function after every test
 */
void UT_TearDown(void);

#endif /* _sbn_coveragetest_common_h_ */
