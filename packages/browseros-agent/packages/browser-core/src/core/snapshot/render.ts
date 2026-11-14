import type { AXNode } from './ax-types'
import { type CursorHit, type CursorHitInput, cursorHit } from './cursor-hits'
import type { DocumentId, FrameId, RefMap } from './refs'
import { INTERACTIVE_ROLES, ROOT_ROLES, SKIP_ROLES, VALUE_ROLES } from './roles'

const IFRAME_ROLES: ReadonlySet<string> = new Set(['Iframe', 'iframe'])

/** Unnamed containers a cursor hit may replace, so the line says how the element behaves. */
const PROMOTABLE_ROLES: Readonly<Record<string, true>> = {
  generic: true,
  group: true,
  none: true,
  presentation: true,
}

export interface IframeStitch {
  /** The iframe element, used to resolve its child frameId for stitching. */
  backendNodeId: number
  /** Index of the `- iframe` line within this render's output, for splicing child content. */
  lineIndex: number
  /** Absolute indent depth of the iframe line. */
  depth: number
}

export interface RenderResult {
  text: string
  iframes: IframeStitch[]
}

export interface RenderOptions {
  /** Shared across all frames of a page so refs form one global namespace. */
  refs: RefMap
  frameId?: FrameId
  documentId?: DocumentId
  /** backendDOMNodeId → hit, from the DOM cursor-augmentation pass. */
  cursorHits?: Map<number, CursorHitInput>
  /** Extra indent levels to prepend (used when splicing a child frame under its iframe line). */
  baseDepth?: number
}

/** Renders a CDP accessibility tree into the canonical agent-facing snapshot. */
export function renderSnapshot(
  nodes: AXNode[],
  opts: RenderOptions,
): RenderResult {
  const byId = new Map<string, AXNode>()
  for (const node of nodes) byId.set(node.nodeId, node)
  const cardForms = cardFormScopes(nodes)

  const base = opts.baseDepth ?? 0
  const lines: string[] = []
  const iframes: IframeStitch[] = []

  const visit = (nodeId: string, depth: number, inCardForm: boolean): void => {
    const node = byId.get(nodeId)
    if (!node) return
    inCardForm = inCardForm || cardForms.has(nodeId)

    const hit = hitFor(node, opts)
    const role = promoteRole(node.ignored ? undefined : strVal(node.role), hit)
    const name = strVal(node.name)

    if (isDropped(role, name, hit)) {
      for (const childId of node.childIds ?? []) visit(childId, depth, inCardForm)
      return
    }

    if (role && IFRAME_ROLES.has(role)) {
      let line = `${'  '.repeat(base + depth)}- iframe`
      if (name) line += ` ${JSON.stringify(name)}`
      lines.push(line)
      if (node.backendDOMNodeId !== undefined) {
        iframes.push({
          backendNodeId: node.backendDOMNodeId,
          lineIndex: lines.length - 1,
          depth: base + depth,
        })
      }
      return
    }

    lines.push(formatLine(node, role as string, name, base + depth, opts, hit, inCardForm))
    for (const childId of node.childIds ?? []) visit(childId, depth + 1, inCardForm)
  }

  for (const rootId of entryNodeIds(nodes)) visit(rootId, 0, false)

  return { text: lines.join('\n'), iframes }
}

/** Where to begin the walk: document roots if present, else the first node. */
function entryNodeIds(nodes: AXNode[]): string[] {
  const roots = nodes
    .filter((n) => ROOT_ROLES.has(strVal(n.role)))
    .map((n) => n.nodeId)
  if (roots.length > 0) return roots
  return nodes[0] ? [nodes[0].nodeId] : []
}

function hitFor(node: AXNode, opts: RenderOptions): CursorHit | undefined {
  if (node.backendDOMNodeId === undefined) return undefined
  return cursorHit(opts.cursorHits?.get(node.backendDOMNodeId))
}

/**
 * An ignored node has no role, so a cursor hit supplies the one the renderer would otherwise
 * drop it for. The same applies to an unnamed generic wrapper, where `- generic [ref=e5]` says
 * nothing about whether the element can be clicked.
 */
function promoteRole(
  role: string | undefined,
  hit: CursorHit | undefined,
): string | undefined {
  if (!hit?.kind) return role
  if (!role) return hit.kind
  return PROMOTABLE_ROLES[role] === true ? hit.kind : role
}

function isDropped(
  role: string | undefined,
  name: string,
  hit: CursorHit | undefined,
): boolean {
  if (!role) return true
  if (SKIP_ROLES.has(role) || ROOT_ROLES.has(role)) return true
  // Unnamed generic containers carry no meaning unless they're cursor-interactive.
  if ((role === 'generic' || role === 'group') && !name && !hit) {
    return true
  }
  return false
}

