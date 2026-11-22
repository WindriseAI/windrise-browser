import { ArrowDownToLine, LayoutPanelLeft, ShieldCheck } from 'lucide-react'
import { copy } from '../copy'

const FEATURE_ICONS = [ArrowDownToLine, LayoutPanelLeft, ShieldCheck] as const

/** Value rail beside the onboarding steps. It sits straight on the silver page
 * (one milky layer, a hairline on its right edge) — no tinted gradient, no
 * colour accent, and the B6 mark stays icon-sized (browser checklist 甲40, 甲42). */
export function VisualRail() {
  const [lead, tail] = splitOnce(copy.railHeadline, copy.railHeadlineAccent)
  return (
    <div className="relative hidden w-[280px] shrink-0 flex-col justify-between overflow-hidden border-line border-r-[0.75px] bg-white/[0.22] p-9 lg:flex xl:w-[340px]">
      <div className="relative flex items-center gap-2.5">
        {/* Must stay a literal public path: importing the asset or inlining it
            breaks scripts/verify-chromium-build.ts (allowlist, no data: URLs).
            make_icons.py writes the B6 app icon here. */}
        <img
          alt=""
          aria-hidden
          className="size-8 rounded-[9px]"
          src="/icon/128.png"
        />
        <div className="font-medium text-[16px] text-ink">
          {copy.brandName}
        </div>
      </div>
      <div className="relative">
        <div className="mb-6 font-medium text-[20px] text-ink leading-[1.4]">
          {lead}
          {tail !== null && (
            <>
              <span className="text-accent">{copy.railHeadlineAccent}</span>
              {tail}
            </>
          )}
        </div>
        <div className="flex flex-col gap-3.5">
          {copy.railFeatures.map((feature, index) => {
            const Icon = FEATURE_ICONS[index] ?? ShieldCheck
            return (
              <div key={feature.title} className="flex items-start gap-3">
                <span className="wr-sunken flex size-8 shrink-0 items-center justify-center text-ink">
                  <Icon className="size-4" strokeWidth={1.75} />
                </span>
                <div className="min-w-0">
                  <div className="font-medium text-[14px] text-ink">
                    {feature.title}
                  </div>
                  <div className="text-[12px] text-ink-2 leading-[1.5]">
                    {feature.description}
                  </div>
                </div>
              </div>
            )
          })}
        </div>
      </div>
      <div className="relative text-[12px] text-ink-3">{copy.railFooter}</div>
    </div>
  )
}

/** Splits text around the first occurrence of `marker`; tail is null if absent. */
function splitOnce(text: string, marker: string): [string, string | null] {
  const index = marker ? text.indexOf(marker) : -1
  if (index < 0) return [text, null]
  return [text.slice(0, index), text.slice(index + marker.length)]
}
