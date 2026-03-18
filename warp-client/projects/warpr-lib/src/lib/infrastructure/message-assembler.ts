import { ArrayStream } from "./array-stream";
import { MessageContentType, MessageContent, MessageFragmentHeader } from "./messages";

class MessageBuilder {
  public readonly FragmentCount: number;
  public FragmentsReceived: number = 0;
  public Buffer: Uint8Array;
  private readonly _textDecoder = new TextDecoder();

  constructor(
    public readonly Id: number,
    public readonly Size: number,
    public readonly FragmentSize: number,
    public readonly ContentType: MessageContentType = MessageContentType.Binary
  ) {
    this.FragmentCount = Math.ceil(Size / FragmentSize);
    this.Buffer = new Uint8Array(Size);
  }

  AddFragment(index: number, buffer: Uint8Array): boolean {
    if (this.FragmentCount == this.FragmentsReceived) throw RangeError("All message parts have been already collected.");

    this.Buffer.set(buffer, this.FragmentSize * index);
    this.FragmentsReceived++;

    return this.FragmentCount == this.FragmentsReceived;
  }

  GetMessage(): MessageContent | null {
    if (this.FragmentCount == this.FragmentsReceived) {
      switch (this.ContentType) {
        case MessageContentType.Binary:
          return this.Buffer.buffer;
        case MessageContentType.Text:
          return this._textDecoder.decode(this.Buffer.buffer);
      }
    }
    
    return null;
  }
}

export class MessageAssembler {

  private _builders = new Map<number, MessageBuilder>();

  public constructor(private _maxBuilderCount = 1) { }

  public PushMessage(buffer: ArrayBuffer): MessageContent | null {
    let stream = new ArrayStream(buffer);
    
    let header = new MessageFragmentHeader();
    header.Read(stream);

    let fragment = new Uint8Array(stream.ReadToEnd());

    let builder = this._builders.get(header.MessageIndex);
    if (builder === undefined) {
      builder = new MessageBuilder(header.MessageIndex, header.MessageSize, header.FragmentSize, header.ContentType);
      this._builders.set(header.MessageIndex, builder);

      if (this._builders.size > this._maxBuilderCount) {
        let [oldest] = this._builders.keys();
        this._builders.delete(oldest);
        console.log("Message lost.");
      }
    }

    if (builder.AddFragment(header.FragmentIndex, fragment)) {
      this._builders.delete(header.MessageIndex);

      return builder.GetMessage();
    } else {
      return null;
    }
  }
}
