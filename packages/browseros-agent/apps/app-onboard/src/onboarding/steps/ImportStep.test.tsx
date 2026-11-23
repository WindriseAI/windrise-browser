import { describe, expect, it } from 'bun:test'
import { renderToStaticMarkup } from 'react-dom/server'
import { useForm } from 'react-hook-form'
import { Form } from '@/components/ui/form'
import type { BrowserOSOnboardingState } from '../browseros-onboarding-api'
import { BROWSEROS_ONBOARDING_API_VERSION } from '../browseros-onboarding-api'
import {
  defaultImportItemsForSource,
  importItemLabel,
  MOCK_BROWSEROS_IMPORT_SOURCES,
} from '../onboarding-v2.helpers'
import {
  type OnboardingFormValues,
  onboardingFormDefaults,
  onboardingFormResolver,
  onboardingFormSchema,
} from '../onboarding-v2.schemas'
import type { ImportPhase } from '../onboarding-v2.types'
import { ImportStep } from './ImportStep'

// Take the message from the resolver rather than restating it, so these tests
// track the production copy instead of a string they hardcoded themselves.
const unpickedResult = onboardingFormSchema.safeParse({
  selectedSourceId: '',
  selectedItems: [],
})
const PICK_PROFILE_MESSAGE = unpickedResult.success
  ? ''
  : (unpickedResult.error.issues[0]?.message ?? '')

function readyState(
  overrides: Partial<BrowserOSOnboardingState> = {},
): BrowserOSOnboardingState {
  return {
    apiVersion: BROWSEROS_ONBOARDING_API_VERSION,
    status: 'ready',
    sources: [...MOCK_BROWSEROS_IMPORT_SOURCES],
    ...overrides,
  }
}

function Harness({
  phase,
  state = readyState(),
  formValues = {},
  unpicked = false,
}: {
  phase: ImportPhase
  state?: BrowserOSOnboardingState
  formValues?: Partial<OnboardingFormValues>
  unpicked?: boolean
}) {
  const form = useForm<OnboardingFormValues>({
    resolver: onboardingFormResolver,
    defaultValues: { ...onboardingFormDefaults, ...formValues },
  })
  // Reproduces the error OnboardingV2 sets on itself: its sources effect calls
  // setValue('selectedSourceId', '', { shouldValidate: true }), and the resolver
  // rejects the empty id. Seeding it here is the only way to get a form error
  // into a server render, nothing else validates (no events, no effects).
  if (unpicked) {
    form.setError('selectedSourceId', {
      type: 'required',
      message: PICK_PROFILE_MESSAGE,
    })
  }
  return (
    <Form {...form}>
      <ImportStep
        phase={phase}
        state={state}
        form={form}
        onImport={() => undefined}
        onRefresh={() => undefined}
        onContinue={() => undefined}
        onBack={() => undefined}
      />
    </Form>
  )
}

function render(
  phase: ImportPhase,
  state: BrowserOSOnboardingState = readyState(),
  formValues: Partial<OnboardingFormValues> = {},
  unpicked = false,
): string {
  return renderToStaticMarkup(
    <Harness
      phase={phase}
      state={state}
      formValues={formValues}
      unpicked={unpicked}
    />,
  )
}

function checklistRowFor(html: string, label: string): string {
  const checklistStart = html.indexOf('What to copy')
  const labelIndex = html.indexOf(`>${label}</span>`, checklistStart)
  if (checklistStart === -1 || labelIndex === -1) return ''
  const rowStart = html.lastIndexOf('<label', labelIndex)
  const rowEnd = html.indexOf('</label>', labelIndex)
  return html.slice(rowStart, rowEnd + '</label>'.length)
}

