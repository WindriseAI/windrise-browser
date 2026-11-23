import { ArrowRight } from 'lucide-react'
import { Button } from '@/components/ui/button'
import { DisplayHeading, Em, StepCopy } from '../components/DisplayHeading'
import { StepWrap } from '../components/StepWrap'
import { copy } from '../copy'

interface WelcomeStepProps {
  onPrimary: () => void
  onSkip: () => void
}

/** Opening step: names the two things onboarding will do, then gets out of the
 * way. One raised white-glass button to press; skipping is a quiet text button.
 * It sits in the vertical middle of the card's step column (my-auto in the
 * scrolling flex column), so the short step never leaves a hole at the bottom. */
export function WelcomeStep({ onPrimary, onSkip }: WelcomeStepProps) {
  return (
    <StepWrap className="my-auto max-w-[460px] pb-10">
      <DisplayHeading>
        {copy.welcomeTitleLead}
        <Em>{copy.welcomeTitleAccent}</Em>
        {copy.welcomeTitleTail}
      </DisplayHeading>
      <StepCopy>{copy.welcomeBody}</StepCopy>
      <div className="flex flex-wrap items-center gap-3">
        <Button type="button" size="lg" onClick={onPrimary}>
          {copy.getStarted}
          <ArrowRight className="size-4" />
        </Button>
        <Button type="button" size="lg" variant="ghost" onClick={onSkip}>
          {copy.skipForNow}
        </Button>
      </div>
    </StepWrap>
  )
}
