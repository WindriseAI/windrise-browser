import { ChevronRight } from 'lucide-react'
import { useId } from 'react'
import { Checkbox } from '@/components/ui/checkbox'
import type { BrowserOSImportItem } from '../browseros-onboarding-api'
import { copy } from '../copy'
import { importItemLabel, splitImportSelection } from '../onboarding-v2.helpers'

interface ImportItemChecklistProps {
  items: readonly BrowserOSImportItem[]
  checkedItems: readonly BrowserOSImportItem[]
  onToggle: (item: BrowserOSImportItem) => void
}

export function ImportItemChecklist({
  items,
  checkedItems,
  onToggle,
}: ImportItemChecklistProps) {
  const checklistId = useId()
  const checkedItemSet = new Set(checkedItems)
  const { defaultItems, optionalItems } = splitImportSelection(items)

  function renderRow(item: BrowserOSImportItem) {
    const controlId = `${checklistId}-${item}`
    return (
      <label
        key={item}
        htmlFor={controlId}
        className="flex cursor-pointer items-center gap-2.5"
      >
        <Checkbox
          id={controlId}
          checked={checkedItemSet.has(item)}
          onCheckedChange={() => onToggle(item)}
        />
        <span className="text-[13px] text-ink">{importItemLabel(item)}</span>
      </label>
    )
  }

  return (
    <div className="wr-card mb-5 p-4">
      <div className="mb-3 flex items-center justify-between gap-3">
        <div className="font-medium text-[13px] text-ink-2">
          {copy.whatToCopy}
        </div>
        <div className="text-[12px] text-ink-3 tabular-nums">
          {copy.categoriesSelected(checkedItems.length)}
        </div>
      </div>
      <div className="grid grid-cols-1 gap-x-4 gap-y-2.5 min-[420px]:grid-cols-2">
        {defaultItems.map(renderRow)}
      </div>
      {optionalItems.length > 0 && (
        <details className="group mt-3 border-line border-t-[0.75px] pt-3">
          <summary className="flex cursor-pointer list-none items-center gap-1.5 text-[12px] text-ink-3 hover:text-ink-2">
            <ChevronRight className="size-3.5 transition-transform duration-180 ease-(--ease) group-open:rotate-90 motion-reduce:transition-none" />
            {copy.alsoCopySetup}
            <span className="text-ink-4">
              (
              {optionalItems
                .map(importItemLabel)
                .join(copy.listSeparator)
                .toLowerCase()}
              )
            </span>
          </summary>
          <div className="mt-2.5 grid grid-cols-1 gap-x-4 gap-y-2.5 min-[420px]:grid-cols-2">
            {optionalItems.map(renderRow)}
          </div>
        </details>
      )}
    </div>
  )
}
