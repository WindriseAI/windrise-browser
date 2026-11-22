import { RefreshCw } from 'lucide-react'
import { copy } from '../copy'

interface ImportingProgressCardProps {
  currentItemLabel?: string
  progress: number
  sourceLabel?: string
  total: number
}

/** Chromium import progress while the selected source data is copied.
 * The bar is an ink fill in a sunken track; it grows with scaleX (compositor
 * only, 180ms --ease) instead of animating width, and the spinner stops under
 * reduced motion. */
export function ImportingProgressCard({
  currentItemLabel,
  progress,
  sourceLabel,
  total,
}: ImportingProgressCardProps) {
  const fraction = total === 0 ? 0 : Math.min(1, progress / total)
  return (
    <div role="status" aria-live="polite" className="wr-card p-5">
      <div className="mb-3.5 flex items-center gap-2.5">
        <RefreshCw
          className="size-[18px] text-accent motion-safe:animate-spin"
          strokeWidth={1.75}
        />
        <span className="font-medium text-[15px] text-ink">
          {copy.importing(currentItemLabel ?? copy.importingData)}
        </span>
      </div>
      {sourceLabel && (
        <div className="mb-3 font-medium text-[13px] text-ink-2">
          {sourceLabel}
        </div>
      )}
      <div className="mb-2.5 h-2 overflow-hidden rounded-full bg-bg-sunken">
        <div
          className="h-full w-full origin-left rounded-full bg-accent transition-transform duration-180 ease-(--ease) motion-reduce:transition-none"
          style={{ transform: `scaleX(${fraction})` }}
        />
      </div>
      <div className="text-[13px] text-ink-2 tabular-nums">
        {copy.itemsProgress(progress, total)}
      </div>
    </div>
  )
}
