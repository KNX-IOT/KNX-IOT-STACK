/*
// Copyright (c) 2016 Intel Corporation
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

#include "port/oc_random.h"
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int urandom_fd;
static mbedtls_entropy_context entropy_ctx;
static mbedtls_ctr_drbg_context ctr_drbg_ctx;

void
oc_random_init(void)
{
  urandom_fd = open("/dev/urandom", O_RDONLY);

  mbedtls_entropy_init(&entropy_ctx);
  mbedtls_ctr_drbg_init(&ctr_drbg_ctx);
  mbedtls_ctr_drbg_seed(&ctr_drbg_ctx, mbedtls_entropy_func, &entropy_ctx, NULL,
                        0);
}

unsigned int
oc_random_value(void)
{
  unsigned int rand = 0;
  int ret = read(urandom_fd, &rand, sizeof(rand));
  assert(ret != -1);
#ifndef DEBUG
  (void)ret;
#endif
  return rand;
}

void
oc_random_destroy(void)
{
  close(urandom_fd);
  mbedtls_ctr_drbg_free(&ctr_drbg_ctx);
  mbedtls_entropy_free(&entropy_ctx);
}

mbedtls_ctr_drbg_context *
oc_random_get_ctr_drbg_context()
{
  return &ctr_drbg_ctx;
}
