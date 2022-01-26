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
#include "port/oc_random.h"

TEST(KNXLSM, LSMConstToStr)
{
  const char *mystring;

  mystring = oc_core_get_lsm_as_string(LSM_UNLOADED);
  EXPECT_STREQ("unloaded", mystring);

  mystring = oc_core_get_lsm_as_string(LSM_LOADED);
  EXPECT_STREQ("loaded", mystring);

  mystring = oc_core_get_lsm_as_string(LSM_LOADCOMPLETE);
  EXPECT_STREQ("loadComplete", mystring);

  mystring = oc_core_get_lsm_as_string(LSM_STARTLOADING);
  EXPECT_STREQ("startLoading", mystring);

  mystring = oc_core_get_lsm_as_string(LSM_LOADING);
  EXPECT_STREQ("loading", mystring);

  mystring = oc_core_get_lsm_as_string(LSM_UNLOAD);
  EXPECT_STREQ("unload", mystring);
}

TEST(KNXLSM, LSMStrValid)
{
  bool retbool;
  retbool = oc_core_lsm_check_string("unloaded");
  EXPECT_EQ(retbool, true);

  retbool = oc_core_lsm_check_string("loading");
  EXPECT_EQ(retbool, true);

  retbool = oc_core_lsm_check_string("loaded");
  EXPECT_EQ(retbool, true);

  retbool = oc_core_lsm_check_string("unload");
  EXPECT_EQ(retbool, true);

  retbool = oc_core_lsm_check_string("startLoading");
  EXPECT_EQ(retbool, true);

  retbool = oc_core_lsm_check_string("loadComplete");
  EXPECT_EQ(retbool, true);

  // errors
  retbool = oc_core_lsm_check_string("loadcomplete");
  EXPECT_EQ(retbool, false);

  retbool = oc_core_lsm_check_string("load");
  EXPECT_EQ(retbool, false);

  retbool = oc_core_lsm_check_string("loadedxx");
  EXPECT_EQ(retbool, false);
}

TEST(KNXLSM, LSMStrToConst)
{
  oc_lsm_state_t mystate;
  mystate = oc_core_lsm_parse_string("unloaded");
  EXPECT_EQ(mystate, LSM_UNLOADED);

  mystate = oc_core_lsm_parse_string("loading");
  EXPECT_EQ(mystate, LSM_LOADING);

  mystate = oc_core_lsm_parse_string("loaded");
  EXPECT_EQ(mystate, LSM_LOADED);

  mystate = oc_core_lsm_parse_string("unload");
  EXPECT_EQ(mystate, LSM_UNLOAD);

  mystate = oc_core_lsm_parse_string("startLoading");
  EXPECT_EQ(mystate, LSM_STARTLOADING);

  mystate = oc_core_lsm_parse_string("loadComplete");
  EXPECT_EQ(mystate, LSM_LOADCOMPLETE);

  // errors
  mystate = oc_core_lsm_parse_string("load");
  EXPECT_EQ(mystate, LSM_UNLOADED);

  mystate = oc_core_lsm_parse_string("loadedxx");
  EXPECT_EQ(mystate, LSM_UNLOADED);
}
