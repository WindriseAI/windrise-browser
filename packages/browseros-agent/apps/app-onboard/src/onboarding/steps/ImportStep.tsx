import {
  AlertTriangle,
  ArrowRight,
  ChevronDown,
  ChevronLeft,
  Download,
  ListChecks,
  Lock,
  RefreshCw,
} from 'lucide-react'
import { useId, useRef, useState } from 'react'
import type { UseFormReturn } from 'react-hook-form'
import { Button } from '@/components/ui/button'
import { FormField, FormItem, FormMessage } from '@/components/ui/form'
import type {
  BrowserOSImportItem,
  BrowserOSImportSource,
  BrowserOSOnboardingState,
} from '../browseros-onboarding-api'
import { DisplayHeading, Em, StepCopy } from '../components/DisplayHeading'
import { ImportedSummaryCard } from '../components/ImportedSummaryCard'
import { ImportItemChecklist } from '../components/ImportItemChecklist'
import { ImportingProgressCard } from '../components/ImportingProgressCard'
import { ImportSourceTile } from '../components/ImportSourceTile'
import { MacKeychainDialog } from '../components/MacKeychainDialog'
import { MacKeychainReminder } from '../components/MacKeychainNotice'
import { StepWrap } from '../components/StepWrap'
import { copy } from '../copy'
import {
  completedImportItemCount,
  defaultImportItemsForSource,
  importFailureMessageFor,
  importItemLabel,
  importItemListLabel,
  importProgressTotal,
  importSourceLabels,
  importSourceName,
  isMacOS,
  needsMacKeychainPermission,
  sanitizeImportSelection,
  selectedSourceById,
  shortBrowserName,
} from '../onboarding-v2.helpers'
import type { OnboardingFormValues } from '../onboarding-v2.schemas'
import type { ImportPhase } from '../onboarding-v2.types'
import { useStepKeys } from '../useStepKeys'

interface ImportStepProps {
  phase: ImportPhase
  state: BrowserOSOnboardingState
  form: UseFormReturn<OnboardingFormValues>
  onImport: () => void
  onRefresh: () => void
  onContinue: () => void
  /** Back to the welcome step (plays the existing reverse step slide). */
  onBack: () => void
}

function importButtonLabelFor(
  hasSource: boolean,
  hasSupportedItems: boolean,
  checkedItems: readonly BrowserOSImportItem[],
  browserName: string,
): string {
  if (!hasSource) return copy.pickProfile
  if (!hasSupportedItems) return copy.nothingToCopy
  if (checkedItems.length === 0) return copy.selectWhatToCopy

  return copy.copyFrom(shortBrowserName(browserName))
}

/** 「来自「Work」」 for a named profile, 「来自「默认账号」」 for a placeholder
 * (never "From Default profile’s profile"). */
function fromLabelFor(source: BrowserOSImportSource | undefined) {
  if (!source) return undefined
  const labels = importSourceLabels(source)
  return labels.isPlaceholder
    ? copy.fromAccount(labels.profileLabel)
    : copy.fromProfile(labels.profileLabel)
}

/** Chromium's joined names ("Microsoft Edge - Default") never reach the
 * screen for a placeholder profile: the summary and progress cards look the
 * source up by id instead. */
function sourceNamesFor(
  state: BrowserOSOnboardingState,
  selectedSource: BrowserOSImportSource | undefined,
) {
  const sourceResult = state.results?.[0]
  const resultSource = sourceResult
    ? selectedSourceById(state.sources, sourceResult.sourceId)
    : selectedSource
  const progressSourceId = state.progress?.currentSourceId
  return {
    sourceName:
      importSourceName(resultSource, sourceResult?.displayName) ||
      copy.sourceFallback,
    currentSourceLabel: importSourceName(
      progressSourceId
        ? selectedSourceById(state.sources, progressSourceId)
        : undefined,
      state.progress?.currentSourceName,
    ),
  }
}

