//
// author: Kazys Stepanas
//
#include "Crc.h"

#include <array>

namespace tes
{
// Crc code taken from http://www.barrgroup.com/Embedded-Systems/How-To/CRC-Calculation-C-Code
template <typename CRC>
class CrcCalc
{
public:
  CrcCalc(CRC initial_remainder, CRC final_xor_value, CRC polynomial) noexcept;

  CRC crc(const uint8_t *message, size_t byte_count) const;

  inline CRC operator()(const uint8_t *message, size_t byte_count) const
  {
    return crc(message, byte_count);
  }

private:
  CRC _initial_remainder;
  CRC _final_xor_value;
  std::array<CRC, 256> _crc_table;

  void initTable(CRC polynomial) noexcept;

  static constexpr CRC kWidth = (8 * sizeof(CRC));
  static constexpr CRC kTopBit = static_cast<CRC>(1u << ((8u * sizeof(CRC)) - 1));
};


template <typename CRC>
CrcCalc<CRC>::CrcCalc(CRC initial_remainder, CRC final_xor_value, CRC polynomial) noexcept
  : _initial_remainder(initial_remainder)
  , _final_xor_value(final_xor_value)
{
  initTable(polynomial);
}


template <typename CRC>
CRC CrcCalc<CRC>::crc(const uint8_t *message, size_t byte_count) const
{
  uint8_t data;
  CRC remainder = _initial_remainder;

  // Divide the message by the polynomial, a byte at a time.
  for (size_t byte = 0u; byte < byte_count; ++byte)
  {
    // NOLINTBEGIN(hicpp-signed-bitwise)
    data = static_cast<uint8_t>(message[byte] ^ (remainder >> (kWidth - 8u)));
    remainder = static_cast<CRC>(_crc_table[data] ^ (remainder << 8u));
    // NOLINTEND(hicpp-signed-bitwise)
  }

  // The final remainder is the CRC.
  return remainder ^ _final_xor_value;
}


template <typename CRC>
void CrcCalc<CRC>::initTable(CRC polynomial) noexcept
{
  CRC remainder = 0;

  // Compute the remainder of each possible dividend.
  for (unsigned dividend = 0; dividend < _crc_table.size(); ++dividend)
  {
    // Start with the dividend followed by zeros.
    remainder = static_cast<CRC>(dividend << (kWidth - 8u));

    // Perform modulo-2 division, a bit at a time.
    for (uint8_t bit = 8; bit > 0; --bit)
    {
      // Try to divide the current data bit.
      if (remainder & kTopBit)
      {
        // NOLINTNEXTLINE(hicpp-signed-bitwise)
        remainder = static_cast<CRC>((remainder << 1u) ^ polynomial);
      }
      else
      {
        remainder = static_cast<CRC>(remainder << 1u);
      }
    }

    // Store the result into the table.
    _crc_table[dividend] = remainder;
  }
}


static const CrcCalc<uint8_t> kCrc8(0xFFu, 0u, 0x21u);
static const CrcCalc<uint16_t> kCrc16(0xFFFFu, 0u, 0x1021u);
static const CrcCalc<uint32_t> kCrc32(0xFFFFFFFFu, 0xFFFFFFFFu, 0x04C11DB7u);


uint8_t crc8(const uint8_t *message, size_t byte_count)
{
  return kCrc8(message, byte_count);
}


uint16_t crc16(const uint8_t *message, size_t byte_count)
{
  return kCrc16(message, byte_count);
}


uint32_t crc32(const uint8_t *message, size_t byte_count)
{
  return kCrc32(message, byte_count);
}
}  // namespace tes
