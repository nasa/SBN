/************************************************************************
 * NASA Docket No. GSC-19,200-1, and identified as "cFS Draco"
 *
 * Copyright (c) 2023 United States Government as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ************************************************************************/

#include "sbn_coveragetest_common.h"
#include "sbn_module.h"
#include "sbn_module_bindings.h"

void Test_SBN_Module_FindBySymName(void)
{
    /* Test case for:
     * const SBN_ModuleBindingEntry_t *SBN_Module_FindBySymName(const char *SymName, const SBN_ModuleBindingEntry_t
     * *BindingList)
     */

    /* This is just for branch coverage */
    SBN_Module_FindBySymName("UT", NULL);
}

void Test_SBN_Module_GetProtocolIf(void)
{
    /* Test Case For:
     * SBN_Status_t SBN_Module_GetProtocolIf(const char *ModName, const SBN_IfOps_t **ProtoIf);
     */
    const SBN_IfOps_t *Intf;

    UT_SBN_MODULE_BINDINGS[0].Intf    = IfOpsPtr;
    UT_SBN_MODULE_BINDINGS[0].SymName = "UTS";
    UT_SBN_MODULE_BINDINGS[0].ModName = "UTM";

    UtAssert_INT32_EQ(SBN_Module_GetProtocolIf("UTS", &Intf), SBN_SUCCESS);
    UtAssert_ADDRESS_EQ(Intf, IfOpsPtr);

    UtAssert_INT32_EQ(SBN_Module_GetProtocolIf("BAD", &Intf), SBN_ERROR);
}

void Test_SBN_Module_GetFilterIf(void)
{
    /* Test Case For:
     * SBN_Status_t SBN_Module_GetFilterIf(const char *ModName, const SBN_FilterInterface_t **FilterIf);
     */
    const SBN_FilterInterface_t *Intf;

    UT_SBN_MODULE_BINDINGS[0].Intf    = FilterInterfacePtr;
    UT_SBN_MODULE_BINDINGS[0].SymName = "UTS";
    UT_SBN_MODULE_BINDINGS[0].ModName = "UTM";

    UtAssert_INT32_EQ(SBN_Module_GetFilterIf("UTS", &Intf), SBN_SUCCESS);
    UtAssert_ADDRESS_EQ(Intf, FilterInterfacePtr);

    UtAssert_INT32_EQ(SBN_Module_GetFilterIf("BAD", &Intf), SBN_ERROR);
}

void UT_Setup(void)
{
    memset(UT_SBN_MODULE_BINDINGS, 0, sizeof(UT_SBN_MODULE_BINDINGS));
} /* end UT_Setup() */

void UT_TearDown(void)
{
} /* end UT_TearDown() */

void UtTest_Setup(void)
{
    ADD_TEST(SBN_Module_FindBySymName);
    ADD_TEST(SBN_Module_GetProtocolIf);
    ADD_TEST(SBN_Module_GetFilterIf);
}
