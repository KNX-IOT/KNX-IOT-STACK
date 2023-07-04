/*
// Copyright (c) 2023 Cascoda Ltd.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

#include "port/oc_spake.h"

int oc_spake_get_parameters(uint8_t rnd[32], uint8_t salt[32], int *it, mbedtls_mpi *w0, mbedtls_ecp_point *L)
{
    return 1; //not implemented
}

int oc_spake_set_parameters(uint8_t rnd[32], uint8_t salt[32], int it, mbedtls_mpi w0, mbedtls_ecp_point L)
{
    return 1; //not implemented
}
