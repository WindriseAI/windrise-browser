import { useEffect } from 'react'
import { stepKeyActionFor } from './onboarding-v2.helpers'

interface StepKeyHandlers {
  /** The step's main action (the one raised button). */
  onPrimary?: () => void
  /** Step back, when the screen offers a way back. */
  onBack?: () => void
}

/** Page-level Enter / Escape for one onboarding step. It never moves focus,
 * so no focus ring appears on load; a focused button still answers Enter
 * itself. Handled keys are consumed (preventDefault) so the same press cannot
 * also activate a control that appears underneath it, such as the Keychain
 * dialog's Continue. Pass `enabled = false` while a dialog owns the keys. */
export function useStepKeys(
  { onPrimary, onBack }: StepKeyHandlers,
  enabled = true,
) {
  useEffect(() => {
    if (!enabled || typeof window === 'undefined') return
    function onKeyDown(event: KeyboardEvent) {
      const action = stepKeyActionFor(event)
      const run =
        action === 'primary' ? onPrimary : action === 'back' ? onBack : null
      if (!run) return
      event.preventDefault()
      run()
    }
    window.addEventListener('keydown', onKeyDown)
    return () => window.removeEventListener('keydown', onKeyDown)
  }, [enabled, onPrimary, onBack])
}
