import { Info, Lock } from 'lucide-react'
import { copy } from '../copy'

/** An inert illustration of the OS-owned prompt; credentials only go to macOS.
 * The illustration keeps macOS's own neutral look (see .keychain-* in
 * styles.css) so people recognise the real dialog when it appears. */
export function MacKeychainPreview() {
  return (
    <figure className="m-0 min-w-0">
      <figcaption className="mb-3 text-ink-2 text-xs">
        {copy.keychainExample}
      </figcaption>
      <div className="keychain-preview" aria-hidden="true">
        <div className="keychain-preview-content">
          <div className="keychain-lock">
            <span />
          </div>
          <div className="min-w-0 flex-1">
            <p className="m-0 font-bold text-[13px] leading-[17px]">
              {copy.keychainPromptBody}
            </p>
            <p className="my-3 text-xs leading-[17px]">
              {copy.keychainPromptHint}
            </p>
            <div className="flex items-center gap-2 text-xs">
              <span>{copy.keychainPassword}</span>
              <span className="keychain-password">••••••••</span>
            </div>
          </div>
        </div>
        <div className="keychain-preview-actions">
          <span className="keychain-help">?</span>
          <span className="keychain-button">{copy.keychainAlwaysAllow}</span>
          <span className="flex-1" />
          <span className="keychain-button">{copy.keychainDeny}</span>
          <span className="keychain-allow">
            <span className="keychain-button">{copy.keychainAllow}</span>
          </span>
        </div>
      </div>
    </figure>
  )
}

export function MacKeychainPermissionNote() {
  return (
    <p className="m-0 flex items-start gap-2 text-[13px] text-ink-2 leading-[19px]">
      <Info aria-hidden className="mt-0.5 size-4 shrink-0" strokeWidth={1.75} />
      {copy.keychainNote}
    </p>
  )
}

/** Native progress does not expose whether a Keychain prompt is currently open.
 * Shown as a sunken note under the progress card (not a tinted alert). */
export function MacKeychainReminder() {
  return (
    <div className="wr-sunken mt-4 flex items-start gap-3 p-4 text-[13px] text-ink-2 leading-relaxed">
      <Lock
        aria-hidden
        className="mt-0.5 size-4 shrink-0 text-ink"
        strokeWidth={1.75}
      />
      <p>
        {copy.keychainReminderLead}
        <strong className="font-medium text-ink">{copy.keychainAllow}</strong>
        {copy.htmlLang === 'en' ? '.' : '。'}
      </p>
    </div>
  )
}
