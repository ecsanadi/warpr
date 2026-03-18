#pragma once
#include "warpr_includes.h"

namespace Warpr::Messaging
{
  class MessageAssembler
  {
  public:
    std::optional<rtc::message_variant> PushMessage(const rtc::binary& message);

  private:
    uint32_t _messageIndex = 0;
    uint32_t _fragmentCount = 0;
    uint32_t _fragmentsReceived = 0;
    rtc::binary _buffer;
  };  
}