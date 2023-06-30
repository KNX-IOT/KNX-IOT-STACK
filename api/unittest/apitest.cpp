/******************************************************************
 *
 * Copyright 2021 Cascoda All Rights Reserved.
 *
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

#include "gtest/gtest.h"
#include <cstdlib>

#include "oc_knx.h"
#include "api/oc_knx_sec.h"
#include "port/oc_random.h"

TEST(KNXLSM, LSMConstToStr)
{
  const char *mystring;

  mystring = oc_core_get_lsm_state_as_string(LSM_S_UNLOADED);
  EXPECT_STREQ("unloaded", mystring);

  mystring = oc_core_get_lsm_state_as_string(LSM_S_LOADED);
  EXPECT_STREQ("loaded", mystring);

  mystring = oc_core_get_lsm_state_as_string(LSM_S_LOADING);
  EXPECT_STREQ("loading", mystring);

  // mystring = oc_core_get_lsm_state_as_string(LSM_S_LOADCOMPLETE);
  // EXPECT_STREQ("loadComplete", mystring);

  mystring = oc_core_get_lsm_event_as_string(LSM_E_STARTLOADING);
  EXPECT_STREQ("startLoading", mystring);

  mystring = oc_core_get_lsm_event_as_string(LSM_E_LOADCOMPLETE);
  EXPECT_STREQ("loadComplete", mystring);

  mystring = oc_core_get_lsm_event_as_string(LSM_E_UNLOAD);
  EXPECT_STREQ("unload", mystring);
}

TEST(KNXSEC, contains_interfaces)
{
  EXPECT_FALSE(
    oc_knx_contains_interface((oc_interface_mask_t)OC_IF_NONE, OC_IF_NONE));

  EXPECT_TRUE(oc_knx_contains_interface((oc_interface_mask_t)OC_IF_I,
                                        (oc_interface_mask_t)OC_IF_I));
  EXPECT_TRUE(oc_knx_contains_interface(
    (oc_interface_mask_t)OC_IF_I, (oc_interface_mask_t)(OC_IF_I | OC_IF_O)));
  EXPECT_FALSE(oc_knx_contains_interface((oc_interface_mask_t)OC_IF_I,
                                         (oc_interface_mask_t)OC_IF_NONE));
  EXPECT_FALSE(oc_knx_contains_interface((oc_interface_mask_t)OC_IF_I,
                                         (oc_interface_mask_t)OC_IF_O));

  EXPECT_TRUE(oc_knx_contains_interface((oc_interface_mask_t)OC_IF_O,
                                        (oc_interface_mask_t)OC_IF_O));
  EXPECT_TRUE(oc_knx_contains_interface(
    (oc_interface_mask_t)OC_IF_O, (oc_interface_mask_t)(OC_IF_O | OC_IF_G)));
  EXPECT_FALSE(oc_knx_contains_interface((oc_interface_mask_t)OC_IF_O,
                                         (oc_interface_mask_t)OC_IF_NONE));
  EXPECT_FALSE(oc_knx_contains_interface((oc_interface_mask_t)OC_IF_O,
                                         (oc_interface_mask_t)OC_IF_I));
  EXPECT_FALSE(oc_knx_contains_interface(
    (oc_interface_mask_t)OC_IF_O, (oc_interface_mask_t)(OC_IF_I | OC_IF_G)));

  EXPECT_TRUE(oc_knx_contains_interface((oc_interface_mask_t)OC_IF_M,
                                        (oc_interface_mask_t)OC_IF_M));
  EXPECT_TRUE(oc_knx_contains_interface(
    (oc_interface_mask_t)OC_IF_M, (oc_interface_mask_t)(OC_IF_M | OC_IF_G)));
  EXPECT_FALSE(oc_knx_contains_interface((oc_interface_mask_t)OC_IF_M,
                                         (oc_interface_mask_t)OC_IF_NONE));
  EXPECT_FALSE(oc_knx_contains_interface((oc_interface_mask_t)OC_IF_M,
                                         (oc_interface_mask_t)OC_IF_I));
  EXPECT_FALSE(oc_knx_contains_interface(
    (oc_interface_mask_t)OC_IF_M, (oc_interface_mask_t)(OC_IF_I | OC_IF_G)));
}

TEST(HELPER_cmp, oc_string_cmp)
{
  oc_string_t compare1;
  oc_new_string(&compare1, "abcd", 4);

  oc_string_t compare2;
  oc_new_string(&compare2, "abdcd", 4);

  int value = oc_string_cmp(compare1, compare1);
  EXPECT_EQ(0, value);
  value = oc_string_cmp(compare1, compare2);
  EXPECT_NE(0, value);

  oc_free_string(&compare1);
  oc_free_string(&compare2);
}

TEST(HELPER_cmp, oc_url_cmp)
{
  oc_string_t compare1;
  oc_new_string(&compare1, "/abcd", 5);

  oc_string_t compare2;
  oc_new_string(&compare2, "abcd", 4);

  oc_string_t compare3;
  oc_new_string(&compare3, "abddc", 4);

  int value = oc_url_cmp(compare1, compare1);
  EXPECT_EQ(0, value);
  value = oc_url_cmp(compare1, compare2);
  EXPECT_EQ(0, value);
  value = oc_url_cmp(compare1, compare3);
  EXPECT_NE(0, value);
  value = oc_url_cmp(compare2, compare3);
  EXPECT_NE(0, value);

  oc_free_string(&compare1);
  oc_free_string(&compare2);
  oc_free_string(&compare3);
}

TEST(HELPER_uint64, oc_conv_uint64_hex)
{
  struct test_vector_entry
  {
    char expected_val[17];
    uint64_t test_val;
  };

  struct test_vector_entry hex_test_vector[] = {
    { "0", 0 },           { "1", 1 },          { "1", 0x01 },
    { "abc", 0xabc },     { "abc", 0x0abc },   { "ab0c", 0xab0c },
    { "ab00c", 0xab00c }, { "d0fc2", 856002 }, { "ab000cd123", 0xAB000CD123 }
  };

  int num_of_tests_hex =
    sizeof(hex_test_vector) / sizeof(struct test_vector_entry);

  for (int i = 0; i < num_of_tests_hex; ++i) {
    char str[17];
    oc_conv_uint64_to_hex_string(str, hex_test_vector[i].test_val);
    EXPECT_STREQ(hex_test_vector[i].expected_val, str);
  }
}

TEST(HELPER_uint64, oc_conv_uint64_dec)
{
  struct test_vector_entry
  {
    char expected_val[22];
    uint64_t test_val;
  };

  struct test_vector_entry decimal_test_vector[] = {
    { "0", 0 }, { "1", 1 }, { "8710", 8710 }, { "255", 0xff }
  };

  int num_of_tests_dec =
    sizeof(decimal_test_vector) / sizeof(struct test_vector_entry);

  for (int i = 0; i < num_of_tests_dec; ++i) {
    char str[22];
    oc_conv_uint64_to_dec_string(str, decimal_test_vector[i].test_val);
    EXPECT_STREQ(decimal_test_vector[i].expected_val, str);
  }
}