#include "warpr_includes.h"
#include "MessageSplitter.h"
#include "FragmentedMessageHeader.h"

using namespace Axodox::Storage;
using namespace std;

namespace Warpr::Messaging
{
  std::vector<memory_stream> MessageSplitter::SplitMessage(size_t maxMessageSize, bool isText, std::span<const uint8_t> bytes, uint32_t index)
  {
    std::vector<memory_stream> result;

    auto messageSize = maxMessageSize - Warpr::Messaging::FragmentedMessageHeader::Size;
    auto messageSizeWithFlag = messageSize | (isText ? FragmentedMessageHeader::TextFlag : 0u);

    memory_stream stream;
    stream.reserve(maxMessageSize);

    size_t position = 0u;
    auto fragmentIndex = 0u;
    while (position < bytes.size())
    {
      auto fragmentLength = min(messageSize, bytes.size() - position);
      stream.reset();

      stream.write(uint32_t(index));
      stream.write(uint32_t(bytes.size()));
      stream.write(uint32_t(messageSizeWithFlag));
      stream.write(uint32_t(fragmentIndex++));
      stream.write(bytes.subspan(position, fragmentLength));

      result.push_back(stream);
      position += fragmentLength;
    }
    return result;
  }
}