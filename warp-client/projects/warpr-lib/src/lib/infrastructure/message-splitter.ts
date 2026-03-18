import { MemoryStream } from "./memory-stream";
import { MaxSafeMessageSize, MessageContent, MessageContentType, MessageFragmentSizeBits } from "./messages";

export class MessageSplitter {
  private _messageIndex: number = 0;
  private readonly _textEncoder = new TextEncoder();

  public constructor(public MaxMessageSize = MaxSafeMessageSize) {
  }

  public *SplitMessage(message: MessageContent): Generator<Uint8Array<ArrayBuffer>> {
    let content: Uint8Array;
    let contentType : MessageContentType;

    if (typeof message === 'string') {
      content = this._textEncoder.encode(message);
      contentType = MessageContentType.Text;
    }
    else{
      content = new Uint8Array(message);
      contentType = MessageContentType.Binary;
    }
    
    let messageSize = this.MaxMessageSize - 16;
    let stream = new MemoryStream(this.MaxMessageSize);

    let fragmentSizeWithFlag = messageSize | (contentType << MessageFragmentSizeBits);
    
    let position = 0;
    let fragmentIndex = 0;
    let fragments: ArrayBuffer[] = [];
    while (position < content.length) {
      let fragmentLength = Math.min(messageSize, content.length - position);

      stream.WriteUInt32(this._messageIndex);
      stream.WriteUInt32(content.length);
      stream.WriteUInt32(fragmentSizeWithFlag);
      stream.WriteUInt32(fragmentIndex++);
      stream.WriteBytes(content.subarray(position, position + fragmentLength));

      yield stream.ToArrayBufferView();
      stream.Clear();

      position += fragmentLength;
    }

    this._messageIndex++;
  }
}
