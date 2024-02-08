#include "gtest/gtest.h"

#include <cstdlib>
#include <time.h>

#include "api/oc_replay.h"

TEST(ReplayProtection, OutOfOrderFrames)
{
  oc_string_t kid;
  oc_string_t kid_ctx;
  oc_new_byte_string(&kid, "thekid", strlen("thekid"));
  oc_new_byte_string(&kid_ctx, "thekidctx", strlen("thekidkctx"));

  oc_replay_add_client(6, kid, kid_ctx);

  // receive some valid frames, shifting the window
  EXPECT_TRUE(oc_replay_check_client(7, kid, kid_ctx, false));
  EXPECT_TRUE(oc_replay_check_client(8, kid, kid_ctx, false));

  // replay the frames
  EXPECT_FALSE(oc_replay_check_client(6, kid, kid_ctx, false));
  EXPECT_FALSE(oc_replay_check_client(7, kid, kid_ctx, false));
  EXPECT_FALSE(oc_replay_check_client(8, kid, kid_ctx, false));

  // receive some valid frames out of order
  EXPECT_TRUE(oc_replay_check_client(4, kid, kid_ctx, false));
  EXPECT_TRUE(oc_replay_check_client(2, kid, kid_ctx, false));
  EXPECT_TRUE(oc_replay_check_client(5, kid, kid_ctx, false));
  EXPECT_TRUE(oc_replay_check_client(3, kid, kid_ctx, false));

  // replay the frames some more
  EXPECT_FALSE(oc_replay_check_client(6, kid, kid_ctx, false));
  EXPECT_FALSE(oc_replay_check_client(7, kid, kid_ctx, false));
  EXPECT_FALSE(oc_replay_check_client(8, kid, kid_ctx, false));
  EXPECT_FALSE(oc_replay_check_client(2, kid, kid_ctx, false));
  EXPECT_FALSE(oc_replay_check_client(3, kid, kid_ctx, false));
  EXPECT_FALSE(oc_replay_check_client(4, kid, kid_ctx, false));

  // shift the window by a lot
  EXPECT_TRUE(oc_replay_check_client(20, kid, kid_ctx, false));

  // replays should still be detected
  EXPECT_FALSE(oc_replay_check_client(6, kid, kid_ctx, false));
  EXPECT_FALSE(oc_replay_check_client(7, kid, kid_ctx, false));
  EXPECT_FALSE(oc_replay_check_client(8, kid, kid_ctx, false));

  // some more valid out-of-order frames
  EXPECT_TRUE(oc_replay_check_client(17, kid, kid_ctx, false));
  EXPECT_TRUE(oc_replay_check_client(18, kid, kid_ctx, false));
  EXPECT_TRUE(oc_replay_check_client(19, kid, kid_ctx, false));
}

TEST(ReplayProtection, MultipleClients)
{
  oc_string_t empty = {};
  oc_string_t kid1, kid2, kid3, kid4, con1, con2;

  oc_new_byte_string(&kid1, "kid1", 4);
  oc_new_byte_string(&kid2, "kid2", 4);
  oc_new_byte_string(&kid3, "kid3", 4);
  oc_new_byte_string(&kid4, "kid4", 4);
  oc_new_byte_string(&con1, "con1", 4);
  oc_new_byte_string(&con2, "con2", 4);

  // null contexts are allowed, for compatibility
  // this should also test out different kids
  oc_replay_add_client(5, kid1, empty);
  oc_replay_add_client(5, kid2, empty);

  // discriminate using context ID
  oc_replay_add_client(5, kid1, con1);
  oc_replay_add_client(5, kid1, con2);

  // discriminate using kid
  oc_replay_add_client(5, kid3, con2);
  oc_replay_add_client(5, kid4, con2);

  // for every added client, test out a new valid packet & a replayed packet

  EXPECT_FALSE(oc_replay_check_client(5, kid1, empty, false));
  EXPECT_TRUE(oc_replay_check_client(6, kid1, empty, false));
  EXPECT_FALSE(oc_replay_check_client(5, kid2, empty, false));
  EXPECT_TRUE(oc_replay_check_client(6, kid2, empty, false));

  EXPECT_FALSE(oc_replay_check_client(5, kid1, con1, false));
  EXPECT_TRUE(oc_replay_check_client(6, kid1, con1, false));
  EXPECT_FALSE(oc_replay_check_client(5, kid1, con2, false));
  EXPECT_TRUE(oc_replay_check_client(6, kid1, con2, false));

  EXPECT_FALSE(oc_replay_check_client(5, kid3, con2, false));
  EXPECT_TRUE(oc_replay_check_client(6, kid3, con2, false));
  EXPECT_FALSE(oc_replay_check_client(5, kid4, con2, false));
  EXPECT_TRUE(oc_replay_check_client(6, kid4, con2, false));
}

TEST(ReplayProtection, TimeBasedFree)
{
  oc_string_t empty = {};
  oc_string_t kid;
  oc_new_byte_string(&kid, "abcd", 4);

  for (int i = 0; i < 40; ++i) {
    oc_replay_add_client(5, kid, empty);
    // increment the first character, so that we get 30 different values
    (*oc_string(kid))++;
    usleep(1);
  }

  // check 20 most recently added values - should still be readable
  for (int i = 20; i < 30; ++i) {
    (*oc_string(kid))--;
    EXPECT_TRUE(oc_replay_check_client(6, kid, empty, false));
  }
}

extern uint64_t g_oscore_replaywindow;

TEST(ReplayProtection, RplWdo)
{

  oc_string_t empty = {};
  oc_string_t kid;
  oc_new_byte_string(&kid, "abcd", 4);

  oc_replay_add_client(5, kid, empty);
  // outside the upper bound of the replay window
  EXPECT_FALSE(oc_replay_check_client(55, kid, empty, false));
  // fake an update to the replay window upper bound
  g_oscore_replaywindow = 64;
  EXPECT_TRUE(oc_replay_check_client(55, kid, empty, false));
}