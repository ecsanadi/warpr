#include "warpr_includes.h"
#include "WebRtcClient.h"

using namespace Axodox::Infrastructure;
using namespace Axodox::Storage;
using namespace Axodox::Threading;
using namespace rtc;
using namespace std;
using namespace std::chrono;

namespace Warpr::Messaging
{
  std::optional<rtc::message_variant> MessageAssembler::PushMessage(const rtc::message_variant& message)
  {
    // Only handle binary messages
    if (!std::holds_alternative<rtc::binary>(message)) {
      return message;
    }

    auto& binaryData = std::get<rtc::binary>(message);
    if (binaryData.size() < 16) {
      return std::nullopt;
    }

    // Decode message header
    memory_stream stream;
    stream.write(std::span<const std::byte>(binaryData.data(), binaryData.size()));
    stream.seek(0);

    uint32_t messageIndex;
    uint32_t messageSize;
    uint32_t messageSizeWithFlag;
    uint32_t fragmentIndex;

    stream.read(messageIndex);
    stream.read(messageSize);
    stream.read(messageSizeWithFlag);
    stream.read(fragmentIndex);

    bool isText = (messageSizeWithFlag & 0x80000000u) != 0;
    uint32_t fragmentSize = messageSizeWithFlag & 0x7FFFFFFFu;

    // Check if this is a new message
    if (messageIndex != _messageIndex) {
      _messageIndex = messageIndex;
      _fragmentCount = (messageSize + fragmentSize - 1) / fragmentSize; // Ceiling division
      _fragmentsReady = 0;
      _buffer.clear();
      _buffer.resize(messageSize);
    }

    // Copy fragment data to buffer
    auto fragmentDataSize = binaryData.size() - 16;
    auto offset = fragmentIndex * fragmentSize;
    if (offset + fragmentDataSize <= _buffer.size()) {
      std::memcpy(_buffer.data() + offset, binaryData.data() + 16, fragmentDataSize);
      _fragmentsReady++;
    }

    // Check if message is complete
    if (_fragmentsReady == _fragmentCount) {
      if (isText) {
        return rtc::string(reinterpret_cast<const char*>(_buffer.data()), _buffer.size());
      } else {
        rtc::binary result;
        result.resize(_buffer.size());
        std::memcpy(result.data(), _buffer.data(), _buffer.size());
        return result;
      }
    }

    return std::nullopt;
  }

  const std::string_view WebRtcClient::_stateNames[] = {
    "New",
    "Connecting",
    "Connected",
    "Disconnected",
    "Failed",
    "Closed"
  };

  const std::string_view WebRtcClient::_iceStateNames[] = {
    "New",
    "Checking",
    "Connected",
    "Completed",
    "Failed",
    "Disconnected",
    "Closed"
  };

  const std::string_view WebRtcClient::_gatheringStateNames[] = {
    "New",
    "InProgress",
    "Complete"
  };

  WebRtcClient::WebRtcClient(Axodox::Infrastructure::dependency_container* container) :
    ControlMessageReceived(_events),
    AuxMessageReceived(_events),
    _containerRef(container->get_ref()),
    _settings(container->resolve<WarpConfiguration>()),
    _signaler(container->resolve<WebSocketClient>()),
    _signalerMessageReceivedSubscription(_signaler->MessageReceived({ this, &WebRtcClient::OnSignalingMessageReceived }))
  { }

  bool WebRtcClient::IsConnected() const
  {
    return _peerConnection && _peerConnection->state() == PeerConnection::State::Connected;
  }

  void WebRtcClient::SendVideoFrame(std::span<const uint8_t> bytes)
  {
    SendMessage(_streamChannel.get(), bytes, false);

    using namespace std::chrono;
    static steady_clock::time_point _lastReportingTime = {};
    static uint64_t _dataSent = 0;
    _dataSent += bytes.size();
    auto now = steady_clock::now();
    if (now - _lastReportingTime > 1s)
    {
      _logger.log(log_severity::debug, L"Output buffer size: {} bytes", _streamChannel->bufferedAmount());
      _logger.log(log_severity::debug, L"Output data rate: {} bytes/second", _dataSent);
      _lastReportingTime = now;
      _dataSent = 0;
    }
  }

  void WebRtcClient::SendControlMessage(const rtc::message_variant& message)
  {
    lock_guard lock(_mutex);
    if (!IsConnected()) return;
    _controlChannel->send(message);
  }

  void WebRtcClient::SendAuxMessage(const rtc::message_variant& message)
  {
    lock_guard lock(_mutex);
    if (!IsConnected()) return;
    
    if (std::holds_alternative<std::string>(message))
    {
      const std::string& text = std::get<std::string>(message);
      auto bytes = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(text.data()), text.size());
      SendMessage(_auxChannel.get(), bytes, true);
    }
    
