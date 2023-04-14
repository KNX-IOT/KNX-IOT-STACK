/******************************************************************
 *
 * Copyright 2018 GRANITE RIVER LABS All Rights Reserved.
 *           2021 CASCODA LTD        All Rights Reserved.
 *
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 ******************************************************************/

#include <cstdlib>
#include <gtest/gtest.h>
#include <stdio.h>
#include <string>

#include "oc_api.h"
#include "oc_helpers.h"
#include "oc_ri.h"
#include "api/oc_knx_sec.h"
#include "port/linux/oc_config.h"

#define RESOURCE_URI "/LightResourceURI"
#define RESOURCE_NAME "roomlights"
#define OBSERVERPERIODSECONDS_P 1

class TestOcRi : public testing::Test {
protected:
  virtual void SetUp() { oc_ri_init(); }
  virtual void TearDown() { oc_ri_shutdown(); }
};

static void
onGet(oc_request_t *request, oc_interface_mask_t iface_mask, void *user_data)
{
  (void)request;
  (void)iface_mask;
  (void)user_data;
}

TEST_F(TestOcRi, GetAppResourceByUri_P)
{
  oc_resource_t *res;

  res = oc_new_resource(RESOURCE_NAME, RESOURCE_URI, 1, 0);
  oc_resource_set_discoverable(res, true);
  oc_resource_set_periodic_observable(res, OBSERVERPERIODSECONDS_P);
  oc_resource_set_request_handler(res, OC_GET, onGet, NULL);
  oc_ri_add_resource(res);

  res = oc_ri_get_app_resource_by_uri(RESOURCE_URI, strlen(RESOURCE_URI), 0);
  EXPECT_NE(res, nullptr);
  oc_ri_delete_resource(res);
}

TEST_F(TestOcRi, GetAppResourceByUri_N)
{
  oc_resource_t *res;

  res = oc_ri_get_app_resource_by_uri(RESOURCE_URI, strlen(RESOURCE_URI), 0);
  EXPECT_EQ(res, nullptr);
}

TEST_F(TestOcRi, RiGetAppResource_P)
{
  oc_resource_t *res;

  res = oc_new_resource(RESOURCE_NAME, RESOURCE_URI, 1, 0);
  oc_resource_set_discoverable(res, true);
  oc_resource_set_periodic_observable(res, OBSERVERPERIODSECONDS_P);
  oc_resource_set_request_handler(res, OC_GET, onGet, NULL);
  oc_ri_add_resource(res);
  res = oc_ri_get_app_resources();
  EXPECT_NE(nullptr, res);
  oc_ri_delete_resource(res);
}

TEST_F(TestOcRi, RiGetAppResource_N)
{
  oc_resource_t *res;

  res = oc_ri_get_app_resources();
  EXPECT_EQ(nullptr, res);
}

TEST_F(TestOcRi, RiAllocResource_P)
{
  oc_resource_t *res;

  res = oc_ri_alloc_resource();
  EXPECT_NE(nullptr, res);
  oc_ri_delete_resource(res);
}

TEST_F(TestOcRi, RiDeleteResource_P)
{
  oc_resource_t *res;
  bool del_check;

  res = oc_ri_alloc_resource();
  del_check = oc_ri_delete_resource(res);
  EXPECT_EQ(del_check, 1);
}

TEST_F(TestOcRi, RiFreeResourceProperties_P)
{
  oc_resource_t *res;

  res = oc_new_resource(RESOURCE_NAME, RESOURCE_URI, 1, 0);
  oc_ri_free_resource_properties(res);
  EXPECT_EQ(0, oc_string_len(res->name));
  oc_ri_delete_resource(res);
}

TEST_F(TestOcRi, RiAddResource_P)
{
  oc_resource_t *res;
  bool res_check;

  res = oc_new_resource(RESOURCE_NAME, RESOURCE_URI, 1, 0);
  oc_resource_set_discoverable(res, true);
  oc_resource_set_periodic_observable(res, OBSERVERPERIODSECONDS_P);
  oc_resource_set_request_handler(res, OC_GET, onGet, NULL);
  res_check = oc_ri_add_resource(res);
  EXPECT_EQ(res_check, 1);
  oc_ri_delete_resource(res);
}

TEST_F(TestOcRi, RIGetQueryValue_P)
{
  const char *input[] = { "key=1",        "data=1&key=2", "key=2&data=3",
                          "key=2&data=3", "key=2&data=3", "key=2",
                          "key=2&y" };
  int ret;
  char *value;

  for (int i = 0; i < 7; i++) {
    ret = oc_ri_get_query_value(input[i], strlen(input[i]), "key", &value);
    EXPECT_EQ(1, ret) << "P input[" << i << "] " << input[i] << " "
                      << "key";
  }
  for (int i = 0; i < 7; i++) {
    ret = oc_ri_get_query_value(input[i], strlen(input[i]), "key2", &value);
    EXPECT_EQ(-1, ret) << "N input[" << i << "] " << input[i] << " "
                       << "key2";
  }
}

