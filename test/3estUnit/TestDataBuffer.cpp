//
// author: Kazys Stepanas
//
#include "TestCommon.h"

#include <3escore/DataBuffer.h>
#include <3escore/PacketReader.h>
#include <3escore/PacketWriter.h>
#include <3escore/tessellate/Sphere.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <functional>
#include <cinttypes>
#include <cmath>
#include <vector>

namespace tes
{
template <typename D, typename T>
void testBufferReadAsType(const DataBuffer &buffer, const std::vector<T> &reference,
                          const char *context,
                          std::function<void(size_t, size_t, D, D, const char *)> validate =
                            std::function<void(size_t, size_t, D, D, const char *)>())
{
  ASSERT_TRUE(buffer.isValid()) << context;
  ASSERT_GT(buffer.count(), 0u) << context;
  ASSERT_EQ(buffer.count() * buffer.componentCount(), reference.size()) << context;

  if (!validate)
  {
    validate = [](size_t i, size_t j, D val, D ref, const char *context)  //
    { ASSERT_EQ(val, ref) << context << " @ [" << i << ',' << j << ']'; };
  }

  D datum{ 0 }, ref{ 0 };
  size_t ref_idx = 0;
  // Start with reading individual elements.
  for (size_t i = 0; i < buffer.count(); ++i)
  {
    for (size_t j = 0; j < buffer.componentCount(); ++j)
    {
      datum = buffer.get<D>(i, j);
      ref = static_cast<D>(reference[ref_idx]);
      validate(i, j, datum, ref, context);
      ++ref_idx;
    }
  }

  // Now try reading as a single block.
  std::vector<D> data(buffer.count() * buffer.componentCount());
  size_t read_elements = buffer.get<D>(0, buffer.count(), data.data(), data.size());
  ASSERT_EQ(read_elements, buffer.count());
  ref_idx = 0;
  for (size_t i = 0; i < buffer.count(); ++i)
  {
    for (size_t j = 0; j < buffer.componentCount(); ++j)
    {
      datum = data[ref_idx];
      ref = static_cast<D>(reference[ref_idx]);
      validate(i, j, datum, ref, context);
      ++ref_idx;
    }
  }

  // Try an offset buffered read.
  size_t offset = buffer.count() / 2;
  size_t read_count = buffer.count() - offset;
  read_elements = buffer.get<D>(offset, read_count, data.data(), data.size());
  ASSERT_EQ(read_elements, read_count);
  ref_idx = 0;
  for (size_t i = 0; i < read_count; ++i)
  {
    for (size_t j = 0; j < buffer.componentCount(); ++j)
    {
      datum = data[ref_idx];
      ref = static_cast<D>(reference[ref_idx + offset * buffer.componentCount()]);
      validate(i, j, datum, ref, context);
      ++ref_idx;
    }
  }
}

template <typename T>
void testBufferRead(const DataBuffer &buffer, const std::vector<T> &reference, const char *context)
{
  testBufferReadAsType<int8_t>(buffer, reference, context);
  testBufferReadAsType<uint8_t>(buffer, reference, context);
  testBufferReadAsType<int16_t>(buffer, reference, context);
  testBufferReadAsType<uint16_t>(buffer, reference, context);
  testBufferReadAsType<int32_t>(buffer, reference, context);
  testBufferReadAsType<uint32_t>(buffer, reference, context);
  testBufferReadAsType<int64_t>(buffer, reference, context);
  testBufferReadAsType<uint64_t>(buffer, reference, context);
  testBufferReadAsType<float>(buffer, reference, context);
  testBufferReadAsType<double>(buffer, reference, context);
}


template <typename real>
void fillDataBuffer(std::vector<Vector3<real>> &vertices, std::vector<real> *reference, real radius)
{
  // Populate our vertices with a set of points from a sphere.
  Vector3<real> vert{ real(0) };

  for (real elevation = 0; elevation < real(90.0); elevation += real(10.0))
  {
    vert.z() = radius * std::sin(elevation * real(M_PI / 180.0));
    for (real azimuth = 0; azimuth < real(360); azimuth += real(10.0))
    {
      const real ring_radius = std::sqrt(radius * radius - vert.z() * vert.z());
      vert.x() = ring_radius * std::cos(azimuth * real(M_PI / 180.0));
      vert.y() = ring_radius * std::sin(azimuth * real(M_PI / 180.0));
      vertices.emplace_back(vert);
    }
  }

  // Convert to a vector of raw reals for the reference set.
  if (reference)
  {
    std::for_each(vertices.begin(), vertices.end(),
                  [reference](const Vector3<real> &v)  //
                  {
                    for (int i = 0; i < 3; ++i)
                    {
                      reference->emplace_back(v[i]);
                    }
                  });
  }
}


template <typename real>
void testVector3Buffer()
{
  std::vector<Vector3<real>> vertices;
  std::vector<real> reference;
  // Use a large radius to excite integer conversions and truncation.
  fillDataBuffer(vertices, &reference, real(128000.0));

  // Populate the buffer from a Vector3 array and test reading as all types.
  DataBuffer buffer(vertices);
  testBufferRead(buffer, reference, "std::vector<Vector3<real>>");

  // Reinitialise the buffer from Vector3 pointer.
  buffer = DataBuffer(vertices.data(), vertices.size());
  testBufferRead(buffer, reference, "Vector3<real>*");

  // Reinitialise from real array
  buffer = DataBuffer(reference, 3);
  testBufferRead(buffer, reference, "std::vector<real>");

  // Reinitialise from real array
  buffer = DataBuffer(reference.data(), reference.size() / 3, 3);
  testBufferRead(buffer, reference, "real*");

  // Now create a strided array which contains padding elements and test with that.
  std::vector<real> strided;
  std::for_each(vertices.begin(), vertices.end(),
                [&strided](const Vector3<real> &v)  //
                {
                  for (int i = 0; i < 3; ++i)
                  {
                    strided.emplace_back(v[i]);
                  }
                  strided.emplace_back(real{ 0 });
                });

  // With std::vector constructor.
  buffer = DataBuffer(strided, 3, 4);
  testBufferRead(buffer, reference, "std::vector<real>*[4]");

  // With pointer constructor.
  buffer = DataBuffer(strided.data(), strided.size() / 4, 3, 4);
  testBufferRead(buffer, reference, "real*[4]");
}

template <typename T>
void testTBuffer(T seed, T increment, size_t count, const char *type_name)
{
  // Build the reference data.
  std::vector<T> reference;
  reference.reserve(count);
  T value{ seed };
  for (size_t i = 0; i < count; ++i, value = T(value + increment))
  {
    reference.emplace_back(value);
  }

  // Migrate into a vertex buffer.
  std::string context;
  DataBuffer buffer;
  buffer = DataBuffer(reference);
  context = std::string("std::vector<") + type_name + ">";
  testBufferRead(buffer, reference, context.c_str());

  buffer = DataBuffer(reference.data(), reference.size());
  context = std::string("") + type_name + "*";
  testBufferRead(buffer, reference, context.c_str());
}

TEST(Buffer, Vector3f)
{
  testVector3Buffer<float>();
}

TEST(Buffer, Vector3d)
{
  testVector3Buffer<double>();
}

TEST(Buffer, Int8)
{
  testTBuffer<int8_t>(-128, 1, 255, "int8");
}

TEST(Buffer, UInt8)
{
  testTBuffer<uint8_t>(0, 1u, 255, "uint8");
}

TEST(Buffer, Int16)
{
  testTBuffer<int16_t>(-500, 1, 1000, "int16");
}

TEST(Buffer, UInt16)
{
  testTBuffer<uint16_t>(0, 1u, 1000, "uint16");
}

TEST(Buffer, Int32)
{
  testTBuffer<int32_t>(-5000, 10, 1000, "int32");
}

TEST(Buffer, UInt32)
{
  testTBuffer<uint32_t>(0, 10u, 1000, "uint32");
}

TEST(Buffer, Int64)
{
  testTBuffer<int64_t>(-5000, 10, 1000, "int64");
}

TEST(Buffer, UInt64)
{
  testTBuffer<uint64_t>(0, 10u, 1000, "uint64");
}

TEST(Buffer, Float32)
{
  testTBuffer<float>(-1000.0f, 3.141f, 1000, "float");
}

TEST(Buffer, Float64)
{
  testTBuffer<double>(-1000.0, 42.42, 1000, "double");
}

template <typename real>
void testPacketStreamVector3(bool packed)
{
  // Test encoding/decoding a vector3 DataBuffer via PacketWriter and PacketReader.
  std::vector<Vector3<real>> vertices;
  std::vector<real> reference;
  fillDataBuffer(vertices, &reference, real(12.8));

  DataBuffer dataBuffer(vertices);

  // Write the pcket. Note, the routing and message types are unimportant.
  std::vector<uint8_t> raw_buffer(std::numeric_limits<uint16_t>::max());
  PacketWriter writer(raw_buffer.data(), int_cast<uint16_t>(raw_buffer.size()));

  unsigned writeCount = 0;
  real quantisation = real(0.001);

  if (!packed)
  {
    writeCount = dataBuffer.write(writer, 0);
  }
  else
  {
    writeCount = dataBuffer.writePacked(writer, 0, quantisation);
  }

  ASSERT_EQ(writeCount, vertices.size());
  ASSERT_TRUE(writer.finalise());

  // Now create a reader around the same data.
  PacketReader reader(reinterpret_cast<PacketHeader *>(raw_buffer.data()));
  // Empty the DataBuffer object before reading.
  dataBuffer = DataBuffer(static_cast<const real *>(nullptr), 0, 3);

  // Now read.
  unsigned readCount = dataBuffer.read(reader);

  ASSERT_NE(readCount, 0);
  ASSERT_EQ(readCount, writeCount);

  // Validate against the reference buffer.

  if (!packed)
  {
    testBufferReadAsType<real, real>(dataBuffer, reference, "Vector3 from stream");
  }
  else
  {
    testBufferReadAsType<real, real>(
      dataBuffer, reference, "Vector3 from stream",
      [quantisation](size_t i, size_t j, real val, real ref, const char *context)  //
      { ASSERT_NEAR(val, ref, quantisation) << context << " @ [" << i << ',' << j << ']'; });
  }
}

TEST(Buffer, StreamVector3f)
{
  testPacketStreamVector3<float>(false);
}

TEST(Buffer, StreamVector3fPacked)
{
  testPacketStreamVector3<float>(true);
}

TEST(Buffer, StreamVector3d)
{
  testPacketStreamVector3<double>(false);
}

TEST(Buffer, StreamVector3dPacked)
{
  testPacketStreamVector3<double>(true);
}
}  // namespace tes
