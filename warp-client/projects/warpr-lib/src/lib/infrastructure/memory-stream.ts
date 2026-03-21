export class MemoryStream {
  private _buffer: Uint8Array;
  private _size: number;
  private _capacity: number;
  private _dataView: DataView;

  public constructor(initialCapacity: number = 0) {
    this._capacity = initialCapacity;
    this._buffer = new Uint8Array(initialCapacity);
    this._dataView = new DataView(this._buffer.buffer);
    this._size = 0;
  }

  public Clear() {
    this._size = 0;
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
    return this._buffer.buffer.slice(0, this._size) as ArrayBuffer;
  }

  public ToArrayBufferView(): Uint8Array<ArrayBuffer> {
    return new Uint8Array(this._buffer.buffer as ArrayBuffer, 0, this._size);
  }

  private ExtendCapacity(additionalBytes: number): void {
    let requiredCapacity = this._size + additionalBytes;
    if (requiredCapacity > this._capacity) {
      let newCapacity = Math.max(this._capacity * 2, requiredCapacity);
      this.Reserve(newCapacity);
    }
  }

  public WriteUInt32(value: number): void {
    this.ExtendCapacity(4);
    this._dataView.setUint32(this._size, value, true);
    this._size += 4;
  }

  public WriteBytes(bytes: Uint8Array): void {
    this.ExtendCapacity(bytes.length);
    this._buffer.set(bytes, this._size);
    this._size += bytes.length;
  }
}
