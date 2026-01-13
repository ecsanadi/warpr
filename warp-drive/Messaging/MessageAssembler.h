#pragma once

namespace Warpr::Messaging
{
  class MessageAssembler
  {
  public:
    std::optional<rtc::message_variant> PushMessage(const rtc::binary& message);

  private:
    uint32_t _messageIndex = 0;
    uint32_t _fragmentCount = 0;
    uint32_t _fragmentsReady = 0;
    std::vector<uint8_t> _buffer;
  };  
}