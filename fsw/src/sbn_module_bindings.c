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

/**
 * @file
 *
 * This source file only defines one symbol: SBN_MODULE_BINDINGS
 * The content of this depends on the configuration of SBN in CMake.
 * It has a pointer to the interface object for each SBN module.
 *
 * The "sbn_modules.inc" file is created by the CMake script.
 * It contains one line for each module in the form:
 *  SBN_MODULE(<mod-name>,<symbol-name>)
 *
 * note: it is not a normal C include file and it is not intended to be,
 * it is only included here to build this list and nothing else.
 */

#include <stddef.h>
#include "sbn_module_bindings.h"

/*
 * Note: SBN does not define a common base type for the protocol and filter interfaces.
 * Rather than having to duplicate this structure for each type of interface, we just
 * use a generic ref type.  The C linker does not know about types, it links purely on the name.
 */
#define SBN_MODULE(mod, sym) extern long sym;
#include "sbn_modules.inc"
#undef SBN_MODULE

#define SBN_MODULE(mod, sym) { #mod, #sym, &sym },
static const SBN_ModuleBindingEntry_t SBN_MODULE_BINDING_LIST[] = {
#include "sbn_modules.inc"
    { NULL, NULL, NULL }  /* end marker */
};
#undef SBN_MODULE

const SBN_ModuleBindingEntry_t *SBN_MODULE_BINDINGS = SBN_MODULE_BINDING_LIST;