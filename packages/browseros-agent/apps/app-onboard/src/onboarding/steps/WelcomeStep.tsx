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
 * way. One raised white-glass button to press; skipping is a quiet text button. */
export function WelcomeStep({ onPrimary, onSkip }: WelcomeStepProps) {
  return (
    <StepWrap>
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
