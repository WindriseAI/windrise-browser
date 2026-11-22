import { cn } from '@/lib/utils'
import { copy } from '../copy'

interface StepDotsProps {
  step: number
  total: number
}

/** Compact step progress: the current step is a short ink pill, done steps are
 * ink 40%, the rest hairline grey. It changes instantly — the step content
 * already slides in, and animating the pill width would be a layout animation. */
export function StepDots({ step, total }: StepDotsProps) {
  return (
    <div
      className="flex items-center gap-[7px]"
      role="progressbar"
      aria-label={copy.progressLabel}
      aria-valuemin={1}
      aria-valuemax={total}
      aria-valuenow={step + 1}
    >
      {Array.from({ length: total }).map((_, idx) => {
        const isActive = idx === step
        const isDone = idx < step
        return (
          <span
            // biome-ignore lint/suspicious/noArrayIndexKey: dots are pure decoration with a fixed total; no DOM identity to preserve
            key={idx}
            data-step-dot="true"
            aria-hidden
            className={cn(
              'h-[7px] rounded-full',
              isActive ? 'w-[22px] bg-ink' : 'w-[7px]',
              !isActive && isDone && 'bg-ink/40',
              !isActive && !isDone && 'bg-ink/15',
            )}
          />
        )
      })}
    </div>
  )
}
