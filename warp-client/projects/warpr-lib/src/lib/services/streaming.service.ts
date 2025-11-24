import { Injectable } from '@angular/core';
import { MessagingService } from './messaging.service';
import { GetConfiguration, PairingCompleteMessage, PeerConnectionCandidateMessage, PeerConnectionDescriptionMessage, WarprSignalingMessage, WarprSignalingMessageType } from '../data/signaling-messages';
import { IMessagingClient } from '../networking/messaging-client';
import { EncodedFrame } from '../data/frames';
import { EventOwner, EventPublisher } from '../infrastructure/events';
import { MessageAssembler } from '../infrastructure/message-assembler';
import { MessageSplitter } from '../infrastructure/message-splitter';

@Injectable({
  providedIn: 'root'
})
export class StreamingService {

  private _peerConnection?: RTCPeerConnection;
  private _controlConnection?: RTCDataChannel;

  private _streamConnection?: RTCDataChannel;
  private _streamMessageBuilder?: MessageAssembler;

  private _auxConnection?: RTCDataChannel;
  private _auxMessageBuilder?: MessageAssembler;
  private _auxMessageSplitter = new MessageSplitter();
  private _auxMessageQueue: any[] = [];

  private readonly _events = new EventOwner();
  public readonly FrameReceived = new EventPublisher<StreamingService, EncodedFrame>(this._events);
  public readonly Connected = new EventPublisher<StreamingService, any>(this._events);
  public readonly AuxChannelReady = new EventPublisher<StreamingService, any>(this._events);

  public readonly ControlMessageReceived = new EventPublisher<StreamingService, any>(this._events);
  public readonly AuxMessageReceived = new EventPublisher<StreamingService, any>(this._events);

  constructor(
    private _messagingService: MessagingService) {

    _messagingService.MessageReceived.Subscribe((sender: IMessagingClient<WarprSignalingMessage>, message: WarprSignalingMessage) => this.OnSignalingMessageReceived(sender, message));
    _messagingService.Connect();
  }

  private Reconnect(configuration: RTCConfiguration) {
    console.log("Establishing peer connection...");

    this._controlConnection?.close();
    this._streamConnection?.close();
    this._streamMessageBuilder = new MessageAssembler();
    this._auxConnection?.close();
    this._auxMessageBuilder = new MessageAssembler(1);
    this._peerConnection?.close();

    this._peerConnection = new RTCPeerConnection(configuration);
    this._peerConnection.onicecandidate = (event) => this.OnIceCandidateAdded(event.candidate?.candidate);
    this._peerConnection.ondatachannel = (event) => this.OnDataChannel(event);
    this._peerConnection.onconnectionstatechange = (event) => this.OnConnectionStateChanged();
  }

  public SendControlMessage(message: any) {
    let json = JSON.stringify(message);
    this._controlConnection?.send(json);
  }

  public SendAuxMessage(message: any) {
    if (!this._auxConnection || this._auxConnection.readyState !== 'open') {
      this._auxMessageQueue.push(message);
      return;
    }

    this.SendAuxMessageFragmented(message);
  }

  private SendAuxMessageFragmented(message: any) {
    if (!this._auxConnection || this._auxConnection.readyState !== 'open') {
      return;
    }

    let bytes: Uint8Array;
    let isText = false;
    
    if (typeof message === 'string') {
      bytes = new TextEncoder().encode(message);
      isText = true;
    } else if (message instanceof ArrayBuffer) {
      bytes = new Uint8Array(message);
    } else if (message instanceof Uint8Array) {
      bytes = message;
    } else {
      let json = JSON.stringify(message);
      bytes = new TextEncoder().encode(json);
      isText = true;
    }

    let maxMessageSize = this._peerConnection?.sctp?.maxMessageSize ?? 262144;
    let fragments = this._auxMessageSplitter.SplitMessage(bytes, maxMessageSize, isText);
    console.log(`Sending message: ${bytes.length} bytes in ${fragments.length} fragment(s) (max: ${maxMessageSize})`);
    
    for (let i = 0; i < fragments.length; i++) {
      this._auxConnection.send(fragments[i]);
    }
  }