function formatLine(
  node: AXNode,
  role: string,
  name: string,
  depth: number,
  opts: RenderOptions,
  hit: CursorHit | undefined,
  inCardForm: boolean,
): string {
  let line = `${'  '.repeat(depth)}- ${role}`
  if (name) line += ` ${JSON.stringify(name)}`

  for (const state of formatStates(node)) line += ` [${state}]`

  const backendNodeId = node.backendDOMNodeId
  // Only nodes that already have an interactive role or a cursor hit earn a ref. Promoted
  // kinds alone do not qualify: `image` and `icon` are real accessibility roles too, and a
  // ref on every plain <img> would flood the snapshot.
  const actionable =
    backendNodeId !== undefined &&
    (INTERACTIVE_ROLES.has(role) || hit !== undefined)

  if (actionable) {
    const ref = opts.refs.mint({
      backendNodeId: backendNodeId as number,
      role,
      name,
      documentId: opts.documentId,
      frameId: opts.frameId,
    })
    line += ` [ref=${ref}]`
  }
  if (hit !== undefined) line += ' [cursor=pointer]'

  // A synthetic prompt name only fills a gap; a real accessible name stays in quotes.
  if (hit?.hint && !name) line += ` (hint: ${hit.hint})`

  if (VALUE_ROLES.has(role)) {
    const value = strVal(node.value)
    if (value) line += `: ${JSON.stringify(visibleValue(name, value, inCardForm))}`
  }

  return line
}

// Port of BrowserOS 6e65fb395 snapshot/render.rs. Password values are already
// masked by the browser's AX tree; card fields are ordinary text inputs.
function visibleValue(name: string, value: string, inCardForm: boolean): string {
  const digits = digitRun(value)
  if (digits === undefined) return value
  if (isCardNumber(digits)) return `•••• ${digits.slice(-4)}`
  const label = name.replace(/[A-Z]/g, (char) => char.toLowerCase())
  if (digits.length >= 3 && digits.length <= 4 &&
      (inCardForm || ['cvc', 'cvv', 'csc', 'security code', 'card code', 'verification']
        .some((term) => label.includes(term)))) {
    return '•••'
  }
  return value
}

/** Only ASCII digits, spaces and dashes, exactly as in the upstream rule. */
function digitRun(value: string): string | undefined {
  return /^[0-9 -]*$/.test(value) ? value.replace(/[ -]/g, '') : undefined
}

function isCardNumber(digits: string): boolean {
  if (digits.length < 13 || digits.length > 19) return false
  let sum = 0
  for (let i = digits.length - 1; i >= 0; i--) {
    let digit = digits.charCodeAt(i) - 48
    if ((digits.length - 1 - i) % 2 === 1) {
      digit *= 2
      if (digit > 9) digit -= 9
    }
    sum += digit
  }
  return sum % 10 === 0
}

/** Nearest form ancestor of a card value, or its tree root on formless pages. */
function cardFormScopes(nodes: AXNode[]): Set<string> {
  const parents = new Map<string, string>()
  const roles = new Map<string, string>()
  for (const node of nodes) {
    roles.set(node.nodeId, strVal(node.role))
    for (const child of node.childIds ?? []) parents.set(child, node.nodeId)
  }
  const scopes = new Set<string>()
  for (const node of nodes) {
    const digits = digitRun(strVal(node.value))
    if (digits === undefined || !isCardNumber(digits)) continue
    let scope = node.nodeId
    // Bound malformed ancestor cycles just as upstream does.
    for (let i = 0; i < nodes.length; i++) {
      const parent = parents.get(scope)
      if (parent === undefined) break
      scope = parent
      if (roles.get(scope) === 'form') break
    }
    scopes.add(scope)
  }
  return scopes
}

function formatStates(node: AXNode): string[] {
  const states: string[] = []
  for (const prop of node.properties ?? []) {
    const v = prop.value.value
    switch (prop.name) {
      case 'checked':
        if (v === true) states.push('checked')
        else if (v === 'mixed') states.push('indeterminate')
        break
      case 'disabled':
        if (v === true) states.push('disabled')
        break
      case 'expanded':
        if (v === true) states.push('expanded')
        else if (v === false) states.push('collapsed')
        break
      case 'required':
        if (v === true) states.push('required')
        break
      case 'selected':
        if (v === true) states.push('selected')
        break
      case 'level':
        states.push(`level=${v}`)
        break
      default:
        break
    }
  }
  return states
}

function strVal(value: AXNode['role']): string {
  return typeof value?.value === 'string' ? value.value : ''
}
