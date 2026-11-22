import { Check, CreditCard, Lock } from 'lucide-react'
import { copy } from '../copy'

interface ImportedSummaryCardProps {
  importedItemCount: number
  itemSummary: string
  sourceName: string
}

/** Completed-import summary. "Done" is an ink badge with a white tick (no
 * green — the desktop and extension mark success the same way). Enters once
 * with a 240ms rise (rare, first-run; stopped under reduced motion). */
export function ImportedSummaryCard({
  importedItemCount,
  itemSummary,
  sourceName,
}: ImportedSummaryCardProps) {
  return (
    <div className="wr-card mb-[18px] animate-fade-up p-[18px]">
      <div className="mb-3 flex items-center gap-2.5">
        <span className="flex size-7 items-center justify-center rounded-full bg-ink text-on-ink">
          <Check className="size-4" strokeWidth={2.25} />
        </span>
        <span className="font-medium text-[15px] text-ink">
          {copy.copiedItems(importedItemCount, sourceName)}
        </span>
      </div>
      <SummaryRow
        icon={<Check className="size-3.5 text-ink" strokeWidth={2} />}
        text={itemSummary}
      />
      <SummaryRow
        icon={<Lock className="size-3.5 text-ink-3" strokeWidth={1.75} />}
        text={copy.staysOnMachine}
      />
      <SummaryRow
        icon={<CreditCard className="size-3.5 text-ink-3" strokeWidth={1.75} />}
        text={copy.cardsSkipped}
      />
    </div>
  )
}

interface SummaryRowProps {
  icon: React.ReactNode
  text: string
}

function SummaryRow({ icon, text }: SummaryRowProps) {
  return (
    <div className="flex items-center gap-2.5 py-1 text-[13px] text-ink-2">
      {icon}
      {text}
    </div>
  )
}
