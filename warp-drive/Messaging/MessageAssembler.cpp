#include "warpr_includes.h"
#include "MessageAssembler.h"
#include "FragmentedMessageHeader.h"

using namespace Axodox::Storage;
using namespace std;
using namespace rtc;

namespace Warpr::Messaging
{
  std::optional<rtc::message_variant> MessageAssembler::PushMessage(const rtc::binary& message)
  {
    //Message must be longer than the header
    if (message.size() < sizeof(FragmentedMessageHeader))
    {
      return std::nullopt;
    }

    //Read header
    array_stream stream{ reinterpret_cast<const vector<uint8_t>&>(message) };

    FragmentedMessageHeader header;
    stream.read(header);

    auto contentType = header.ContentType();
    auto fragmentSize = header.FragmentSize();

    //Check if we have a new message - this method only supports reliable connections
    if (header.MessageIndex != _messageIndex)
    {
      _messageIndex = header.MessageIndex;
      _fragmentCount = (header.MessageSize + fragmentSize - 1) / fragmentSize; // Ceiling division
      _fragmentsReceived = 0;
      _buffer.clear();
      _buffer.resize(header.MessageSize);
    }

    //Copy received segment
    auto fragmentDataSize = message.size() - sizeof(FragmentedMessageHeader);
    auto offset = header.FragmentIndex * fragmentSize;
    if (offset + fragmentDataSize <= _buffer.size())
    {
      std::memcpy(_buffer.data() + offset, message.data() + sizeof(FragmentedMessageHeader), fragmentDataSize);
      _fragmentsReceived++;
    }

    //Check if the message is ready
    if (_fragmentsReceived == _fragmentCount)
    {
      switch (contentType)
      {
        case MessageContentType::Binary:
          return move(_buffer);
        case MessageContentType::Text:
          return string(reinterpret_cast<const char*>(_buffer.data()), _buffer.size());
      }
    }

    return nullopt;
  }  
}