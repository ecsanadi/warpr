#pragma once
#include "warpr_includes.h"

namespace Warpr::Messaging
{
  enum class MessageContentType
  {
    Binary,
    Text
  };

  struct FragmentedMessageHeader
  {
    uint32_t MessageIndex = 0;
    uint32_t MessageSize = 0;
    uint32_t FragmentSizeWithContentType = 0;
    uint32_t FragmentIndex = 0;

    MessageContentType ContentType() const;
    void ContentType(MessageContentType value);

    uint32_t FragmentSize() const;
    void FragmentSize(uint32_t value);
  };  

  static_assert(sizeof(FragmentedMessageHeader) == 16, "FragmentedMessageHeader must be 16 bytes.");
}
