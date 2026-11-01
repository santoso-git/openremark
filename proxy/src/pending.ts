import type { ChatMessage } from './upstream.ts'

// The previous turn of one client, kept so a follow-up tap ("yes") continues it.
export type Pending = { messages: ChatMessage[]; blocked?: { fqName: string; args: string } }

export class PendingStore {
  #ttlMs: number
  #now: () => number
  #items = new Map<string, { at: number; pending: Pending }>()

  constructor(ttlMs: number, now: () => number = Date.now) {
    this.#ttlMs = ttlMs
    this.#now = now
  }

  get(clientKey: string): Pending | undefined {
    const item = this.#items.get(clientKey)
    if (!item) return undefined
    if (this.#now() - item.at > this.#ttlMs) { this.#items.delete(clientKey); return undefined }
    return item.pending
  }

  set(clientKey: string, pending: Pending): void { this.#items.set(clientKey, { at: this.#now(), pending }) }

  clear(clientKey: string): void { this.#items.delete(clientKey) }
}
