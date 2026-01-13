#pragma once
//#include "warpr_includes.h"

namespace Warpr::Messaging
{
  class MessageSplitter
  {
  public:
    std::vector<Axodox::Storage::memory_stream> SplitMessage(size_t maxMessageSize, bool isText, std::span<const uint8_t> bytes, uint32_t index);
  };
}