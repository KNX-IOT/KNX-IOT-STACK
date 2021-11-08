/******************************************************************
 *
 * Copyright 2021 Cascoda Ltd All Rights Reserved.
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

#include <cstdlib>
#include <gtest/gtest.h>
#include <stdio.h>
#include <string>

#include "oc_api.h"
#include "oc_core_res.h"
#include "oc_helpers.h"

class TestLinkFormat : public testing::Test {
protected:
  virtual void SetUp()
  {
    oc_core_init();
    oc_random_init();
  }
  virtual void TearDown()
  {
    oc_core_shutdown();
    oc_random_destroy();
  }
};


TEST_F(TestLinkFormat, LF_)
{
  const char input[] = "<coap://[fe80::8d4c:632a:c5e7:ae09]:60054/p/a>;rt="urn:knx:dpa.352.51";if=if.a;ct=60, \
  <coap://[fe80::8d4c:632a:c5e7:ae09]:60054/p/b>;rt="urn:knx:dpa.352.52";if=if.s;ct=60, \
<coap://[fe80::8d4c:632a:c5e7:ae09]:60054/p/c>;rt="urn:knx:dpa.353.52";if=if.s;ct=60, \
<coap://[fe80::8d4c:632a:c5e7:ae09]:60054/dev>;rt="urn:knx:fb.0";ct=40,\
<coap://[fe80::8d4c:632a:c5e7:ae09]:60054/swu>;rt="urn:knx:fbswu";ct=40";

  int nr_entries = oc_lf_number_of_entries(payload, len);
  
  const char* uri;
  int uri_len;
  
  PRINT(" entries %d\n", nr_entries);
  EXPECT_EQ(5, nr_entries);
 
  int i = 0
  oc_lf_get_entry_uri(payload, len, i,
                       &uri, &uri_len);

  EXPECT_STREQ("coap://[fe80::8d4c:632a:c5e7:ae09]:60054/p/a", uri);


  //  oc_lf_get_entry_param(payload, len, i,
  //                        "rt", &param, &param_len);
  //  PRINT(" DISCOVERY RT %.*s\n", param_len, param);

  //  oc_lf_get_entry_param(payload, len, i, "if", &param, &param_len);
  //  PRINT(" DISCOVERY IF %.*s\n", param_len, param);

  //  oc_lf_get_entry_param(payload, len, i, "ct", &param, &param_len);
  //  PRINT(" DISCOVERY CT %.*s\n", param_len, param);


}
