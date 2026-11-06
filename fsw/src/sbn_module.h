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

#ifndef SBN_MODULE_H
#define SBN_MODULE_H

#include "sbn_error.h"
#include "sbn_types.h"
#include "sbn_interfaces.h"

SBN_Status_t SBN_Module_GetProtocolIf(const char *ModName, const SBN_IfOps_t **ProtoIf);
SBN_Status_t SBN_Module_GetFilterIf(const char *ModName, const SBN_FilterInterface_t **FilterIf);

#endif /* SBN_MODULE_H */
