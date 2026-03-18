import { MemoryStream } from "./memory-stream";
import { MaxSafeMessageSize, MessageContent, MessageContentType, MessageFragmentHeader } from "./messages";

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

    let header = new MessageFragmentHeader();
    header.MessageIndex = this._messageIndex++;
    header.MessageSize = content.length;
    header.ContentType = contentType;
    header.FragmentSize = messageSize;
    
    let position = 0;
    let fragmentIndex = 0;
    let fragments: ArrayBuffer[] = [];
    while (position < content.length) {
      let fragmentLength = Math.min(messageSize, content.length - position);

      header.Write(stream);      
      stream.WriteBytes(content.subarray(position, position + fragmentLength));

      yield stream.ToArrayBufferView();
      stream.Clear();

      position += fragmentLength;
      header.FragmentIndex++;
    }
  }
}
