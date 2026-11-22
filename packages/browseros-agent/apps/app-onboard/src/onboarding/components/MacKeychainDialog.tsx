import { Dialog } from '@base-ui/react/dialog'
import { ArrowRight, Lock, X } from 'lucide-react'
import { type RefObject, useRef } from 'react'
import { Button } from '@/components/ui/button'
import { copy } from '../copy'
import {
  MacKeychainPermissionNote,
  MacKeychainPreview,
} from './MacKeychainNotice'

interface MacKeychainDialogProps {
  open: boolean
  onOpenChange: (open: boolean) => void
  onContinue: () => void
  returnFocus: RefObject<HTMLButtonElement | null>
}

/** Prepares the user before Chromium requests Keychain access, without handling credentials.
 * Dialog chrome follows browser checklist 乙08 (#F7F8F9, radius 16, one shadow);
 * it is a modal, so it scales from the centre (.96 → 1) with its scrim, 240ms,
 * and only fades under reduced motion. */
export function MacKeychainDialog({
  open,
  onOpenChange,
  onContinue,
  returnFocus,
}: MacKeychainDialogProps) {
  const titleRef = useRef<HTMLHeadingElement>(null)
  return (
    <Dialog.Root open={open} onOpenChange={onOpenChange}>
      <Dialog.Portal>
        <Dialog.Backdrop className="wr-scrim fixed inset-0 z-40 transition-opacity duration-240 ease-(--ease) data-[ending-style]:opacity-0 data-[starting-style]:opacity-0 motion-reduce:transition-none" />
        <Dialog.Popup
          initialFocus={titleRef}
          finalFocus={returnFocus}
          className="fixed top-1/2 left-1/2 z-50 flex max-h-[calc(100dvh-32px)] w-[600px] max-w-[calc(100vw-32px)] -translate-x-1/2 -translate-y-1/2 flex-col gap-5 overflow-y-auto rounded-[16px] border-[0.75px] border-white/90 bg-popover p-5 text-ink shadow-pop outline-none transition-[opacity,scale] duration-240 ease-(--ease) data-[ending-style]:scale-96 data-[starting-style]:scale-96 data-[ending-style]:opacity-0 data-[starting-style]:opacity-0 motion-reduce:transition-none motion-reduce:data-[ending-style]:scale-100 motion-reduce:data-[starting-style]:scale-100 sm:p-7"
        >
          <div className="flex items-center justify-between">
            <span className="wr-sunken flex size-10 items-center justify-center text-ink">
              <Lock aria-hidden className="size-5" strokeWidth={1.75} />
            </span>
            <Dialog.Close
              render={
                <Button
                  variant="ghost"
                  size="icon"
                  aria-label={copy.keychainBackLabel}
                />
              }
            >
              <X className="size-[18px]" />
            </Dialog.Close>
          </div>
          <div>
            <Dialog.Title
              ref={titleRef}
              tabIndex={-1}
              className="mb-3 font-medium text-[24px] leading-[32px] outline-none"
            >
              {copy.keychainTitle}
            </Dialog.Title>
            <Dialog.Description className="text-[15px] text-ink-2 leading-[23px]">
              {copy.keychainBodyLead}
              <br />
              {copy.keychainBodyAsk}
              <strong className="font-medium text-ink">
                {copy.keychainAllow}
              </strong>
              {copy.htmlLang === 'en' ? '.' : '。'}
            </Dialog.Description>
          </div>
          <MacKeychainPreview />
          <MacKeychainPermissionNote />
          <div className="flex justify-end gap-2.5 pt-1">
            <Dialog.Close
              render={<Button size="lg" variant="outline" className="px-5" />}
            >
              {copy.keychainBack}
            </Dialog.Close>
            <Button
              type="button"
              size="lg"
              className="px-5"
              onClick={onContinue}
            >
              {copy.continue} <ArrowRight aria-hidden className="size-4" />
            </Button>
          </div>
        </Dialog.Popup>
      </Dialog.Portal>
    </Dialog.Root>
  )
}
