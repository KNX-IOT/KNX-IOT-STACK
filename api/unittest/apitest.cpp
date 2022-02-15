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
  EXPECT_FALSE(oc_knx_contains_interface((oc_interface_mask_t)OC_IF_NONE, OC_IF_NONE));

  EXPECT_TRUE(oc_knx_contains_interface((oc_interface_mask_t)OC_IF_I, (oc_interface_mask_t)OC_IF_I));
  EXPECT_TRUE(oc_knx_contains_interface((oc_interface_mask_t)OC_IF_I, (oc_interface_mask_t)(OC_IF_I | OC_IF_O)));
  EXPECT_FALSE(oc_knx_contains_interface((oc_interface_mask_t)OC_IF_I, (oc_interface_mask_t)OC_IF_NONE));
  EXPECT_FALSE(oc_knx_contains_interface((oc_interface_mask_t)OC_IF_I, (oc_interface_mask_t)OC_IF_O));

  EXPECT_TRUE(oc_knx_contains_interface((oc_interface_mask_t)OC_IF_O, (oc_interface_mask_t)OC_IF_O));
  EXPECT_TRUE(oc_knx_contains_interface((oc_interface_mask_t)OC_IF_O, (oc_interface_mask_t)(OC_IF_O | OC_IF_G)));
  EXPECT_FALSE(oc_knx_contains_interface((oc_interface_mask_t)OC_IF_O,(oc_interface_mask_t) OC_IF_NONE));
  EXPECT_FALSE(oc_knx_contains_interface((oc_interface_mask_t)OC_IF_O, (oc_interface_mask_t)OC_IF_I));
  EXPECT_FALSE(oc_knx_contains_interface((oc_interface_mask_t)OC_IF_O,(oc_interface_mask_t) (OC_IF_I | OC_IF_G)));

  EXPECT_TRUE(oc_knx_contains_interface((oc_interface_mask_t)OC_IF_M, (oc_interface_mask_t)OC_IF_M));
  EXPECT_TRUE(oc_knx_contains_interface((oc_interface_mask_t)OC_IF_M, (oc_interface_mask_t)(OC_IF_M | OC_IF_G)));
  EXPECT_FALSE(oc_knx_contains_interface((oc_interface_mask_t)OC_IF_M, (oc_interface_mask_t)OC_IF_NONE));
  EXPECT_FALSE(oc_knx_contains_interface((oc_interface_mask_t)OC_IF_M, (oc_interface_mask_t)OC_IF_I));
  EXPECT_FALSE(oc_knx_contains_interface((oc_interface_mask_t)OC_IF_M,(oc_interface_mask_t)( OC_IF_I | OC_IF_G)));
}
