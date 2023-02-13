//
// author: Kazys Stepanas
//
#include "TestCommon.h"

#include <3escore/IntArg.h>
#include <3escore/Ptr.h>
#include <3escore/V3Arg.h>
#include <3escore/shapes/SimpleMesh.h>

#include <algorithm>
#include <cinttypes>
#include <iterator>

#include <gtest/gtest.h>

namespace tes
{
template <typename DSTINT, typename SRCINT>
void TestIntArg(SRCINT value)
{
  EXPECT_EQ(IntArgT<DSTINT>(value).i, value);
}

TEST(Core, IntArg)
{
  TestIntArg<int, int>(42);
  TestIntArg<int, unsigned>(42);
  TestIntArg<int, size_t>(42);
  TestIntArg<int, int32_t>(42);
  TestIntArg<int, uint32_t>(42);

  TestIntArg<unsigned, int>(42);
  TestIntArg<unsigned, unsigned>(42);
  TestIntArg<unsigned, size_t>(42);
  TestIntArg<unsigned, int32_t>(42);
  TestIntArg<unsigned, uint32_t>(42);

  TestIntArg<size_t, int>(42);
  TestIntArg<size_t, unsigned>(42);
  TestIntArg<size_t, size_t>(42);
  TestIntArg<size_t, int32_t>(42);
  TestIntArg<size_t, uint32_t>(42);

  TestIntArg<int32_t, int>(42);
  TestIntArg<int32_t, unsigned>(42);
  TestIntArg<int32_t, size_t>(42);
  TestIntArg<int32_t, int32_t>(42);
  TestIntArg<int32_t, uint32_t>(42);

  TestIntArg<uint32_t, int>(42);
  TestIntArg<uint32_t, unsigned>(42);
  TestIntArg<uint32_t, size_t>(42);
  TestIntArg<uint32_t, int32_t>(42);
  TestIntArg<uint32_t, uint32_t>(42);
}

template <typename SRCTYPE>
void TestV3Arg(SRCTYPE value, const Vector3f &expect)
{
  EXPECT_EQ(V3Arg(value).v3, expect);
}

TEST(Core, V3Arg)
{
  const float vf3[3] = { 1.1f, 2.2f, 3.3f };
  const double vd3[3] = { 1.1, 2.2, 3.3 };
  const Vector3f vf{ 1.1f, 2.2f, 3.3f };
  const Vector3d vd{ 1.1, 2.2, 3.3 };

  TestV3Arg(vf3, vf);
  TestV3Arg(vd3, vf);
  TestV3Arg(vf, vf);
  TestV3Arg(vd, vf);
}

TEST(Core, PtrManagement)
{
  struct Datum
  {
    unsigned value = 0;
    unsigned &data_count;

    Datum(unsigned value, unsigned &data_count)
      : value(value)
      , data_count(data_count)
    {
      ++data_count;
    }
    ~Datum() { --data_count; }
  };

  // This variable tracks how many Datum objects we have allocated.
  unsigned item_count = 0;

  const unsigned pointer_count = 2048u;
  // Build a set of pointers to reference with Ptr.
  std::vector<std::shared_ptr<Datum>> src_pointers;
  for (unsigned i = 0; i < pointer_count; ++i)
  {
    src_pointers.emplace_back(std::make_shared<Datum>(i, item_count));
  }

  EXPECT_EQ(item_count, src_pointers.size());

  // Make a predicate which determines whether to add a shared or borrowed pointer from src_pointers
  // to ptr_set.
  const auto add_shared = [](unsigned value) { return (value & 1u) == 0; };
  std::vector<Ptr<Datum>> ptr_set;
  for (const auto &ptr : src_pointers)
  {
    if (add_shared(ptr->value))
    {
      // Add shared pointer
      ptr_set.emplace_back(Ptr<Datum>(ptr));
    }
    else
    {
      // Add borrowed pointer
      ptr_set.emplace_back(Ptr<Datum>(ptr.get()));
    }
  }

  EXPECT_EQ(ptr_set.size(), src_pointers.size());
  EXPECT_EQ(item_count, src_pointers.size());

  // Validate our pointers
  for (unsigned i = 0; i < static_cast<unsigned>(ptr_set.size()); ++i)
  {
    const auto &ptr = ptr_set[i];
    // Validated we have the correct pointer type and shared pointer reference count is correct.
    if (add_shared(i))
    {
      EXPECT_EQ(ptr.status(), Ptr<Datum>::Status::Shared);
      EXPECT_EQ(src_pointers[i].use_count(), 2);
    }
    else
    {
      EXPECT_EQ(ptr.status(), Ptr<Datum>::Status::Borrowed);
      EXPECT_EQ(src_pointers[i].use_count(), 1);
    }

    // Validate the value.
    EXPECT_EQ(ptr->value, i);  // Use -> operator to test. * used later.
  }

  // Release the src_pointers first. The borrowed pointers will become invalid, but the shared
  // pointer should stay valid.
  src_pointers.clear();

  EXPECT_EQ(item_count, ptr_set.size() / 2);

  for (unsigned i = 0; i < static_cast<unsigned>(ptr_set.size()); ++i)
  {
    const auto &ptr = ptr_set[i];
    // Validated we have the correct pointer type and shared pointer reference count is correct.
    if (add_shared(i))
    {
      ASSERT_EQ(ptr.status(), Ptr<Datum>::Status::Shared);
      ASSERT_TRUE(ptr.shared());
      const auto ptr_shared = ptr.shared();
      ASSERT_GT(ptr_shared.use_count(), 0);
      // We have a local reference and one in ptr_set, so expect use count of 2.
      EXPECT_EQ(ptr_shared.use_count(), 2);
      EXPECT_EQ(ptr_shared->value, i);
      EXPECT_EQ((*ptr).value, i);  // Use * operator to test. -> used above.
    }
  }

  // Release everything.
  EXPECT_NO_THROW(ptr_set.clear());
  EXPECT_EQ(item_count, 0u);
}

// To test implicit argument conversion
template <typename T, typename U>
inline void testImplicitArgConvert(const Ptr<T> &ptr, const std::shared_ptr<U> &src)
{
  EXPECT_EQ(ptr.get(), src.get());
}

template <typename T, typename U>
inline void testPtrCast(const std::shared_ptr<U> &src)
{
  size_t use_count = src.use_count();

  // Assign to Ptr
  auto ptr_shared = Ptr<T>(src);          // shared
  auto ptr_borrowed = Ptr<T>(src.get());  // borrowed.
  ++use_count;

  EXPECT_EQ(src.use_count(), use_count);
  EXPECT_EQ(ptr_shared.get(), src.get());
  EXPECT_EQ(ptr_borrowed.get(), src.get());
  EXPECT_EQ(ptr_shared.shared(), src);
  EXPECT_EQ(ptr_borrowed.borrowed(), src.get());
  EXPECT_EQ(ptr_shared.borrowed(), nullptr);
  EXPECT_EQ(ptr_borrowed.shared(), std::shared_ptr<T>());
}

TEST(Core, PtrAssign)
{
  // Test assigning from various sources with up casting and const addition.
  auto mesh = std::make_shared<SimpleMesh>(0u);

  // Test assignment to the same type.
  testPtrCast<SimpleMesh>(mesh);
  testImplicitArgConvert<SimpleMesh>(mesh, mesh);
  // Test assignment to const
  testPtrCast<const SimpleMesh>(mesh);
  testImplicitArgConvert<const SimpleMesh>(mesh, mesh);
  // Test upcast
  testPtrCast<Resource>(mesh);
  testImplicitArgConvert<Resource>(mesh, mesh);
  // Test const upcast
  testPtrCast<const Resource>(mesh);
  testImplicitArgConvert<const Resource>(mesh, mesh);
}
}  // namespace tes