/** Never falls back to the selected items: only what Chromium reports done. */
function importedSummaryFor(state: BrowserOSOnboardingState) {
  if (!state.progress) return copy.noItemDetails
  const importedItems = state.progress.completedItems
  return importedItems.length
    ? importItemListLabel(importedItems)
    : copy.noCompletedItems
}

/** Enter = the raised button of this screen, Escape = back (picker only). */
function stepKeysFor(
  view: ImportPhase,
  actions: {
    requestImport: () => void
    onContinue: () => void
    onBack: () => void
  },
) {
  if (view === 'picker')
    return { onPrimary: actions.requestImport, onBack: actions.onBack }
  if (view === 'failed') return { onPrimary: actions.requestImport }
  if (view === 'imported') return { onPrimary: actions.onContinue }
  return {}
}

/** Collapsing the list must not hide the profile whose data will be copied. */
function visibleProfiles(
  sources: readonly BrowserOSImportSource[],
  selectedSource: BrowserOSImportSource | undefined,
  expanded: boolean,
) {
  if (expanded) return sources
  const visible = sources.slice(0, 4)
  if (selectedSource && !visible.includes(selectedSource))
    visible[3] = selectedSource
  return visible
}

export function ImportStep({
  phase,
  state,
  form,
  onImport,
  onRefresh,
  onContinue,
  onBack,
}: ImportStepProps) {
  const [permissionOpen, setPermissionOpen] = useState(false)
  const [showAllProfiles, setShowAllProfiles] = useState(false)
  // 「重新选择」 after a failed import shows the picker again without asking
  // Chromium for anything: the form (profile + ticked items) lives in
  // Onboarding and the failure snapshot still carries the sources, so the
  // choice is exactly as the user left it and the error stays on screen above
  // the action bar. Starting the next import clears it, so a second failure
  // lands on the failure screen again.
  const [repicking, setRepicking] = useState(false)
  const view: ImportPhase = phase === 'failed' && repicking ? 'picker' : phase
  const importButtonRef = useRef<HTMLButtonElement>(null)
  const profileListId = useId()
  const selectedSourceId = form.watch('selectedSourceId')
  const selectedSource = selectedSourceById(state.sources, selectedSourceId)
  const checkedItems = selectedSource
    ? sanitizeImportSelection(selectedSource, form.watch('selectedItems'))
    : []
  const needsPermission = needsMacKeychainPermission(
    isMacOS(),
    selectedSource,
    checkedItems,
  )
  const visibleSources = visibleProfiles(
    state.sources,
    selectedSource,
    showAllProfiles,
  )
  const fromLabel = fromLabelFor(selectedSource)
  const { sourceName, currentSourceLabel } = sourceNamesFor(
    state,
    selectedSource,
  )
  const failedSource =
    (state.results?.[0]
      ? selectedSourceById(state.sources, state.results[0].sourceId)
      : undefined) ?? selectedSource
  const importErrorMessage = importFailureMessageFor(state.error, failedSource)
  const isDetecting = state.status === 'detecting'
  const hasNoProfiles = !isDetecting && state.sources.length === 0
  const hasSupportedItems = (selectedSource?.supportedItems.length ?? 0) > 0
  const isPickerValid =
    Boolean(selectedSource) &&
    hasSupportedItems &&
    checkedItems.length > 0 &&
    !isDetecting
  const completedItems = completedImportItemCount(state.progress)
  const totalItems = selectedSource
    ? importProgressTotal(checkedItems.length, state.progress)
    : (state.progress?.totalItems ?? 0)
  const currentItemLabel = state.progress?.currentItem
    ? importItemLabel(state.progress.currentItem)
    : undefined
  const importedItemSummary = importedSummaryFor(state)
  const importButtonLabel = importButtonLabelFor(
    Boolean(selectedSource),
    hasSupportedItems,
    checkedItems,
    selectedSource?.browserName ?? copy.browserFallback,
  )

  function requestImport() {
    if (!isPickerValid) return
    if (needsPermission) {
      setPermissionOpen(true)
      return
    }
    setRepicking(false)
    onImport()
  }

  function confirmImport() {
    setPermissionOpen(false)
    // Do not defer to an effect or animation: native import needs the recent
    // Continue gesture. The bridge owns the pending lock until native replies.
    if (isPickerValid && (phase === 'picker' || phase === 'failed')) {
      setRepicking(false)
      onImport()
    }
  }

  // Enter = the raised button of this screen, Escape = back (picker only).
  // The Keychain dialog owns the keys while it is open.
  useStepKeys(
    stepKeysFor(view, { requestImport, onContinue, onBack }),
    !permissionOpen,
  )

  function toggleImportItem(item: BrowserOSImportItem) {
    if (!selectedSource) return
    const currentItems = sanitizeImportSelection(
      selectedSource,
      form.getValues('selectedItems'),
    )
    const nextItems = currentItems.includes(item)
      ? currentItems.filter((currentItem) => currentItem !== item)
      : sanitizeImportSelection(selectedSource, [...currentItems, item])
    form.setValue('selectedItems', nextItems, {
      shouldDirty: true,
      shouldValidate: true,
    })
  }

  return (
    <StepWrap className="max-w-[560px]">
      <DisplayHeading>
        {copy.importTitleLead}
        <Em>{copy.importTitleAccent}</Em>
        {copy.importTitleTail}
      </DisplayHeading>
      <StepCopy>{copy.importBody}</StepCopy>

      {view === 'picker' && (
        <>
          <div className="mb-2.5 flex items-center justify-between gap-3">
            <div className="font-medium text-[13px] text-ink-2">
              {isDetecting ? copy.lookingForProfiles : copy.pickProfile}
            </div>
            <Button
              type="button"
              size="sm"
              variant="ghost"
              onClick={onRefresh}
              disabled={isDetecting}
            >
              <RefreshCw
                className={`size-3.5 ${isDetecting ? 'motion-safe:animate-spin' : ''}`}
              />
              {copy.refresh}
            </Button>
          </div>
          <FormField
            control={form.control}
            name="selectedSourceId"
            render={({ field }) => (
              <FormItem
                id={profileListId}
                className="mb-3 flex flex-col gap-2"
                role="radiogroup"
                aria-label={copy.browserProfiles}
              >
                {visibleSources.map((source) => (
                  <ImportSourceTile
                    key={source.id}
                    source={source}
                    selected={field.value === source.id}
                    onSelect={() => {
                      if (field.value === source.id) return
                      field.onChange(source.id)
                      form.setValue(
                        'selectedItems',
                        defaultImportItemsForSource(source),
                        {
                          shouldDirty: true,
                          shouldValidate: true,
                        },
                      )
                    }}
                  />
                ))}
                {hasNoProfiles && (
                  <div className="wr-sunken p-4 text-[13px] text-ink-2">
                    {copy.noProfiles}
                  </div>
                )}
                {/* Only ask for a pick when there is one to make. With no
                    sources the error is self-inflicted and unactionable:
                    OnboardingV2 clears selectedSourceId with shouldValidate
                    whenever the list is empty, including while detecting. */}
                {state.sources.length > 0 && <FormMessage />}
              </FormItem>
            )}
          />
          {state.sources.length > 4 && (
            <Button
              type="button"
              variant="ghost"
              size="sm"
              className="mb-4 text-ink-2"
              aria-expanded={showAllProfiles}
              aria-controls={profileListId}
              onClick={() => setShowAllProfiles(!showAllProfiles)}
            >
              <ChevronDown
                aria-hidden
                className={`size-3.5 ${showAllProfiles ? 'rotate-180' : ''}`}
              />
              {showAllProfiles
                ? copy.showFewerProfiles
                : copy.showMoreProfiles(state.sources.length - 4)}
            </Button>
          )}
          {selectedSource && hasSupportedItems && (
            <ImportItemChecklist
              items={selectedSource.supportedItems}
              checkedItems={checkedItems}
              onToggle={toggleImportItem}
            />
          )}
          {state.error && (
            <div className="wr-sunken-alert mb-4 p-4 text-[13px] text-ink">
              {importErrorMessage}
            </div>
          )}
          <div className="wr-action-bar sticky -bottom-6 z-10 pt-6 pb-4">
            {needsPermission && (
              <p className="mb-3 flex items-center gap-2 text-ink-3 text-xs">
                <Lock aria-hidden className="size-3.5" />
                {copy.macPasswordHint}
              </p>
            )}
            <div className="flex flex-wrap items-center gap-3">
              {/* Back sits left of the main action, where both the Windows
                  wizard and the macOS setup assistant put it; it is a quiet
                  text button so the raised one stays the only bright one. */}
              <Button type="button" size="lg" variant="ghost" onClick={onBack}>
                <ChevronLeft aria-hidden className="size-4" />
                {copy.back}
              </Button>
              <Button
                type="button"
                size="lg"
                ref={importButtonRef}
                onClick={requestImport}
                disabled={!isPickerValid}
              >
                <Download className="size-4" />
                {importButtonLabel}
              </Button>
              <Button
                type="button"
                size="lg"
                variant="ghost"
                onClick={onContinue}
              >
                {copy.skipForNow}
              </Button>
              {fromLabel && (
                <span className="text-ink-3 text-xs">{fromLabel}</span>
              )}
            </div>
          </div>
        </>
      )}

      {view === 'importing' && (
        <>
          <ImportingProgressCard
            currentItemLabel={currentItemLabel}
            progress={completedItems}
            sourceLabel={currentSourceLabel}
            total={totalItems}
          />
          {needsPermission && <MacKeychainReminder />}
        </>
      )}

      {view === 'failed' && (
        <>
          <div className="wr-sunken-alert mb-4 p-4">
            <div className="mb-2 flex items-center gap-2 font-medium text-[14px] text-ink">
              <AlertTriangle className="size-4 text-amber" strokeWidth={1.75} />
              {copy.somethingWentWrong}
            </div>
            <div className="text-[13px] text-ink-2">{importErrorMessage}</div>
          </div>
          <div className="flex flex-wrap gap-2.5">
            <Button
              type="button"
              size="lg"
              ref={importButtonRef}
              onClick={requestImport}
              disabled={!isPickerValid}
            >
              <Download className="size-4" />
              {copy.tryAgain}
            </Button>
            {/* Back to the picker with the same profile and items, so a
                locked Chrome can be swapped for Edge or the locked items
                unticked. Refresh lives in the picker header. */}
            <Button
              type="button"
              size="lg"
              variant="outline"
              onClick={() => setRepicking(true)}
            >
              <ListChecks aria-hidden className="size-4" />
              {copy.chooseAgain}
            </Button>
            <Button
              type="button"
              size="lg"
              variant="ghost"
              onClick={onContinue}
            >
              {copy.skipForNow}
            </Button>
          </div>
        </>
      )}

      {view === 'imported' && (
        <>
          <ImportedSummaryCard
            importedItemCount={completedItems}
            itemSummary={importedItemSummary}
            sourceName={sourceName}
          />
          <Button type="button" size="lg" onClick={onContinue}>
            {copy.continue}
            <ArrowRight className="size-4" />
          </Button>
        </>
      )}
      <MacKeychainDialog
        open={
          permissionOpen &&
          isPickerValid &&
          (phase === 'picker' || phase === 'failed')
        }
        onOpenChange={setPermissionOpen}
        onContinue={confirmImport}
        returnFocus={importButtonRef}
      />
    </StepWrap>
  )
}
