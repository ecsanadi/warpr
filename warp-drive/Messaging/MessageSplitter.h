#pragma once
#include "warpr_includes.h"

namespace Warpr::Messaging
{
  class MessageSplitter
  {
  public:
    MessageSplitter(size_t maxMessageSize = 256 * 1024);

    size_t MaxMessageSize;

    void SplitMessage(const rtc::message_variant& message, std::function<void(const rtc::binary&)> onMessage);

  private:
    rtc::binary _buffer;
  };
}