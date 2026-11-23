/**
 * @license
 * Copyright 2025 BrowserOS
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

import { motion, useReducedMotion } from 'motion/react'
import { useEffect, useRef, useState } from 'react'
import { useForm } from 'react-hook-form'
import { Form } from '@/components/ui/form'
import {
  BROWSEROS_ONBOARDING_API_VERSION,
  type BrowserOSImportStatus,
  type BrowserOSOnboardingState,
} from './browseros-onboarding-api'
import { createBrowserOSOnboardingBridge } from './browseros-onboarding-bridge'
import { OnboardingShell } from './components/OnboardingShell'
import {
  importSourceSelectionChangeFor,
  selectedSourceById,
  startImportRequestFor,
} from './onboarding-v2.helpers'
import {
  type OnboardingFormValues,
  onboardingFormDefaults,
  onboardingFormResolver,
} from './onboarding-v2.schemas'
import type { ImportPhase, Step } from './onboarding-v2.types'
import { ImportStep } from './steps/ImportStep'
import { SetupStep } from './steps/SetupStep'
import { WelcomeStep } from './steps/WelcomeStep'

const TOTAL_STEPS = 2

const initialOnboardingState: BrowserOSOnboardingState = {
  apiVersion: BROWSEROS_ONBOARDING_API_VERSION,
  status: 'idle',
  sources: [],
}

/** Maps Chromium importer status into the local three-step onboarding screen state. */
export function importPhaseFor(status: BrowserOSImportStatus): ImportPhase {
  if (status === 'importing') return 'importing'
  if (status === 'failed') return 'failed'
  if (status === 'succeeded') return 'imported'
  return 'picker'
}

/** Runs the standalone three-step BrowserOS onboarding flow. */
export function Onboarding() {
  const reduce = useReducedMotion()
  const form = useForm<OnboardingFormValues>({
    resolver: onboardingFormResolver,
    defaultValues: onboardingFormDefaults,
    mode: 'onChange',
  })

  const [step, setStep] = useState<Step>(0)
  const direction = useRef(1)
  const [bridge] = useState(() => createBrowserOSOnboardingBridge())
  const [onboardingState, setOnboardingState] =
    useState<BrowserOSOnboardingState>(initialOnboardingState)
  const didNotifyPageReady = useRef(false)
  const importPhase = importPhaseFor(onboardingState.status)
  const isFinishing = Boolean(
    onboardingState.setupState && onboardingState.setupState !== 'idle',
  )

  function goTo(next: Step) {
    direction.current = next >= step ? 1 : -1
    setStep(next)
  }

  useEffect(() => {
    // Install first: pageReady may synchronously restore an in-flight setup
    // after reload. Receiving it only renders state; it never sends COMPLETE.
    const cleanup = bridge.registerReceiver(setOnboardingState)
    if (!didNotifyPageReady.current) {
      didNotifyPageReady.current = true
      bridge.pageReady()
    }
    return cleanup
  }, [bridge])

  // Sync the picker form to whatever profiles the native side reports. This is a
  // subscription to an external source (the chrome.send bridge), so it stays an
  // effect rather than derived render state.
  useEffect(() => {
    const currentSourceId = form.getValues('selectedSourceId')
    const selectionChange = importSourceSelectionChangeFor(
      onboardingState.sources,
      currentSourceId,
    )
    if (!selectionChange) return
    if (selectionChange.selectedSourceId !== currentSourceId) {
      form.setValue('selectedSourceId', selectionChange.selectedSourceId, {
        shouldValidate: true,
      })
    }
    if (selectionChange.selectedItems.length === 0) {
      if (form.getValues('selectedItems').length > 0) {
        form.setValue('selectedItems', [], { shouldValidate: true })
      }
      return
    }
    form.setValue('selectedItems', selectionChange.selectedItems, {
      shouldValidate: true,
    })
  }, [form, onboardingState.sources])

  function startImport() {
    const source = selectedSourceById(
      onboardingState.sources,
      form.getValues('selectedSourceId'),
    )
    if (!source) return
    const request = startImportRequestFor(
      source,
      form.getValues('selectedItems'),
    )
    if (!request) return
    bridge.startImport(request)
  }

  // The bridge publishes preparing before sending the finish request and
  // suppresses repeated exits. Chromium alone decides when it is safe to leave.
  function finishOnboarding() {
    bridge.complete()
  }

  return (
    <Form {...form}>
      <OnboardingShell
        step={step}
        totalSteps={TOTAL_STEPS}
        showProgress={!isFinishing}
      >
        {isFinishing ? (
          <SetupStep
            failed={onboardingState.setupState === 'failed'}
            onRetry={() => bridge.retrySetup()}
          />
        ) : (
          <>
            {/* Re-mounts on each step (keyed) and plays a short directional
            slide-in: 12px + fade, nothing else moves (the card outline and the
            rail stay put, so the step change reads as the content turning a
            page, not the window jumping). No exit animation, so the flow can
            never stall waiting on one. Full transform string (not the x
            shorthand) keeps it on the compositor; critically damped spring,
            no overshoot (apple-design §4: damping 1.0, response ~0.3);
            opacity 220ms on the strong ease-out (animate §5). Reduced motion:
            no movement, no fade — the step simply appears. */}
            <motion.div
              key={step}
              className="flex min-h-full flex-1 flex-col"
              initial={
                reduce
                  ? false
                  : {
                      transform: `translateX(${direction.current >= 0 ? 12 : -12}px)`,
                      opacity: 0,
                    }
              }
              animate={{ transform: 'translateX(0px)', opacity: 1 }}
              transition={{
                type: 'spring',
                bounce: 0,
                duration: 0.32,
                opacity: { duration: 0.22, ease: [0.23, 1, 0.32, 1] },
              }}
            >
              {step === 0 && (
                <WelcomeStep
                  onPrimary={() => goTo(1)}
                  onSkip={finishOnboarding}
                />
              )}
              {step === 1 && (
                <ImportStep
                  phase={importPhase}
                  state={onboardingState}
                  form={form}
                  onImport={startImport}
                  onRefresh={() => bridge.refreshSources()}
                  onContinue={finishOnboarding}
                />
              )}
            </motion.div>
          </>
        )}
      </OnboardingShell>
    </Form>
  )
}
