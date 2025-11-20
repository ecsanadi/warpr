export class MemoryStream {
  private _buffer: Uint8Array;
  private _position: number;
  private _capacity: number;

  public constructor(initialCapacity: number = 256) {
    this._capacity = initialCapacity;
    this._buffer = new Uint8Array(initialCapacity);
    this._position = 0;
  }

  public get Position(): number {
    return this._position;
  }

  public get Length(): number {
    return this._position;
  }

  public Reserve(capacity: number): void {
    if (capacity > this._capacity) {
      this._capacity = capacity;
      const newBuffer = new Uint8Array(capacity);
      newBuffer.set(this._buffer);
      this._buffer = newBuffer;
    }
  }

  public Reset(): void {
    this._position = 0;
  }

  public ToArrayBuffer(): ArrayBuffer {
    return this._buffer.buffer.slice(0, this._position) as ArrayBuffer;
  }

  private EnsureCapacity(additionalBytes: number): void {
    const requiredCapacity = this._position + additionalBytes;
    if (requiredCapacity > this._capacity) {
      const newCapacity = Math.max(this._capacity * 2, requiredCapacity);
      this.Reserve(newCapacity);
    }
  }

  public WriteUInt32(value: number): void {
    this.EnsureCapacity(4);
    const view = new DataView(this._buffer.buffer);
    view.setUint32(this._position, value, true);
    this._position += 4;
  }

  public WriteBytes(bytes: Uint8Array): void {
    this.EnsureCapacity(bytes.length);
    this._buffer.set(bytes, this._position);
    this._position += bytes.length;
  }
}
