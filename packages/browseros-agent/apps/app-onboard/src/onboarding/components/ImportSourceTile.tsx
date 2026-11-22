import { CheckCircle2, Circle, User } from 'lucide-react'
import { cn } from '@/lib/utils'
import type { BrowserOSImportSource } from '../browseros-onboarding-api'

interface ImportSourceTileProps {
  source: BrowserOSImportSource
  selected: boolean
  onSelect: () => void
}

/** One Chromium import source in the picker. Unselected rows are the shared
 * white-72% card; the chosen one is the white "selected" glass (same as the
 * desktop settings nav), marked by an ink check — never a colour fill. */
export function ImportSourceTile({
  source,
  selected,
  onSelect,
}: ImportSourceTileProps) {
  return (
    <label
      className={cn(
        'wr-card flex w-full cursor-pointer items-center gap-3 px-4 py-2.5 text-left has-[:focus-visible]:outline-(--focus) has-[:focus-visible]:outline-2 has-[:focus-visible]:outline-offset-2',
        selected ? 'wr-selected' : 'wr-hoverable',
      )}
    >
      <input
        type="radio"
        name="browseros-import-source"
        checked={selected}
        onChange={onSelect}
        className="sr-only"
      />
      {selected ? (
        <CheckCircle2 className="size-4 shrink-0 text-ink" strokeWidth={2} />
      ) : (
        <Circle className="size-4 shrink-0 text-ink-4" strokeWidth={1.75} />
      )}
      <span className="wr-sunken flex size-[30px] shrink-0 items-center justify-center text-ink-2">
        <User className="size-[15px]" strokeWidth={1.75} />
      </span>
      <div className="min-w-0 flex-1">
        <div className="truncate font-medium text-[14px] text-ink">
          {source.profileName || source.displayName}
        </div>
        <div className="truncate text-[12px] text-ink-3">
          {source.browserName}
        </div>
      </div>
    </label>
  )
}
