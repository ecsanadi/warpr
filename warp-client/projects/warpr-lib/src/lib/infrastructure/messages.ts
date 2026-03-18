export enum MessageContentType {
  Binary,
  Text
}

export type MessageContent = ArrayBufferLike | string;

export const MessageFragmentSizeBits = 31;
export const MaxSafeMessageSize = 256 * 1024;
