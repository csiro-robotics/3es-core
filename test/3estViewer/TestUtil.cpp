//
// author: Kazys Stepanas
//

#include <3esview/util/ResourceList.h>

#include <gtest/gtest.h>

#include <chrono>
#include <iostream>
#include <list>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

namespace tes::viewer
{
struct Resource
{
  int value = 0;
};

using ResourceList = util::ResourceList<Resource>;

void buildResources(ResourceList &list, unsigned item_count)
{
  for (unsigned i = 0; i < item_count; ++i)
  {
    list.allocate()->value = i;
  }
}

TEST(Util, ResourceList_Allocate)
{
  ResourceList resources;
  buildResources(resources, 1000);

  for (size_t i = 0; i < resources.size(); ++i)
  {
    auto resource = resources[i];
    EXPECT_EQ(resource->value, i);
  }
}


TEST(Util, ResourceList_Release)
{
  ResourceList resources;
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
  ResourceList resources;
  buildResources(resources, 1000);

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


TEST(Util, ResourceList_Iteration)
{
  // To test interation, we'll allocate a number of resource, then free every second one. On iteration, we'll validate
  // we hit every second item.
  const unsigned target_resource_count = 10000u;
  ResourceList resources;
  buildResources(resources, target_resource_count);

  // Now free every second item. Just for fun. Make sure we release the first item though, so we can test begin()
  // skipping invalid items correctly.
  const unsigned stride = 2;
  for (util::ResourceListId id = 0; id < target_resource_count; id += stride)
  {
    resources.release(id);
  }

  // Now iterate and check.
  unsigned expected_value = 1;
  for (const auto &resource : const_cast<const ResourceList &>(resources))
  {
    EXPECT_EQ(resource.value, expected_value);
    expected_value += stride;
  }

  // And again, non-const this time.
  expected_value = 1;
  for (auto &resource : resources)
  {
    EXPECT_EQ(resource.value, expected_value);
    expected_value += stride;
  }
}


TEST(Util, ResourceList_Threads)
{
  struct SharedData
  {
    std::mutex mutex;
    std::atomic_int contended_count = { 0 };
    std::atomic_bool running = { false };
  };

  // Test expected thread behaviour for the resource list. Things to test:
  // 1. Recursive mutex: one thread can attain multiple resource locks.
  // 2. Multi-thread access: only one thread can have resources at a time.
  // 3. (maybe as it will leak) Release exception: releasing the resource list while another thread has locks throws.
  //
  // Note: This test is not bullet proof on the thread management. There is technically a race condition where the next
  // thread may fail to respond while the other sleeps.
  const auto sleep_duration = std::chrono::milliseconds(500);
  ResourceList resources;
  SharedData shared;

  // First resource will lock the list.
  auto ref1 = resources.allocate();
  ref1->value = unsigned(resources.size());
  // Second resource will lock again the list.
  auto ref2 = resources.allocate();
  ref2->value = unsigned(resources.size());

  // Release ref1 and start a thread. It should not be able to lock the resource list until after we unlock ref2.
  ref1.release();

  // Start with the mutex lock here so we can block the second thread.
  // We'll use this for partial synchronisation. Not great, but direct.
  std::unique_lock<std::mutex> lock(shared.mutex);

  const auto thread_func = [&resources, &shared, sleep_duration]() {
    shared.running = true;
    // Lock mutex here so we block and wait for control here.
    std::unique_lock<std::mutex> thread_lock(shared.mutex);

    // Try attain a resource.
    auto thread_resource = resources.allocate();
    ++shared.contended_count;
    // We should block here until ref2 is released.
    thread_resource->value = unsigned(resources.size());

    // Allow the other thread to have control.
    thread_lock.unlock();

    // Sleep a while to allow the other thread to try allocate; expect it can't
    std::this_thread::sleep_for(sleep_duration);
    EXPECT_EQ(shared.contended_count.load(), 1);

    // Unlock our resource to allow the other thread to allocate.
    thread_resource.release();
    std::this_thread::sleep_for(sleep_duration);
    thread_lock.lock();
    EXPECT_EQ(shared.contended_count.load(), 2);
  };

  // Start the thread. Will immediately block on the mutex.
  std::thread thread(thread_func);

  // Spin lock while we wait for the thread to start.
  while (!shared.running)
  {
    std::this_thread::yield();
  }

  // Thread is now running, but blocked. Allow it to continue.
  lock.unlock();

  // Sleep a while; we expect the other thread cannot attain any resources.
  std::this_thread::sleep_for(sleep_duration);
  EXPECT_EQ(shared.contended_count.load(), 0);
  // Release this thread's reference.
  ref2.release();
  // Wait for the other thread; it should be able to allocate now.
  std::this_thread::sleep_for(sleep_duration);
  EXPECT_EQ(shared.contended_count.load(), 1);

  // Now try allocate a new item here. We'll use the mutex to know we have control.
  lock.lock();
  ref1 = resources.allocate();
  ++shared.contended_count;
  ref1->value = unsigned(resources.size());
  // No more blocking.
  ref1.release();
  lock.unlock();

  ASSERT_NO_THROW(thread.join());

  EXPECT_EQ(resources.size(), 4);
  unsigned expected_value = 1;
  for (auto &&resource : resources)
  {
    EXPECT_EQ(resource.value, expected_value);
    ++expected_value;
  }
}  // namespace tes::viewer
}  // namespace tes::viewer
