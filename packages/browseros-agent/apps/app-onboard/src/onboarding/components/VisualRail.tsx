import { ArrowDownToLine, LayoutPanelLeft, ShieldCheck } from 'lucide-react'
import { copy } from '../copy'

const FEATURE_ICONS = [ArrowDownToLine, LayoutPanelLeft, ShieldCheck] as const

/** Left column of the onboarding card (Opus-2, 11-23). It is the heavier
 * structural layer of the card (apple-design §12: structure reads darker, the
 * interactive column lighter): one faint milky wash and a hairline on its right
 * edge, no tinted gradient, no colour accent. The B6 mark stays icon-sized
 * (browser checklist 甲40, 甲42). Brand at the top, the value points sit on the
 * bottom edge; the old footer line repeated the third point word for word and
 * was the line that got cut by the taskbar, so it is gone. */
export function VisualRail() {
  const [lead, tail] = splitOnce(copy.railHeadline, copy.railHeadlineAccent)
  return (
    <div className="wr-rail relative hidden w-[300px] shrink-0 flex-col justify-between overflow-hidden border-line border-r-[0.75px] px-8 pt-9 pb-9 md:flex lg:w-[320px]">
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
        <div className="font-medium text-[16px] text-ink tracking-[0.01em]">
          {copy.brandName}
        </div>
      </div>
      <div className="relative">
        <div className="mb-7 font-medium text-[20px] text-ink leading-[1.4] tracking-[-0.005em]">
          {lead}
          {tail !== null && (
            <>
              <span className="text-accent">{copy.railHeadlineAccent}</span>
              {tail}
            </>
          )}
        </div>
        <div className="flex flex-col gap-4">
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
    </div>
  )
}

/** Splits text around the first occurrence of `marker`; tail is null if absent. */
function splitOnce(text: string, marker: string): [string, string | null] {
  const index = marker ? text.indexOf(marker) : -1
  if (index < 0) return [text, null]
  return [text.slice(0, index), text.slice(index + marker.length)]
}
