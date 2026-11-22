import { CircleAlert, LoaderCircle } from 'lucide-react'
import { Button } from '@/components/ui/button'
import { DisplayHeading, StepCopy } from '../components/DisplayHeading'
import { copy } from '../copy'

interface SetupStepProps {
  failed: boolean
  onRetry: () => void
}

/** Holds the browser-owned WebUI open while native prepares the product extension.
 * Native owns success navigation; this view only offers recovery after failure.
 * Status marks are ink (no red / amber); the spinner stops under reduced motion. */
export function SetupStep({ failed, onRetry }: SetupStepProps) {
  return (
    <div className="flex flex-1 items-center">
      <div className="w-full max-w-[560px]">
        <div role={failed ? 'alert' : 'status'} aria-atomic="true">
          {failed ? (
            <CircleAlert
              aria-hidden="true"
              className="mb-6 size-8 text-accent"
              strokeWidth={1.75}
            />
          ) : (
            <LoaderCircle
              aria-hidden="true"
              className="mb-6 size-8 text-accent motion-safe:animate-spin"
              strokeWidth={1.75}
            />
          )}
          <DisplayHeading>
            {failed ? copy.setupFailedTitle : copy.setupTitle}
          </DisplayHeading>
          <StepCopy>{failed ? copy.setupFailedBody : copy.setupBody}</StepCopy>
        </div>
        {failed && (
          <Button type="button" size="lg" onClick={onRetry}>
            {copy.retry}
          </Button>
        )}
      </div>
    </div>
  )
}
