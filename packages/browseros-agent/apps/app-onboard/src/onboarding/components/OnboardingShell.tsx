import { type ReactNode, useEffect, useRef } from 'react'
import { copy } from '../copy'
import { StepDots } from './StepDots'
import { VisualRail } from './VisualRail'

interface OnboardingShellProps {
  step: number
  totalSteps: number
  showProgress?: boolean
  children: ReactNode
}

/** One liquid-glass card in the middle of the window (Opus-2, 11-23).
 *
 * The first-run window is 960×640 (profile_picker_view.cc patch), clamped to
 * the display work area; the card is at most 920×540 and always keeps 24px of
 * page around it, so nothing can reach the window's bottom edge or slide
 * under the Windows taskbar (B11②: 「数据留在本机。」 was cut at y=1032).
 * The card has a fixed height: the short welcome step and the tall import
 * step share the same outline, and only the right column scrolls. The visual
 * rail is the card's left column from 768px up; below that the brand moves to
 * the top of the step column so the B6 mark never disappears. */
export function OnboardingShell({
  step,
  totalSteps,
  showProgress = true,
  children,
}: OnboardingShellProps) {
  const scrollRef = useRef<HTMLDivElement>(null)

  // A new step always starts at its top (the previous step may have scrolled).
  useEffect(() => {
    if (step >= 0 && scrollRef.current) scrollRef.current.scrollTop = 0
  }, [step])

  return (
    <div className="flex h-dvh min-h-0 items-center justify-center overflow-hidden p-6">
      <div className="wr-stage-card relative flex h-[min(540px,calc(100dvh-48px))] w-full max-w-[920px] overflow-hidden">
        <VisualRail />
        <main className="flex min-h-0 min-w-0 flex-1 flex-col">
          <div className="flex items-center gap-2.5 px-6 pt-6 sm:px-10 md:hidden">
            {/* Same literal public path as the rail (verify-chromium-build.ts
                allowlist). Only shown when the rail is hidden. */}
            <img
              alt=""
              aria-hidden
              className="size-6 rounded-[7px]"
              src="/icon/128.png"
            />
            <div className="font-medium text-[15px] text-ink">
              {copy.brandName}
            </div>
          </div>
          {showProgress && (
            <div className="px-6 pt-6 sm:px-10 md:pt-9">
              <StepDots step={step} total={totalSteps} />
            </div>
          )}
          <div
            ref={scrollRef}
            className="wr-scroll flex min-h-0 flex-1 flex-col overflow-y-auto overflow-x-hidden px-6 pt-6 pb-6 sm:px-10"
          >
            {children}
          </div>
        </main>
      </div>
    </div>
  )
}
