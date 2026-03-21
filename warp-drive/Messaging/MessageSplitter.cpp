#include "warpr_includes.h"
#include "MessageSplitter.h"
#include "MessageFragmentHeader.h"

using namespace Axodox::Storage;
using namespace std;
using namespace rtc;

namespace Warpr::Messaging
{
  MessageSplitter::MessageSplitter(size_t maxMessageSize) :
    MaxMessageSize(maxMessageSize)
  {
    _buffer.reserve(maxMessageSize);
  }

  void MessageSplitter::SplitMessage(const rtc::message_variant& message, std::function<void(const rtc::binary&)> onMessage)
  {
    //Retrieve content
    span<const uint8_t> content;
    MessageContentType contentType;
    if (holds_alternative<binary>(message))
    {
      auto& binaryMessage = get<binary>(message);
      content = { reinterpret_cast<const uint8_t*>(binaryMessage.data()), binaryMessage.size() };
      contentType = MessageContentType::Binary;
    }
    else if (holds_alternative<string>(message))
    {
      auto& textMessage = get<string>(message);
      content = { reinterpret_cast<const uint8_t*>(textMessage.data()), textMessage.size() };
      contentType = MessageContentType::Text;
    }
    else
    {
      return;
    }

    //Create header
    auto maxFragmentSize = MaxMessageSize - sizeof(MessageFragmentHeader);

    MessageFragmentHeader header;
    header.MessageIndex++;
    header.MessageSize = uint32_t(content.size());
    header.ContentType(contentType);
    header.FragmentSize(uint32_t(maxFragmentSize));

    //Write fragments
    size_t position = 0u;
    while (position < content.size())
    {
      auto fragmentLength = min(maxFragmentSize, content.size() - position);

      _buffer.resize(fragmentLength + sizeof(MessageFragmentHeader));

      memcpy(_buffer.data(), &header, sizeof(MessageFragmentHeader));
      memcpy(_buffer.data() + sizeof(MessageFragmentHeader), content.data() + position, fragmentLength);

      onMessage(_buffer);
      header.FragmentIndex++;
      position += fragmentLength;
    }
  }
}