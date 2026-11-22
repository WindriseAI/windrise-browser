import { Checkbox as CheckboxPrimitive } from "@base-ui/react/checkbox"

import { cn } from "@/lib/utils"
import { CheckIcon } from "lucide-react"

/* Windrise checkbox: ink 42% hairline over a faint white when off, solid ink
 * with a white tick when on (the only ink-filled mark, same as the desktop
 * switch "on"). Focus ring is the shared 2px --focus outline. */
function Checkbox({ className, ...props }: CheckboxPrimitive.Root.Props) {
  return (
    <CheckboxPrimitive.Root
      data-slot="checkbox"
      className={cn(
        "peer relative flex size-4 shrink-0 items-center justify-center rounded-[5px] border border-input bg-white/70 outline-none after:absolute after:-inset-x-3 after:-inset-y-2 focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-(--focus) disabled:cursor-not-allowed disabled:opacity-45 data-checked:border-ink data-checked:bg-ink data-checked:text-on-ink",
        className
      )}
      {...props}
    >
      <CheckboxPrimitive.Indicator
        data-slot="checkbox-indicator"
        className="grid place-content-center text-current transition-none [&>svg]:size-3.5"
      >
        <CheckIcon strokeWidth={2.25} />
      </CheckboxPrimitive.Indicator>
    </CheckboxPrimitive.Root>
  )
}

export { Checkbox }