  private OnConnectionStateChanged() {
    console.log("WebRTC: " + this._peerConnection?.connectionState);
  }

  private OnDataChannel(event: RTCDataChannelEvent) {
    console.log(`WebRTC data channel ${event.channel.label} connected.`);

    switch (event.channel.label) {
      case "control":
        this._controlConnection = event.channel;
        this._controlConnection.binaryType = "arraybuffer"
        this._controlConnection.onmessage = (event) => this._events.Raise(this.ControlMessageReceived, this, event.data);
        this._events.Raise(this.Connected, this, null);
        break;
      case "stream":
        this._streamConnection = event.channel;
        this._streamConnection.binaryType = "arraybuffer"
        this._streamConnection.onmessage = (event) => this.OnStreamMessage(event);
        break;
      case "aux":
        this._auxConnection = event.channel;
        this._auxConnection.binaryType = "arraybuffer"
        this._auxConnection.onmessage = (event) => this.OnAuxMessage(event);
        this._auxConnection.onopen = () => {
          console.log('Aux channel is now open and ready');
          this._events.Raise(this.AuxChannelReady, this, null);
          if (this._auxMessageQueue.length > 0) {
            console.log(`Flushing ${this._auxMessageQueue.length} queued message(s)`);
            while (this._auxMessageQueue.length > 0) {
              let queuedMessage = this._auxMessageQueue.shift();
              this.SendAuxMessageFragmented(queuedMessage);
            }
          }
        };
        this._events.Raise(this.Connected, this, null);
        break;
    }
  }

  private _lastRefreshTime = performance.now();
  private _count = 0;

  private OnStreamMessage(event: MessageEvent<any>) {

    let message = this._streamMessageBuilder?.PushMessage(event.data as ArrayBuffer);
    if (!message) return;

    let now = performance.now();
    let elapsed = now - this._lastRefreshTime;
    this._count++;

    if (elapsed > 1000) {
      this._lastRefreshTime = now;

      console.debug(`Messages: ${this._count / elapsed * 1000} FPS`);
      this._count = 0;
    }

    let frame = new EncodedFrame(message as ArrayBuffer);
    this._events.Raise(this.FrameReceived, this, frame);
  }

  private OnAuxMessage(event: MessageEvent<any>) {
    if (event.data instanceof ArrayBuffer && event.data.byteLength >= 16) {
      let message = this._auxMessageBuilder?.PushMessage(event.data);
      if (!message) return;
      
      this._events.Raise(this.AuxMessageReceived, this, message);
    } else {
      this._events.Raise(this.AuxMessageReceived, this, event.data);
    }
  }

  private OnIceCandidateAdded(candidate?: string) {
    if (candidate == null) return;

    let message = new PeerConnectionCandidateMessage();
    message.Candidate = candidate;
    this._messagingService.SendMessage(message);
  }

  private async OnSignalingMessageReceived(sender: IMessagingClient<WarprSignalingMessage>, message: WarprSignalingMessage) {

    switch (message.$type) {
      case WarprSignalingMessageType.PairingCompleteMessage:
        this.Reconnect(GetConfiguration(message.IceServers));
        break;
      case WarprSignalingMessageType.PeerConnectionDescriptionMessage:
        if (!this._peerConnection) return;

        await this._peerConnection.setRemoteDescription({ type: "offer", sdp: message.Description });
        let answer = await this._peerConnection.createAnswer();
        await this._peerConnection.setLocalDescription(answer);

        let answerMessage = new PeerConnectionDescriptionMessage();
        answerMessage.Description = answer.sdp;
        this._messagingService.SendMessage(answerMessage);

        break;
      case WarprSignalingMessageType.PeerConnectionCandidateMessage:
        if (!this._peerConnection) return;

        await this._peerConnection.addIceCandidate({ candidate: message.Candidate, sdpMid: "0" });
        break;
    }

  }
}