    if (std::holds_alternative<rtc::binary>(message))
    {
      const rtc::binary& bin = std::get<rtc::binary>(message);  
      auto bytes = std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(bin.data()), bin.size());
      SendMessage(_auxChannel.get(), bytes, false);
    }
  }

  void WebRtcClient::OnSignalingMessageReceived(WebSocketClient* sender, const WarprSignalingMessage* message)
  {
    lock_guard lock(_mutex);

    switch (message->Type())
    {
    case WarprSignalingMessageType::PairingCompleteMessage:
      OnPairingComplete(static_cast<const PairingCompleteMessage*>(message));
      break;

    case WarprSignalingMessageType::PeerConnectionDescriptionMessage:
      if (_peerConnection)
      {
        auto descriptionMessage = static_cast<const PeerConnectionDescriptionMessage*>(message);
        _peerConnection->setRemoteDescription(*descriptionMessage->Description);
        _logger.log(log_severity::debug, "Remote description:\n{}", *descriptionMessage->Description);
      }
      break;

    case WarprSignalingMessageType::PeerConnectionCandidateMessage:
      if (_peerConnection)
      {
        auto descriptionMessage = static_cast<const PeerConnectionCandidateMessage*>(message);
        _peerConnection->addRemoteCandidate(*descriptionMessage->Candidate);
        _logger.log(log_severity::debug, "Remote candidate: {}", *descriptionMessage->Candidate);
      }
      break;

    default:
      break;
    }
  }

  void WebRtcClient::OnPairingComplete(const PairingCompleteMessage* message)
  {
    _configuration.ConnectionTimeout = duration<float>(*message->ConnectionTimeout);
    _configuration.IceServers.clear();
    _configuration.IceServers.reserve(message->IceServers->size());
    for (auto& server : *message->IceServers)
    {
      _configuration.IceServers.emplace_back(server);
    }
    _configuration.IsConnectionUnordered = *message->StreamChannelReliability->IsUnordered;
    _configuration.MaxRetransmits = *message->StreamChannelReliability->MaxRetransmits;
    _configuration.MaxPacketLifetime = *message->StreamChannelReliability->MaxPacketLifetime;

    _connectionThread.reset();
    _connectionThread = background_thread({ this, &WebRtcClient::Connect }, "* webrtc connect");
  }

  void WebRtcClient::SendMessage(rtc::DataChannel* channel, std::span<const uint8_t> bytes, bool isText)
  {
    lock_guard lock(_mutex);

    if (!IsConnected()) return;

    {
      auto messageSize = _peerConnection->remoteMaxMessageSize() - 16;
      
      uint32_t messageSizeWithFlag = messageSize | (isText ? 0x80000000u : 0u);

      memory_stream stream;
      stream.reserve(_peerConnection->remoteMaxMessageSize());

      size_t position = 0u;
      auto fragmentIndex = 0u;
      while (position < bytes.size())
      {
        auto fragmentLength = min(messageSize, bytes.size() - position);
        stream.reset();

        stream.write(uint32_t(_messageIndex));
        stream.write(uint32_t(bytes.size()));
        stream.write(uint32_t(messageSizeWithFlag));
        stream.write(uint32_t(fragmentIndex++));
        stream.write(bytes.subspan(position, fragmentLength));

        channel->send(reinterpret_cast<const std::byte*>(stream.data()), stream.length());
        position += fragmentLength;
      }

      _messageIndex++;
    }
  }

  void WebRtcClient::Connect()
  {
    while (!_connectionThread.is_exiting())
    {
      _logger.log(log_severity::information, "Initializing WebRTC connection...");

      {
        lock_guard lock(_mutex);

        //Reset existing connection
        _auxChannel.reset();
        _controlChannel.reset();
        _streamChannel.reset();
        _peerConnection.reset();

        //Create configuration
        Configuration config;
        config.iceServers = _configuration.IceServers;
        config.maxMessageSize = 256 * 1024;
        config.mtu = 1500;

        //Create peer connection      
        _peerConnection = make_unique<PeerConnection>(config);

        _peerConnection->onLocalDescription([=](const Description& description) {
          PeerConnectionDescriptionMessage message;
          *message.Description = string(description);
          _signaler->SendMessage(message);

          _logger.log(log_severity::debug, "Local description:\n{}", *message.Description);
          });

        _peerConnection->onLocalCandidate([=](const Candidate& candidate) {
          PeerConnectionCandidateMessage message;
          *message.Candidate = string(candidate);
          _signaler->SendMessage(message);

          _logger.log(log_severity::debug, "Local candidate: {}", *message.Candidate);
          });

        _peerConnection->onStateChange([=](PeerConnection::State state) {
          _logger.log(log_severity::information, "State changed to '{}'.", _stateNames[size_t(state)]);
          });

        _peerConnection->onIceStateChange([=](PeerConnection::IceState state) {
          _logger.log(log_severity::information, "ICE State changed to '{}'.", _iceStateNames[size_t(state)]);
          });

        _peerConnection->onGatheringStateChange([=](PeerConnection::GatheringState state) {
          _logger.log(log_severity::information, "Gathering State changed to '{}'.", _gatheringStateNames[size_t(state)]);
          });

        //Create data channel
        _streamChannel = _peerConnection->createDataChannel("stream", {
          .reliability = {
            .unordered = _configuration.IsConnectionUnordered,
            .maxPacketLifeTime = _configuration.MaxPacketLifetime,
            .maxRetransmits = _configuration.MaxRetransmits
          }
          });
        _controlChannel = _peerConnection->createDataChannel("control");
        _auxChannel = _peerConnection->createDataChannel("aux");

        _controlChannel->onMessage([=](message_variant data) {
          _events.raise(ControlMessageReceived, this, &data);
          });
        _auxChannel->onMessage([=](message_variant data) {
          auto assembled = _auxMessageAssembler.PushMessage(data);
          if (assembled.has_value()) {
            _events.raise(AuxMessageReceived, this, &assembled.value());
          }
          });
      }

      this_thread::sleep_for(_configuration.ConnectionTimeout);
      if (_peerConnection->state() == PeerConnection::State::Connected) break;

      _logger.log(log_severity::warning, "Failed to connect, retrying...");
    }
  }
}