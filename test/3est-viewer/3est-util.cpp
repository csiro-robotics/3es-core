//
// author: Kazys Stepanas
//

#include <3es-viewer/util/3esresourcelist.h>

#include <gtest/gtest.h>

#include <list>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

namespace tes::viewer
{
struct Resource
{
  int value = 0;
};

TEST(Util, ResourceList_Allocate)
{
  util::ResourceList<Resource> resources;
  std::vector<util::ResourceListId> ids;
  for (int i = 0; i < 1000; ++i)
  {
    auto resource = resources.allocate();
    (*resource).value = i;
    ids.emplace_back(resource.id());
  }

  for (size_t i = 0; i < ids.size(); ++i)
  {
    auto resource = resources[ids[i]];
    EXPECT_EQ(resource->value, i);
  }
}


TEST(Util, ResourceList_Release)
{
  util::ResourceList<Resource> resources;
  // Make stochastic allocations and releases.
  std::mt19937 rand_eng(0x01020304);
  std::uniform_int_distribution<> rand(1, 6);
  std::list<util::ResourceListId> ids;
  std::vector<bool> expect_valid;
  size_t allocated = 0;
  size_t released = 0;

  bool allocate = true;
  for (int i = 0; i < 1000; ++i)
  {
    int action_count = rand(rand_eng);
    if (allocate)
    {
      for (int j = 0; j < action_count; ++j)
      {
        auto res = resources.allocate();
        ids.emplace_back(res.id());
        while (expect_valid.size() < res.id() + 1)
        {
          expect_valid.emplace_back(false);
        }
        EXPECT_FALSE(expect_valid[res.id()]);
        expect_valid[res.id()] = true;
        ++allocated;
      }
    }
    else
    {
      // Bias allocation over release.
      action_count = std::max(1, action_count / 2);
      for (int j = 0; j < action_count && !ids.empty(); ++j)
      {
        const auto id = ids.front();
        ids.pop_front();
        resources.release(id);
        EXPECT_FALSE(resources.at(id).isValid());
        expect_valid[id] = false;
        ++released;
      }
    }
    allocate = !allocate;
  }

  // Ensure what's left is valid.
  size_t allocated_final = 0;
  for (size_t i = 0; i < expect_valid.size(); ++i)
  {
    EXPECT_EQ(resources.at(i).isValid(), expect_valid[i]);
    allocated_final += expect_valid[i] == true;
  }

  EXPECT_GE(allocated, released);
  EXPECT_EQ(allocated_final, allocated - released);
}


TEST(Util, ResourceList_OutOfRange)
{
  util::ResourceList<Resource> resources;
  for (int i = 0; i < 1000; ++i)
  {
    resources.allocate()->value = i;
  }

  // Fetch a valid item.
  const size_t at = 42;
  auto res1 = resources.at(at);
  EXPECT_TRUE(res1.isValid());
  EXPECT_EQ(res1->value, at);
  res1 = resources[0];
  EXPECT_EQ(res1->value, 0);

  // Fetch an out of range item.
  res1 = resources.at(resources.size());
  EXPECT_FALSE(res1.isValid());
}
}  // namespace tes::viewer
