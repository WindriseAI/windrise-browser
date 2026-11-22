import type { ReactNode } from 'react'

export interface DisplayHeadingProps {
  children: ReactNode
}

/** Shared step heading: 30px Medium ink, tight leading and slight negative
 * tracking for display size (apple-design §15). HarmonyOS Sans ships only
 * Regular / Medium, so weight stays 500 like the desktop titles. */
export function DisplayHeading({ children }: DisplayHeadingProps) {
  return (
    <h1 className="mb-3 font-medium font-sans text-[30px] text-ink leading-[1.2] tracking-[-0.01em]">
      {children}
    </h1>
  )
}

export interface EmProps {
  children: ReactNode
}

/** Emphasised words in a heading. There is no colour accent in Windrise
 * (browser checklist 甲04), so emphasis is the same ink; the span stays so the
 * copy keeps its structure for both languages. */
export function Em({ children }: EmProps) {
  return <span className="text-accent">{children}</span>
}

export interface StepCopyProps {
  children: ReactNode
  className?: string
}

export function StepCopy({ children, className = '' }: StepCopyProps) {
  return (
    <p
      className={`mb-6 max-w-[480px] text-[15px] text-ink-2 leading-[1.6] ${className}`}
    >
      {children}
    </p>
  )
}
