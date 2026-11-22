import { Button as ButtonPrimitive } from "@base-ui/react/button"
import { cva, type VariantProps } from "class-variance-authority"

import { cn } from "@/lib/utils"

/*
 * Windrise liquid-glass buttons, three layers (same tokens as the desktop app
 * and the browser extension settings page; the look lives in styles.css
 * .lg-btn-*): default = the one raised white-glass primary per screen,
 * outline / secondary = ink 42% hairline over a faint glass, ghost = text that
 * only grows a thin glass on hover. Pressing scales to .97 (transform only);
 * there is no colour accent and no red. The bg-primary utility stays as the
 * flat fallback under the gradient.
 */
const buttonVariants = cva(
  "lg-btn group/button inline-flex shrink-0 items-center justify-center rounded-full border border-transparent bg-clip-padding text-sm font-medium whitespace-nowrap outline-none select-none disabled:pointer-events-none [&_svg]:pointer-events-none [&_svg]:shrink-0 [&_svg:not([class*='size-'])]:size-4",
  {
    variants: {
      variant: {
        default: "lg-btn-primary bg-primary text-primary-foreground",
        outline: "lg-btn-secondary",
        secondary: "lg-btn-secondary",
        ghost: "lg-btn-text",
        destructive: "lg-btn-secondary",
        link: "lg-btn-text underline-offset-4 hover:underline",
      },
      size: {
        default:
          "h-9 gap-1.5 px-4 has-data-[icon=inline-end]:pr-3 has-data-[icon=inline-start]:pl-3",
        xs: "h-6 gap-1 px-2.5 text-xs [&_svg:not([class*='size-'])]:size-3",
        sm: "h-8 gap-1 px-3",
        lg: "h-11 gap-2 px-5 text-[15px]",
        icon: "size-9",
        "icon-xs": "size-6 [&_svg:not([class*='size-'])]:size-3",
        "icon-sm": "size-8",
        "icon-lg": "size-10",
      },
    },
    defaultVariants: {
      variant: "default",
      size: "default",
    },
  }
)

function Button({
  className,
  variant = "default",
  size = "default",
  ...props
}: ButtonPrimitive.Props & VariantProps<typeof buttonVariants>) {
  return (
    <ButtonPrimitive
      data-slot="button"
      className={cn(buttonVariants({ variant, size, className }))}
      {...props}
    />
  )
}

export { Button, buttonVariants }