// The empty picker renders a disabled import CTA next to an enabled skip, so
// `disabled` assertions have to be scoped to one button's own markup. Labels can
// also appear outside buttons ("Pick a profile" is a prefix of the "Pick a
// profile to import" heading), so skip matches that aren't inside a button.
function buttonMarkupFor(html: string, label: string): string {
  for (
    let labelIndex = html.indexOf(label);
    labelIndex !== -1;
    labelIndex = html.indexOf(label, labelIndex + 1)
  ) {
    const buttonStart = html.lastIndexOf('<button', labelIndex)
    const buttonEnd = html.indexOf('</button>', labelIndex)
    if (buttonStart === -1 || buttonEnd === -1) continue
    if (html.indexOf('</button>', buttonStart) !== buttonEnd) continue
    return html.slice(buttonStart, buttonEnd + '</button>'.length)
  }
  return ''
}

describe('ImportStep', () => {
  it('renders a compact picker without the permission dialog before Copy', () => {
    const html = render('picker')
    expect(html).toContain('Pick a profile')
    expect(html).toContain('Work')
    expect(html).toContain('Personal')
    expect(html).toContain('>Edge</div>')
    expect(html).toContain('>Default profile</div>')
    expect(html).toContain('What to copy')
    for (const item of defaultImportItemsForSource(
      MOCK_BROWSEROS_IMPORT_SOURCES[0],
    )) {
      expect(checklistRowFor(html, importItemLabel(item))).toContain(
        'aria-checked="true"',
      )
    }
    expect(html).toContain('5 categories selected')
    expect(html).not.toContain('Example macOS dialog')
    expect(html).not.toContain('Always Allow')
    expect(html).not.toContain('/icon/keychain-prompt.png')
    expect(html).toContain('Copy from Chrome')
    expect(html).not.toContain('Chrome is open')
    expect(html).not.toContain('Quit Chrome for me')
    expect(html).not.toContain('disabled=""')
  })

  it('checks every supported non-optional item by default', () => {
    const source = MOCK_BROWSEROS_IMPORT_SOURCES[1]
    const html = render('picker', readyState(), {
      selectedSourceId: source.id,
      selectedItems: defaultImportItemsForSource(source),
    })

    expect(html).toContain('5 categories selected')
    expect(html).toContain('Copy from Chrome')
    for (const item of [
      'History',
      'Bookmarks',
      'Signed-in sessions (cookies)',
      'Saved passwords',
      'Autofill',
    ]) {
      const row = checklistRowFor(html, item)
      expect(row).toContain('data-checked=""')
      expect(row).toContain('aria-checked="true"')
    }
    expect(html).not.toContain('<details')
  })

  it('collapses only search engines and extensions behind a disclosure', () => {
    const html = render('picker')
    const detailsStart = html.indexOf('<details')
    const detailsEnd = html.indexOf('</details>', detailsStart)
    const details = html.slice(detailsStart, detailsEnd)

    expect(detailsStart).toBeGreaterThan(-1)
    expect(html).toContain('Also copy my browsing setup')
    expect(details).toContain('Search engines')
    expect(details).toContain('Extensions')
    for (const item of [
      'History',
      'Bookmarks',
      'Signed-in sessions (cookies)',
      'Saved passwords',
      'Autofill',
    ]) {
      expect(details).not.toContain(item)
    }
    for (const item of ['Search engines', 'Extensions']) {
      expect(checklistRowFor(html, item)).toContain('aria-checked="false"')
    }
  })

  it('counts only the selected categories', () => {
    const html = render('picker', readyState(), {
      selectedItems: ['cookies', 'passwords', 'history'],
    })

    expect(html).toContain('3 categories selected')
    expect(html).toContain('Copy from Chrome')
  })

  it('disables the copy action until at least one item is selected', () => {
    const html = render('picker', readyState(), { selectedItems: [] })

    expect(html).toContain('0 categories selected')
    expect(html).toContain('Select what to copy')
    expect(html).toContain('disabled=""')
  })

  it('disables import while Chromium is detecting sources', () => {
    const html = render('picker', readyState({ status: 'detecting' }))
    expect(html).toContain('Looking for profiles')
    expect(html).toContain('disabled=""')
  })

  it('disables import when the selected source has no supported items', () => {
    const html = render(
      'picker',
      readyState({
        sources: [
          {
            ...MOCK_BROWSEROS_IMPORT_SOURCES[0],
            recommendedItems: [],
            supportedItems: [],
          },
        ],
      }),
    )
    expect(html).toContain('Nothing to copy from this profile')
    expect(html).not.toContain('What to copy')
    expect(html).toContain('disabled=""')
  })

  it('allows copying one selected category', () => {
    const html = render('picker', readyState(), {
      selectedItems: ['history'],
    })

    expect(html).toContain('1 categories selected')
    expect(html).toContain('Copy from Chrome')
  })

  it('renders the importing progress card during importing phase', () => {
    const html = render(
      'importing',
      readyState({
        status: 'importing',
        progress: {
          currentItem: 'cookies',
          currentSourceId: MOCK_BROWSEROS_IMPORT_SOURCES[0].id,
          currentSourceName: MOCK_BROWSEROS_IMPORT_SOURCES[0].displayName,
          completedItems: ['history', 'bookmarks'],
          totalItems: 7,
          completedSources: 0,
          totalSources: 1,
        },
        results: [
          {
            sourceId: MOCK_BROWSEROS_IMPORT_SOURCES[0].id,
            displayName: MOCK_BROWSEROS_IMPORT_SOURCES[0].displayName,
            status: 'importing',
          },
        ],
      }),
    )
    expect(html).toContain('Importing Signed-in sessions (cookies)')
    expect(html).toContain('Google Chrome - Work')
    expect(html).toContain('2 / 7 items')
  })

  it('renders a failure recovery state when Chromium reports failed', () => {
    const html = render(
      'failed',
      readyState({
        status: 'failed',
        error: {
          code: 'import_failed',
          message: 'Chrome needs to be closed before importing.',
        },
      }),
    )

    expect(html).toContain('Something went wrong.')
    expect(html).toContain('Chrome needs to be closed before importing.')
    expect(html).toContain('Try again')
    expect(html).not.toContain('Pick a profile to import')
  })

  it('renders the success card and continue CTA in imported phase', () => {
    const html = render(
      'imported',
      readyState({
        status: 'succeeded',
        progress: {
          completedItems: MOCK_BROWSEROS_IMPORT_SOURCES[0].recommendedItems,
          totalItems: 7,
          completedSources: 1,
          totalSources: 1,
        },
        results: [
          {
            sourceId: MOCK_BROWSEROS_IMPORT_SOURCES[0].id,
            displayName: MOCK_BROWSEROS_IMPORT_SOURCES[0].displayName,
            status: 'succeeded',
          },
        ],
      }),
    )
    expect(html).toContain('Copied 7 items from Google Chrome - Work')
    expect(html).toContain('History, Bookmarks')
    expect(html).toContain('Continue')
  })

  it('does not fabricate a success summary when progress is missing', () => {
    const html = render('imported', readyState({ status: 'succeeded' }))

    expect(html).toContain('Copied 0 items from Work')
    expect(html).toContain('No item details reported')
    expect(html).not.toContain('History, Bookmarks')
  })

  it('does not fall back to selected items when no completed items are reported', () => {
    const html = render(
      'imported',
      readyState({
        status: 'succeeded',
        progress: {
          completedItems: [],
          totalItems: 7,
        },
      }),
    )

    expect(html).toContain('Copied 0 items from Work')
    expect(html).toContain('No completed items reported')
    expect(html).not.toContain('History, Bookmarks')
  })

  it('leaves an enabled way forward when no profiles are found', () => {
    const html = render('picker', readyState({ sources: [] }))
    const skip = buttonMarkupFor(html, 'Skip for now')

    expect(html).toContain('No profiles found.')
    expect(buttonMarkupFor(html, 'Pick a profile')).toContain('disabled=""')
    expect(skip).toContain('Skip for now')
    expect(skip).not.toContain('disabled=""')
  })

  it('keeps the skip available when profiles are present', () => {
    const skip = buttonMarkupFor(render('picker'), 'Skip for now')

    expect(skip).toContain('Skip for now')
    expect(skip).not.toContain('disabled=""')
  })

  it('keeps the skip enabled while profiles are still being detected', () => {
    const html = render('picker', readyState({ status: 'detecting' }))
    const skip = buttonMarkupFor(html, 'Skip for now')

    expect(html).toContain('Looking for profiles')
    expect(skip).toContain('Skip for now')
    expect(skip).not.toContain('disabled=""')
  })

  it('offers a skip out of a failed import', () => {
    const html = render('failed', readyState({ status: 'failed', sources: [] }))
    const skip = buttonMarkupFor(html, 'Skip for now')

    expect(html).toContain('Try again')
    expect(html).toContain('Choose again')
    expect(skip).toContain('Skip for now')
    expect(skip).not.toContain('disabled=""')
  })

  it('does not offer a skip while an import is running', () => {
    const html = render('importing', readyState({ status: 'importing' }))

    expect(html).not.toContain('Skip for now')
  })

  it('does not offer a skip once the import has succeeded', () => {
    const html = render('imported', readyState({ status: 'succeeded' }))

    expect(html).toContain('Continue')
    expect(html).not.toContain('Skip for now')
  })

  it('does not blame the user when there is no profile to pick', () => {
    const html = render('picker', readyState({ sources: [] }), {}, true)

    expect(html).toContain('No profiles found.')
    expect(html).not.toContain('data-slot="form-message"')
  })

  it('does not blame the user while profiles are still being detected', () => {
    const html = render(
      'picker',
      readyState({ status: 'detecting', sources: [] }),
      {},
      true,
    )

    expect(html).toContain('Looking for profiles')
    expect(html).not.toContain('data-slot="form-message"')
  })

  it('still asks for a pick when profiles are there to pick from', () => {
    const html = render('picker', readyState(), {}, true)

    expect(PICK_PROFILE_MESSAGE).not.toBe('')
    expect(html).toContain('data-slot="form-message"')
    expect(html).toContain(PICK_PROFILE_MESSAGE)
  })
  it('shows four profiles while keeping a selection outside the first four visible', () => {
    const sources = Array.from({ length: 7 }, (_, index) => ({
      ...MOCK_BROWSEROS_IMPORT_SOURCES[0],
      id: `profile-${index}`,
      profileName: `Person ${index}`,
    }))
    const html = render('picker', readyState({ sources }), {
      selectedSourceId: 'profile-6',
    })
    // "Person N" is a Chromium placeholder: it shows as Profile N under the
    // browser name, never as the tile's title.
    expect(html).toContain('Profile 6')
    expect(html).not.toContain('Person 6')
    expect(html).not.toContain('Profile 3')
    expect(html).not.toContain('Profile 4')
    expect(html).toContain('Show 3 more profiles')
    expect(html.match(/type="radio"/g)).toHaveLength(4)
  })

  // DS9-P0-11: a failed import must never trap the user on the failure screen.
  it('offers retry, choose again and skip after a failed import', () => {
    const html = render(
      'failed',
      readyState({
        status: 'failed',
        error: { code: 'import_failed', message: 'Close Chrome first.' },
      }),
    )
    const retry = buttonMarkupFor(html, 'Try again')
    const chooseAgain = buttonMarkupFor(html, 'Choose again')

    expect(retry).not.toContain('disabled=""')
    expect(chooseAgain).toContain('Choose again')
    expect(chooseAgain).not.toContain('disabled=""')
    expect(buttonMarkupFor(html, 'Skip for now')).not.toContain('disabled=""')
    // The failure screen itself has no back: choose again is the way back.
    expect(html).not.toContain('>Back<')
  })

  it('names choose again in the fallback failure text', () => {
    const html = render('failed', readyState({ status: 'failed' }))

    expect(html).toContain('choose another profile or fewer items')
    expect(html).toContain('close it completely and try again')
    expect(html).not.toContain('refresh the profile list')
  })

  it('tells the user to close the running browser when the profile DB is locked', () => {
    const edge = MOCK_BROWSEROS_IMPORT_SOURCES[2]
    const html = render(
      'failed',
      readyState({
        status: 'failed',
        error: {
          code: 'source_browser_locked',
          message: 'database is locked',
        },
        results: [
          {
            sourceId: edge.id,
            displayName: edge.displayName,
            status: 'failed',
          },
        ],
      }),
      { selectedSourceId: edge.id },
    )

    expect(html).toContain(
      'Edge is running and locking your bookmarks and sign-in data. Please close Edge completely, then click Try again.',
    )
  })

  it('detects raw SQLite lock messages and names Chrome when Chrome is selected', () => {
    const html = render(
      'failed',
      readyState({
        status: 'failed',
        error: {
          code: 'import_failed',
          message: 'SQLITE_BUSY: database is locked',
        },
      }),
    )

    expect(html).toContain(
      'Chrome is running and locking your bookmarks and sign-in data. Please close Chrome completely, then click Try again.',
    )
  })

  // DS9-P1-18: step two has a way back to the welcome step.
  it('offers a quiet back button in the picker only', () => {
    const picker = render('picker')
    const back = buttonMarkupFor(picker, '>Back<')

    expect(back).toContain('lg-btn-text')
    expect(back).not.toContain('disabled=""')
    expect(
      render('importing', readyState({ status: 'importing' })),
    ).not.toContain('>Back<')
    expect(
      render('imported', readyState({ status: 'succeeded' })),
    ).not.toContain('>Back<')
  })

  // DS9-P1-19: Chromium placeholder names never lead.
  it('leads with the browser name for an untouched Default profile', () => {
    const edge = MOCK_BROWSEROS_IMPORT_SOURCES[2]
    const html = render('picker', readyState(), {
      selectedSourceId: edge.id,
      selectedItems: defaultImportItemsForSource(edge),
    })

    expect(html).toContain('>Edge</div>')
    expect(html).toContain('>Default profile</div>')
    expect(html).not.toContain('>Default</div>')
    expect(html).toContain('Copy from Edge')
    expect(html).not.toContain('Microsoft Edge</div>')
    expect(html).toContain('From Default profile<')
    expect(html).not.toContain('From Default’s profile')
  })

  it('keeps a real profile name as the title', () => {
    const html = render('picker')

    expect(html).toContain('>Work</div>')
    expect(html).toContain('>Google Chrome</div>')
    expect(html).toContain('From Work’s profile')
  })

  it('names a placeholder profile kindly in the summary and progress cards', () => {
    const edge = MOCK_BROWSEROS_IMPORT_SOURCES[2]
    const imported = render(
      'imported',
      readyState({
        status: 'succeeded',
        progress: {
          completedItems: edge.recommendedItems,
          totalItems: 3,
        },
        results: [
          {
            sourceId: edge.id,
            displayName: edge.displayName,
            status: 'succeeded',
          },
        ],
      }),
      { selectedSourceId: edge.id },
    )
    const importing = render(
      'importing',
      readyState({
        status: 'importing',
        progress: {
          currentItem: 'history',
          currentSourceId: edge.id,
          currentSourceName: edge.displayName,
          completedItems: [],
          totalItems: 3,
        },
      }),
      { selectedSourceId: edge.id },
    )

    expect(imported).toContain('Copied 3 items from Edge (Default profile).')
    expect(imported).not.toContain('Microsoft Edge - Default')
    expect(importing).toContain('Edge (Default profile)')
    expect(importing).not.toContain('Microsoft Edge - Default')
  })
})
