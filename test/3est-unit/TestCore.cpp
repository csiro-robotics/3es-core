//
// author: Kazys Stepanas
//
#include <gtest/gtest.h>

#include <cinttypes>

#include "3est-common.h"

#include <3escore/IntArg.h>
#include <3escore/V3Arg.h>

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
}  // namespace tes
