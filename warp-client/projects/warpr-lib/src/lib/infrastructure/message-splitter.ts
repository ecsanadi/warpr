import { MemoryStream } from "./memory-stream";

export class MessageSplitter {
  private _messageIndex: number = 0;

  public SplitMessage(
    bytes: Uint8Array,
    maxMessageSize: number,
    isText: boolean = false
  ): ArrayBuffer[] {
    const fragments: ArrayBuffer[] = [];
    const messageSize = maxMessageSize - 16;
    
    const messageSizeWithFlag = messageSize | (isText ? 0x80000000 : 0);

    let position = 0;
    let fragmentIndex = 0;

    while (position < bytes.length) {
      const fragmentLength = Math.min(messageSize, bytes.length - position);
      const stream = new MemoryStream(maxMessageSize);

      stream.WriteUInt32(this._messageIndex);
      stream.WriteUInt32(bytes.length);
      stream.WriteUInt32(messageSizeWithFlag);
      stream.WriteUInt32(fragmentIndex++);
      stream.WriteBytes(bytes.subarray(position, position + fragmentLength));

      fragments.push(stream.ToArrayBuffer());
      position += fragmentLength;
    }

    this._messageIndex++;
    return fragments;
  }
}
