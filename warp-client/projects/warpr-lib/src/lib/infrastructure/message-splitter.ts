import { MemoryStream } from "./memory-stream";

export class MessageSplitter {
  private _messageIndex: number = 0;
  private _maxMessageSize: number;

  public constructor(maxMessageSize: number = 262144) {
    this._maxMessageSize = maxMessageSize;
  }

  set MaxMessageSize(value: number) {
    this._maxMessageSize = value;
  }

  public SplitMessage(message: any): ArrayBuffer[] {
    let bytes: Uint8Array;
    let isText: boolean = false;
    let fragments: ArrayBuffer[] = [];    
    let position = 0;
    let fragmentIndex = 0;

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

    let messageSize = this._maxMessageSize - 16;
    let fragmentSizeWithFlag = messageSize | (isText ? 0x80000000 : 0);

    while (position < bytes.length) {
      let fragmentLength = Math.min(messageSize, bytes.length - position);
      let stream = new MemoryStream(this._maxMessageSize);

      stream.WriteUInt32(this._messageIndex);
      stream.WriteUInt32(bytes.length);
      stream.WriteUInt32(fragmentSizeWithFlag);
      stream.WriteUInt32(fragmentIndex++);
      stream.WriteBytes(bytes.subarray(position, position + fragmentLength));

      fragments.push(stream.ToArrayBuffer());
      position += fragmentLength;
    }

    this._messageIndex++;
    return fragments;
  }
}
