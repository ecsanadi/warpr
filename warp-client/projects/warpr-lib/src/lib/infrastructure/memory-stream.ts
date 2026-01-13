export class MemoryStream {
  private _buffer: Uint8Array;
  private _position: number;
  private _capacity: number;
  private _dataView: DataView;

  public constructor(initialCapacity: number = 256) {
    this._capacity = initialCapacity;
    this._buffer = new Uint8Array(initialCapacity);
    this._dataView = new DataView(this._buffer.buffer);
    this._position = 0;
  }

  private Reserve(capacity: number): void {
    if (capacity > this._capacity) {
      this._capacity = capacity;
      let newBuffer = new Uint8Array(capacity);
      newBuffer.set(this._buffer);
      this._buffer = newBuffer;
      this._dataView = new DataView(this._buffer.buffer);
    }
  }

  public ToArrayBuffer(): ArrayBuffer {
    return this._buffer.buffer.slice(0, this._position) as ArrayBuffer;
  }

  private ExtendCapacity(additionalBytes: number): void {
    let requiredCapacity = this._position + additionalBytes;
    if (requiredCapacity > this._capacity) {
      let newCapacity = Math.max(this._capacity * 2, requiredCapacity);
      this.Reserve(newCapacity);
    }
  }

  public WriteUInt32(value: number): void {
    this.ExtendCapacity(4);
    this._dataView.setUint32(this._position, value, true);
    this._position += 4;
  }

  public WriteBytes(bytes: Uint8Array): void {
    this.ExtendCapacity(bytes.length);
    this._buffer.set(bytes, this._position);
    this._position += bytes.length;
  }
}
