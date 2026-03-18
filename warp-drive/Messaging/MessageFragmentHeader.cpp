#include "warpr_includes.h"
#include "MessageFragmentHeader.h"

namespace
{
  const uint32_t MessageFragmentSizeBits = 31;
  constexpr uint32_t MessageFragmentSizeMask = ~(1u << MessageFragmentSizeBits);
}

namespace Warpr::Messaging
{
  MessageContentType MessageFragmentHeader::ContentType() const
  {
    return MessageContentType(FragmentSizeWithContentType >> MessageFragmentSizeBits);
  }

  void MessageFragmentHeader::ContentType(MessageContentType value)
  {
    FragmentSizeWithContentType = FragmentSizeWithContentType & MessageFragmentSizeMask | (uint32_t(value) << MessageFragmentSizeBits);
  }

  uint32_t MessageFragmentHeader::FragmentSize() const
  {
    return FragmentSizeWithContentType & MessageFragmentSizeMask;
  }

  void MessageFragmentHeader::FragmentSize(uint32_t value)
  {
    FragmentSizeWithContentType = FragmentSizeWithContentType & ~MessageFragmentSizeMask | value;
  }
}