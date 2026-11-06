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

/*
 ** Include Files
 */
#include "sbn_error.h"
#include "sbn_module.h"
#include "sbn_module_bindings.h"

const SBN_ModuleBindingEntry_t *SBN_ModuleBinding_FindByModName(const char                     *ModName,
                                                                const SBN_ModuleBindingEntry_t *BindingList)
{
    const SBN_ModuleBindingEntry_t *Entry;

    Entry = BindingList;
    while (Entry != NULL)
    {
        if (Entry->ModName == NULL)
        {
            /* end of list */
            Entry = NULL;
            break;
        }
        if (strcmp(Entry->ModName, ModName) == 0)
        {
            /* found a match */
            break;
        }
        ++Entry;
    }
    return Entry;
}

const SBN_ModuleBindingEntry_t *SBN_ModuleBinding_FindBySymName(const char                     *SymName,
                                                                const SBN_ModuleBindingEntry_t *BindingList)
{
    const SBN_ModuleBindingEntry_t *Entry;

    Entry = BindingList;
    while (Entry != NULL)
    {
        if (Entry->SymName == NULL)
        {
            /* end of list */
            Entry = NULL;
            break;
        }
        if (strcmp(Entry->SymName, SymName) == 0)
        {
            /* found a match */
            break;
        }
        ++Entry;
    }
    return Entry;
}

SBN_Status_t SBN_Module_GetGenericIntf(const char *SymName, const void **OutPtr)
{
    const SBN_ModuleBindingEntry_t *Entry;
    SBN_Status_t                    Status;

    Entry = SBN_ModuleBinding_FindBySymName(SymName, SBN_MODULE_BINDINGS);
    if (Entry == NULL)
    {
        Status  = SBN_ERROR;
        *OutPtr = NULL;
    }
    else
    {
        Status  = SBN_SUCCESS;
        *OutPtr = Entry->Intf;
    }

    return Status;
}

SBN_Status_t SBN_Module_GetProtocolIf(const char *SymName, const SBN_IfOps_t **ProtoIf)
{
    return SBN_Module_GetGenericIntf(SymName, (const void **)ProtoIf);
}

SBN_Status_t SBN_Module_GetFilterIf(const char *SymName, const SBN_FilterInterface_t **FilterIf)
{
    return SBN_Module_GetGenericIntf(SymName, (const void **)FilterIf);
}
