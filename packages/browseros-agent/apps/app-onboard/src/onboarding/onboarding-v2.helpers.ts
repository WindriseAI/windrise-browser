/**
 * @license
 * Copyright 2025 BrowserOS
 * SPDX-License-Identifier: AGPL-3.0-or-later
 */

import type {
  BrowserOSImportItem,
  BrowserOSImportProgress,
  BrowserOSImportSource,
  BrowserOSStartImportRequest,
} from './browseros-onboarding-api'
import { copy } from './copy'

export const MOCK_BROWSEROS_IMPORT_SOURCES: readonly BrowserOSImportSource[] = [
  {
    id: 'chrome-work',
    displayName: 'Google Chrome - Work',
    browserName: 'Google Chrome',
    profileName: 'Work',
    accountName: 'work@example.com',
    isManaged: true,
    supportedItems: [
      'history',
      'bookmarks',
      'cookies',
      'passwords',
      'searchEngines',
      'autofill',
      'extensions',
    ],
    recommendedItems: [
      'history',
      'bookmarks',
      'cookies',
      'passwords',
      'searchEngines',
      'autofill',
      'extensions',
    ],
  },
  {
    id: 'chrome-personal',
    displayName: 'Google Chrome - Personal',
    browserName: 'Google Chrome',
    profileName: 'Personal',
    accountName: 'personal@example.com',
    isManaged: false,
    supportedItems: [
      'history',
      'bookmarks',
      'cookies',
      'passwords',
      'autofill',
    ],
    recommendedItems: ['history', 'bookmarks', 'cookies', 'passwords'],
  },
  {
    id: 'edge-default',
    displayName: 'Microsoft Edge - Default',
    browserName: 'Microsoft Edge',
    profileName: 'Default',
    accountName: '',
    isManaged: false,
    supportedItems: ['history', 'bookmarks', 'cookies', 'passwords'],
    recommendedItems: ['history', 'bookmarks', 'cookies'],
  },
]

export const DEFAULT_BROWSEROS_IMPORT_SOURCE_ID =
  MOCK_BROWSEROS_IMPORT_SOURCES[0]?.id ?? ''

function humanizeImportItem(item: string): string {
  const label = item
    .replace(/([a-z0-9])([A-Z])/g, '$1 $2')
    .replace(/[-_]+/g, ' ')
    .trim()
  if (!label) return copy.unknownData
  return label.charAt(0).toUpperCase() + label.slice(1)
}

/** Localized label for one Chromium import item (English without a DOM). */
export function importItemLabel(item: string): string {
  return copy.importItems[item] ?? humanizeImportItem(item)
}

export function importItemListLabel(items: readonly string[]): string {
  if (items.length === 0) return copy.noSupportedData
  return items.map(importItemLabel).join(copy.listSeparator)
}

export const OPTIONAL_IMPORT_ITEMS: readonly BrowserOSImportItem[] = [
  'searchEngines',
  'extensions',
]

function isOptionalImportItem(item: BrowserOSImportItem): boolean {
  return OPTIONAL_IMPORT_ITEMS.includes(item)
}

export function defaultImportItemsForSource(
  source: BrowserOSImportSource,
): BrowserOSImportItem[] {
  return source.supportedItems.filter((item) => !isOptionalImportItem(item))
}

export interface ImportSelectionSplit {
  defaultItems: BrowserOSImportItem[]
  optionalItems: BrowserOSImportItem[]
}

export function splitImportSelection(
  items: readonly BrowserOSImportItem[],
): ImportSelectionSplit {
  return {
    defaultItems: items.filter((item) => !isOptionalImportItem(item)),
    optionalItems: items.filter(isOptionalImportItem),
  }
}

export function sanitizeImportSelection(
  source: BrowserOSImportSource,
  items: readonly BrowserOSImportItem[],
): BrowserOSImportItem[] {
  const selectedItems = new Set(items)
  const emittedItems = new Set<BrowserOSImportItem>()
  return source.supportedItems.filter((item) => {
    if (!selectedItems.has(item) || emittedItems.has(item)) return false
    emittedItems.add(item)
    return true
  })
}

export function selectedSourceById(
  sources: readonly BrowserOSImportSource[],
  sourceId: string,
): BrowserOSImportSource | undefined {
  return sources.find((source) => source.id === sourceId)
}

export interface ImportSourceSelectionChange {
  selectedSourceId: string
  selectedItems: BrowserOSImportItem[]
}

export function importSourceSelectionChangeFor(
  sources: readonly BrowserOSImportSource[],
  currentSourceId: string,
): ImportSourceSelectionChange | null {
  if (sources.length === 0) {
    return { selectedSourceId: '', selectedItems: [] }
  }
  if (selectedSourceById(sources, currentSourceId)) return null
  const nextSource = sources[0]
  return {
    selectedSourceId: nextSource.id,
    selectedItems: defaultImportItemsForSource(nextSource),
  }
}

export function startImportRequestFor(
  source: BrowserOSImportSource,
  items?: readonly BrowserOSImportItem[],
): BrowserOSStartImportRequest | null {
  const importItems =
    items === undefined
      ? defaultImportItemsForSource(source)
      : sanitizeImportSelection(source, items)
  if (importItems.length === 0) return null
  return {
    sourceId: source.id,
    items: importItems,
  }
}

