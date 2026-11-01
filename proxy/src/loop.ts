import type { McpRegistry, ToolEntry } from './mcp.ts'
import type { Pending, PendingStore } from './pending.ts'
import type { ChatMessage, ChatRequest, ChatResponse, ToolSpec, Upstream } from './upstream.ts'

export type LoopContext = {
  clientKey: string
  registry: McpRegistry
  upstream: Upstream
  pending: PendingStore
  maxSteps: number
  onToolCall?: (fqName: string) => void
  // The tablet gives up after 180 s; nothing may run after that.
  deadlineMs?: number
  now?: () => number
  signal?: AbortSignal
}

export class TurnTimeoutError extends Error {
  constructor() { super('The request took too long') }
}

const NOT_EXECUTED = "Not executed: this action needs the user's confirmation. Summarise exactly what would be done and ask the user to write yes and tap again."

function systemNote(tools: ToolEntry[], unavailable: string[], prior: Pending | undefined, instructions: Record<string, string>): string {
  const servers = [...new Set(tools.map((t) => t.server))]
  const lines = [`You can use tools from these services: ${servers.join(', ')}.`]
  for (const server of servers) if (instructions[server]) lines.push(`About ${server}: ${instructions[server]}`)
  lines.push('Anything that belongs to one of these services must be done or looked up with its tools, judged by the descriptions above and by the tools themselves, even when the question does not name the service. If the user asks to create or change something there, use the tool that does it. Answer without tools only when no service fits.')
  lines.push('Never invent codes, prices, offers, numbers or other facts that a tool did not return; if a tool cannot give it, say so.')
  if (unavailable.length) lines.push(`Unavailable right now: ${unavailable.join(', ')}. Mention this if the user asks about it.`)
  if (prior) {
    lines.push(prior.blocked
      ? `A previous turn is still open. If the user's newest writing approves the pending action ${prior.blocked.fqName}(${prior.blocked.args}), call it again with identical arguments. Otherwise do not call it.`
      : 'A previous turn is still open.')
  }
  return lines.join('\n')
}

export async function runCompletion(req: ChatRequest, ctx: LoopContext): Promise<ChatResponse> {
  // The web search request already carries OpenRouter's plugin; tools do not apply.
  if (req.plugins) return ctx.upstream(req)
  const { tools, unavailable, instructions } = await ctx.registry.listTools()
  // Without any usable tool there is nothing to loop over, and an empty tools list is rejected by some providers.
  if (tools.length === 0) return ctx.upstream(req)
  const now = ctx.now ?? Date.now
  const started = now()
  const checkTime = () => {
    if (ctx.signal?.aborted || now() - started > (ctx.deadlineMs ?? 160_000)) throw new TurnTimeoutError()
  }

  const prior = ctx.pending.get(ctx.clientKey)
  const [system, ...rest] = req.messages
  const head: ChatMessage = { role: 'system', content: `${String(system?.content ?? '')}\n\n${systemNote(tools, unavailable, prior, instructions)}` }
  const turn: ChatMessage[] = [...(prior?.messages ?? []), ...rest]
  const specs: ToolSpec[] = tools.map((t) => ({ type: 'function', function: { name: t.fqName, description: t.description, parameters: t.inputSchema } }))
  const byName = new Map(tools.map((t) => [t.fqName, t]))
  const { response_format, ...base } = req

  let blocked: Pending['blocked']
  let approval = prior?.blocked
  let usedTools = false
  for (let step = 0; step < ctx.maxSteps; step++) {
    checkTime()
    const res = await ctx.upstream({ ...base, messages: [head, ...turn], tools: specs })
    checkTime()
    const calls = res.choices[0]?.message.tool_calls ?? []
    if (calls.length === 0) break
    usedTools = true
    turn.push({ role: 'assistant', content: res.choices[0].message.content ?? null, tool_calls: calls })
    for (const call of calls) {
      const tool = byName.get(call.function.name)
      const args = call.function.arguments || '{}'
      ctx.onToolCall?.(call.function.name)
      let content: string
      if (!tool) content = `Unknown tool ${call.function.name}`
      else if (tool.needsConfirm && !(approval?.fqName === tool.fqName && approval.args === args)) {
        blocked = { fqName: tool.fqName, args }
        content = NOT_EXECUTED
      } else {
        checkTime()
        if (tool.needsConfirm) {
          // An approval runs once: spend it before the call, so a failure later in the turn cannot replay it.
          approval = undefined
          if (prior) ctx.pending.set(ctx.clientKey, { messages: prior.messages })
        }
        let parsed: object = {}
        try { parsed = JSON.parse(args) } catch { /* the model sent invalid JSON; the tool sees no arguments */ }
        content = await ctx.registry.call(tool.fqName, parsed)
      }
      turn.push({ role: 'tool', tool_call_id: call.id, content })
    }
  }

  checkTime()
  const final = await ctx.upstream({ ...base, ...(response_format ? { response_format } : {}), messages: [head, ...turn] })
  if (usedTools) {
    ctx.pending.set(ctx.clientKey, { messages: [...turn, { role: 'assistant', content: final.choices[0]?.message.content ?? '' }], blocked })
  } else {
    ctx.pending.clear(ctx.clientKey)
  }
  return final
}
