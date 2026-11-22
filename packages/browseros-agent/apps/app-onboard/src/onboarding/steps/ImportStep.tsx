import {
  AlertTriangle,
  ArrowRight,
  ChevronDown,
  Download,
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
  importItemLabel,
  importItemListLabel,
  importProgressTotal,
  isMacOS,
  needsMacKeychainPermission,
  sanitizeImportSelection,
  selectedSourceById,
} from '../onboarding-v2.helpers'
import type { OnboardingFormValues } from '../onboarding-v2.schemas'
import type { ImportPhase } from '../onboarding-v2.types'

interface ImportStepProps {
  phase: ImportPhase
  state: BrowserOSOnboardingState
  form: UseFormReturn<OnboardingFormValues>
  onImport: () => void
  onRefresh: () => void
  onContinue: () => void
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

  return copy.copyFrom(browserName.replace(/^Google /, ''))
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
}: ImportStepProps) {
  const [permissionOpen, setPermissionOpen] = useState(false)
  const [showAllProfiles, setShowAllProfiles] = useState(false)
  const importButtonRef = useRef<HTMLButtonElement>(null)
  const profileListId = useId()
  const selectedSourceId = form.watch('selectedSourceId')
  const selectedSource = selectedSourceById(state.sources, selectedSourceId)
  const sourceResult = state.results?.[0]
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
  const sourceName =
    sourceResult?.displayName ||
    selectedSource?.profileName ||
    selectedSource?.browserName ||
    copy.sourceFallback
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
  const currentSourceLabel = state.progress?.currentSourceName
  const importedItems = state.progress?.completedItems ?? []
  const importedItemSummary = state.progress
    ? importedItems.length
      ? importItemListLabel(importedItems)
      : copy.noCompletedItems
    : copy.noItemDetails
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
    onImport()
  }

  function confirmImport() {
    setPermissionOpen(false)
    // Do not defer to an effect or animation: native import needs the recent
    // Continue gesture. The bridge owns the pending lock until native replies.
    if (isPickerValid && (phase === 'picker' || phase === 'failed')) onImport()
  }

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
    <StepWrap className="max-w-[800px]">
      <DisplayHeading>
        {copy.importTitleLead}
        <Em>{copy.importTitleAccent}</Em>
        {copy.importTitleTail}
      </DisplayHeading>
      <StepCopy>{copy.importBody}</StepCopy>

      {phase === 'picker' && (
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
              {state.error.message}
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
              {selectedSource?.profileName && (
                <span className="text-ink-3 text-xs">
                  {copy.fromProfile(selectedSource.profileName)}
                </span>
              )}
            </div>
          </div>
        </>
      )}

      {phase === 'importing' && (
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

      {phase === 'failed' && (
        <>
          <div className="wr-sunken-alert mb-4 p-4">
            <div className="mb-2 flex items-center gap-2 font-medium text-[14px] text-ink">
              <AlertTriangle className="size-4 text-amber" strokeWidth={1.75} />
              {copy.somethingWentWrong}
            </div>
            <div className="text-[13px] text-ink-2">
              {state.error?.message ?? copy.importFailedFallback}
            </div>
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
            <Button
              type="button"
              size="lg"
              variant="outline"
              onClick={onRefresh}
            >
              <RefreshCw className="size-4" />
              {copy.refreshProfiles}
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

      {phase === 'imported' && (
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
