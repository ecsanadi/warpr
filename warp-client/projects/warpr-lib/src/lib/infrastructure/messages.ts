import { ArrayStream } from "./array-stream";
import { MemoryStream } from "./memory-stream";

export enum MessageContentType {
  Binary,
  Text
}

export type MessageContent = ArrayBufferLike | string;

export const MaxSafeMessageSize = 256 * 1024;

export class MessageFragmentHeader {
  static readonly _fragmentSizeBits = 31;
  static readonly _fragmentSizeMask = ~(1 << MessageFragmentHeader._fragmentSizeBits);

  public MessageIndex = 0;
  public MessageSize = 0;
  public FragmentSizeWithContentType = 0;
  public FragmentIndex = 0;

  public get ContentType(): MessageContentType {
    return this.FragmentSizeWithContentType >>> MessageFragmentHeader._fragmentSizeBits;
  }

  public set ContentType(value: MessageContentType) {
     this.FragmentSizeWithContentType = this.FragmentSizeWithContentType & MessageFragmentHeader._fragmentSizeMask | (value << MessageFragmentHeader._fragmentSizeBits);
  }

  public get FragmentSize() : number
  {
    return this.FragmentSizeWithContentType & MessageFragmentHeader._fragmentSizeMask;
  }

  public set FragmentSize(value: number)
  {
    this.FragmentSizeWithContentType = this.FragmentSizeWithContentType & ~MessageFragmentHeader._fragmentSizeMask | value;
  }

  public Write(stream: MemoryStream)
  {
    stream.WriteUInt32(this.MessageIndex);
    stream.WriteUInt32(this.MessageSize);
    stream.WriteUInt32(this.FragmentSizeWithContentType);
    stream.WriteUInt32(this.FragmentIndex);
  }

  public Read(stream: ArrayStream)
  {
    this.MessageIndex = stream.ReadUInt32();
    this.MessageSize = stream.ReadUInt32();
    this.FragmentSizeWithContentType = stream.ReadUInt32();
    this.FragmentIndex = stream.ReadUInt32();
  }
}
