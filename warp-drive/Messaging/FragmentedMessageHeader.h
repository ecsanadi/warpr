#pragma once

#include <cstdint>
#include <cstddef>

namespace Warpr::Messaging
{
  struct FragmentedMessageHeader
  {
    uint32_t MessageIndex;
    uint32_t MessageSize;
    uint32_t FragmentSizeWithFlags;
    uint32_t FragmentIndex;

    static constexpr uint32_t TextFlag = 0x80000000u;
    static constexpr uint32_t SizeMask = 0x7FFFFFFFu;
    static size_t Size;

    bool IsText() const { return (FragmentSizeWithFlags & TextFlag) != 0; }
    uint32_t FragmentSize() const { return FragmentSizeWithFlags & SizeMask; }
  };  

  static_assert(sizeof(FragmentedMessageHeader) == 16, "FragmentedMessageHeader must be 16 bytes.");
}