TEST_F(TestOcRi, RIQueryExists_P)
{
  const char *input[] = { "key=1",
                          "key",
                          "data=1&key=2",
                          "data=2&key",
                          "key&data=3",
                          "key=2&data=3",
                          "x=1&key=2&data=3",
                          "y=&key=2&data=3",
                          "y=1&x&key=2&data=3",
                          "y=1&x&key" };
  int ret;
  for (int i = 0; i < 10; i++) {
    ret = oc_ri_query_exists(input[i], strlen(input[i]), "key");
    EXPECT_EQ(1, ret) << "P input[" << i << "] " << input[i] << " "
                      << "key";
  }
  for (int i = 0; i < 10; i++) {
    ret = oc_ri_query_exists(input[i], strlen(input[i]), "key2");
    EXPECT_EQ(-1, ret) << "N input[" << i << "] " << input[i] << " "
                       << "key2";
  }
}

TEST_F(TestOcRi, RIinterfacestring_P)
{
  const char *interface;

  interface = get_interface_string(OC_IF_I);
  EXPECT_STREQ("if.i", interface);

  interface = get_interface_string(OC_IF_O);
  EXPECT_STREQ("if.o", interface);

  // interface = get_interface_string(OC_IF_G);
  // EXPECT_EQ("if.i", interface);

  interface = get_interface_string(OC_IF_C);
  EXPECT_STREQ("if.c", interface);

  interface = get_interface_string(OC_IF_P);
  EXPECT_STREQ("if.p", interface);

  interface = get_interface_string(OC_IF_D);
  EXPECT_STREQ("if.d", interface);

  interface = get_interface_string(OC_IF_A);
  EXPECT_STREQ("if.a", interface);

  interface = get_interface_string(OC_IF_S);
  EXPECT_STREQ("if.s", interface);

  interface = get_interface_string(OC_IF_LI);
  EXPECT_STREQ("if.ll", interface);

  interface = get_interface_string(OC_IF_B);
  EXPECT_STREQ("if.b", interface);

  interface = get_interface_string(OC_IF_SEC);
  EXPECT_STREQ("if.sec", interface);

  interface = get_interface_string(OC_IF_SWU);
  EXPECT_STREQ("if.swu", interface);

  interface = get_interface_string(OC_IF_PM);
  EXPECT_STREQ("if.pm", interface);
}

TEST_F(TestOcRi, RIinterface_securitycheck)
{
  bool retval = false;
  retval = oc_knx_contains_interface(OC_IF_I, OC_IF_I);
  EXPECT_EQ(retval, true);

  retval = oc_knx_contains_interface(OC_IF_I, OC_IF_I | OC_IF_O);
  EXPECT_EQ(retval, true);

  retval = oc_knx_contains_interface(
    OC_IF_I, OC_IF_I | OC_IF_O | OC_IF_C | OC_IF_P | OC_IF_D | OC_IF_A |
               OC_IF_S | OC_IF_LI | OC_IF_B | OC_IF_SEC | OC_IF_SWU);
  EXPECT_EQ(retval, true);

  retval = oc_knx_contains_interface(
    OC_IF_SEC, OC_IF_I | OC_IF_O | OC_IF_C | OC_IF_P | OC_IF_D | OC_IF_A |
                 OC_IF_S | OC_IF_LI | OC_IF_B | OC_IF_SEC | OC_IF_SWU);
  EXPECT_EQ(retval, true);

  retval = oc_knx_contains_interface(OC_IF_SEC | OC_IF_I,
                                     OC_IF_I | OC_IF_O | OC_IF_C | OC_IF_P |
                                       OC_IF_D | OC_IF_A | OC_IF_S | OC_IF_LI |
                                       OC_IF_B | OC_IF_SEC | OC_IF_SWU);
  EXPECT_EQ(retval, true);

  retval = oc_knx_contains_interface(
    OC_IF_I | OC_IF_O | OC_IF_C | OC_IF_P | OC_IF_D | OC_IF_A | OC_IF_S |
      OC_IF_LI | OC_IF_B | OC_IF_SEC | OC_IF_SWU,
    OC_IF_I | OC_IF_O | OC_IF_C | OC_IF_P | OC_IF_D | OC_IF_A | OC_IF_S |
      OC_IF_LI | OC_IF_B | OC_IF_SEC | OC_IF_SWU);
  EXPECT_EQ(retval, true);

  retval = oc_knx_contains_interface(
    OC_IF_NONE, OC_IF_I | OC_IF_O | OC_IF_C | OC_IF_P | OC_IF_D | OC_IF_A |
                  OC_IF_S | OC_IF_LI | OC_IF_B | OC_IF_SEC | OC_IF_SWU);
  EXPECT_EQ(retval, false);

  retval = oc_knx_contains_interface(
    OC_IF_SEC, OC_IF_I | OC_IF_O | OC_IF_C | OC_IF_P | OC_IF_D | OC_IF_A |
                 OC_IF_S | OC_IF_LI | OC_IF_B | OC_IF_SWU);
  EXPECT_EQ(retval, false);

  retval = oc_knx_contains_interface(
    OC_IF_SWU | OC_IF_SEC, OC_IF_I | OC_IF_O | OC_IF_C | OC_IF_P | OC_IF_D |
                             OC_IF_A | OC_IF_S | OC_IF_LI | OC_IF_B);
  EXPECT_EQ(retval, false);
}