export function completedImportItemCount(
  progress: BrowserOSImportProgress | undefined,
): number {
  return progress?.completedItems.length ?? 0
}

export function importProgressTotal(
  selectedItemCount: number,
  progress: BrowserOSImportProgress | undefined,
): number {
  return progress?.totalItems ?? selectedItemCount
}

/** The native API omits platform; use browser platform hints, never source IDs. */
export function isMacOS(): boolean {
  if (typeof navigator === 'undefined') return false
  const client = navigator as Navigator & {
    userAgentData?: { platform?: string }
  }
  const platform = client.userAgentData?.platform || client.platform
  return /^(macOS|MacIntel|MacPPC|Mac68K)$/i.test(platform)
}

/** Match the Chrome-specific explanation only to supported encrypted data. */
export function needsMacKeychainPermission(
  isMac: boolean,
  source: BrowserOSImportSource | undefined,
  selectedItems: readonly BrowserOSImportItem[],
): boolean {
  if (!isMac || !source || !/^(Google )?Chrome$/i.test(source.browserName))
    return false
  return sanitizeImportSelection(source, selectedItems).some(
    (item) => item === 'cookies' || item === 'passwords',
  )
}

/** Chromium's untouched profile names. Most people never rename their Chrome or
 * Edge profile, so these would otherwise be the biggest text on the tile. */
const PLACEHOLDER_PROFILE_NAME = /^(Default|Profile (\d+)|Person (\d+))$/i

/** "Google Chrome" → "Chrome", "Microsoft Edge" → "Edge". */
export function shortBrowserName(browserName: string): string {
  return browserName.replace(/^(Google|Microsoft) /, '')
}

export interface ImportSourceLabels {
  /** First line of the profile tile. */
  title: string
  /** Second line of the profile tile. */
  subtitle: string
  /** The profile as people should read it (a real name, or 默认账号 / 账号 N). */
  profileLabel: string
  /** True when Chromium only had a placeholder name ("Default", "Profile 1"). */
  isPlaceholder: boolean
}

/** Tile and status wording for one source. A real profile name leads; a
 * placeholder ("Default", "Profile 1", "Person 2", or nothing) never does —
 * the browser leads and the profile becomes 默认账号 / 账号 N. */
export function importSourceLabels(
  source: BrowserOSImportSource,
): ImportSourceLabels {
  const profileName = source.profileName.trim()
  const placeholder = PLACEHOLDER_PROFILE_NAME.exec(profileName)
  if (profileName && !placeholder) {
    return {
      title: profileName,
      subtitle: source.browserName,
      profileLabel: profileName,
      isPlaceholder: false,
    }
  }
  const number = placeholder?.[2] ?? placeholder?.[3]
  const profileLabel = number
    ? copy.numberedProfile(number)
    : copy.defaultProfile
  return {
    title:
      shortBrowserName(source.browserName) ||
      source.displayName ||
      copy.browserFallback,
    subtitle: profileLabel,
    profileLabel,
    isPlaceholder: true,
  }
}

/** One-line source name for the progress and summary cards. Chromium joins
 * its own displayName ("Microsoft Edge - Default"), so placeholder profiles
 * are rebuilt from the source; real names keep the native wording. */
export function importSourceName(
  source: BrowserOSImportSource | undefined,
  nativeName: string | undefined,
): string | undefined {
  if (!source) return nativeName
  const labels = importSourceLabels(source)
  if (labels.isPlaceholder) {
    return copy.sourceWithProfile(labels.title, labels.profileLabel)
  }
  return nativeName || source.profileName
}

/** What a step-level key press should do. Enter runs the step's main action
 * and Escape steps back, but only when the key is not already owned by the
 * focused control (a button, checkbox, radio or link activates itself on
 * Enter) or by an open dialog. Held keys never repeat an action, so holding
 * Enter cannot run from the welcome screen straight into an import. */
export type StepKeyAction = 'primary' | 'back' | null

export interface StepKeyEvent {
  key: string
  repeat: boolean
  isComposing: boolean
  altKey: boolean
  ctrlKey: boolean
  metaKey: boolean
  shiftKey: boolean
  defaultPrevented: boolean
  target: EventTarget | null
}

const KEY_OWNING_CONTROL = [
  'a[href]',
  'button',
  'input',
  'select',
  'textarea',
  'summary',
  '[contenteditable]:not([contenteditable="false"])',
  '[role="button"]',
  '[role="checkbox"]',
  '[role="radio"]',
  '[role="switch"]',
  '[role="link"]',
  '[role="menuitem"]',
  '[role="option"]',
  '[role="combobox"]',
].join(',')

const DIALOG = '[role="dialog"],[role="alertdialog"]'

function closestFrom(target: EventTarget | null, selector: string): boolean {
  const element = target as { closest?: (selector: string) => unknown } | null
  return typeof element?.closest === 'function'
    ? Boolean(element.closest(selector))
    : false
}

export function stepKeyActionFor(event: StepKeyEvent): StepKeyAction {
  if (event.defaultPrevented || event.repeat || event.isComposing) return null
  if (event.altKey || event.ctrlKey || event.metaKey || event.shiftKey)
    return null
  if (closestFrom(event.target, DIALOG)) return null
  if (event.key === 'Escape') return 'back'
  if (event.key !== 'Enter') return null
  return closestFrom(event.target, KEY_OWNING_CONTROL) ? null : 'primary'
}
