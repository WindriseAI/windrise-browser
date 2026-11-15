/**
 * What the DOM cursor scan learned about one element: why it behaves like a control even
 * though the accessibility tree gives it no interactive role. A plain `reasons` array is
 * accepted too and read as `{ reasons }`, so older callers keep working.
 */
export interface CursorHit {
  /** Signals that made the scan pick the element, e.g. `cursor:pointer` or `onclick`. */
  reasons: string[]
  /** Role to show when the node has none of its own, or an unnamed generic/group role. */
  kind?: string
  /** Synthetic prompt name used when the node has no accessible name. */
  hint?: string
}

/** The form a hit arrives in; the scan may pass either shape. */
export type CursorHitInput = string[] | CursorHit

/**
 * Roles the scan can promote an unnamed node to. The accessibility tree cannot find these
 * nodes by role and name, so a stale ref on one is a miss, not a re-query.
 */
export const PROMOTED_ROLES: Readonly<Record<string, true>> = {
  clickable: true,
  image: true,
  icon: true,
}

/** Normalizes either hit shape to the object form. */
export function cursorHit(
  raw: CursorHitInput | undefined,
): CursorHit | undefined {
  if (raw === undefined) return undefined
  return Array.isArray(raw) ? { reasons: raw } : raw
}
