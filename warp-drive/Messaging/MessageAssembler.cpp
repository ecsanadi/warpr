#include "warpr_includes.h"
#include "MessageAssembler.h"
#include "FragmentedMessageHeader.h"

using namespace Axodox::Storage;

namespace Warpr::Messaging
{
  std::optional<rtc::message_variant> MessageAssembler::PushMessage(const rtc::binary& message)
  {

    auto& binaryData = (message);
    if (binaryData.size() < Warpr::Messaging::FragmentedMessageHeader::Size)
    {
      return std::nullopt;
    }

    // Message header
    memory_stream stream;
    stream.write(std::span<const std::byte>(binaryData.data(), binaryData.size()));
    stream.seek(0);

    FragmentedMessageHeader header{};
    stream.read(header.MessageIndex);
    stream.read(header.MessageSize);
    stream.read(header.FragmentSizeWithFlags);
    stream.read(header.FragmentIndex);

    bool isText = header.IsText();
    uint32_t fragmentSize = header.FragmentSize();

    if (header.MessageIndex != _messageIndex)
    {
      _messageIndex = header.MessageIndex;
      _fragmentCount = (header.MessageSize + fragmentSize - 1) / fragmentSize; // Ceiling division
      _fragmentsReady = 0;
      _buffer.clear();
      _buffer.resize(header.MessageSize);
    }

    // Copy to buffer
    auto fragmentDataSize = binaryData.size() - FragmentedMessageHeader::Size;
    auto offset = header.FragmentIndex * fragmentSize;
    if (offset + fragmentDataSize <= _buffer.size())
    {
      std::memcpy(_buffer.data() + offset, binaryData.data() + FragmentedMessageHeader::Size, fragmentDataSize);
      _fragmentsReady++;
    }

    if (_fragmentsReady == _fragmentCount)
    {
      if (isText)
      {
        return rtc::string(reinterpret_cast<const char*>(_buffer.data()), _buffer.size());
      }
      else
      {
        rtc::binary result;
        result.resize(_buffer.size());
        std::memcpy(result.data(), _buffer.data(), _buffer.size());
        return result;
      }
    }

    return std::nullopt;
  }  
}