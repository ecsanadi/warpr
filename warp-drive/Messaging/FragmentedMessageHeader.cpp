#include "warpr_includes.h"
#include "FragmentedMessageHeader.h"

namespace
{
  const uint32_t MessageFragmentSizeBits = 31;
  constexpr uint32_t MessageFragmentSizeMask = ~(1u << MessageFragmentSizeBits);
}

namespace Warpr::Messaging
{
  MessageContentType FragmentedMessageHeader::ContentType() const
  {
    return MessageContentType(FragmentSizeWithContentType >> MessageFragmentSizeBits);
  }

  void FragmentedMessageHeader::ContentType(MessageContentType value)
  {
    FragmentSizeWithContentType = FragmentSizeWithContentType & MessageFragmentSizeMask | (uint32_t(value) << MessageFragmentSizeBits);
  }

  uint32_t FragmentedMessageHeader::FragmentSize() const
  {
    return FragmentSizeWithContentType & MessageFragmentSizeMask;
  }

  void FragmentedMessageHeader::FragmentSize(uint32_t value)
  {
    FragmentSizeWithContentType = FragmentSizeWithContentType & ~MessageFragmentSizeMask | value;
  }
}