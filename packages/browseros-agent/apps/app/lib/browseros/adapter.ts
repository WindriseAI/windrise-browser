// biome-ignore-all lint/suspicious/noExplicitAny: Low-level browser API adapter requires flexible types
/// <reference path="./chrome-browser-os.d.ts" />

export type PrefObject = chrome.windrise.PrefObject
export type ChoosePathOptions = chrome.windrise.ChoosePathOptions
export type SelectedPath = chrome.windrise.SelectedPath

export class BrowserOSAdapter {
  private static instance: BrowserOSAdapter | null = null

  private constructor() {}

  static getInstance(): BrowserOSAdapter {
    if (!BrowserOSAdapter.instance) {
      BrowserOSAdapter.instance = new BrowserOSAdapter()
    }
    return BrowserOSAdapter.instance
  }

  async getVersion(): Promise<string | null> {
    return new Promise<string | null>((resolve, reject) => {
      if (typeof chrome.windrise.getVersionNumber !== 'function') {
        resolve(null)
        return
      }

      chrome.windrise.getVersionNumber((version: string) => {
        if (chrome.runtime.lastError) {
          reject(new Error(chrome.runtime.lastError.message || 'Unknown error'))
        } else {
          resolve(version)
        }
      })
    })
  }

  async getBrowserosVersion(): Promise<string | null> {
    return new Promise<string | null>((resolve, reject) => {
      if (typeof chrome.windrise.getWindriseVersionNumber !== 'function') {
        resolve(null)
        return
      }

      chrome.windrise.getWindriseVersionNumber((version: string) => {
        if (chrome.runtime.lastError) {
          reject(new Error(chrome.runtime.lastError.message || 'Unknown error'))
        } else {
          resolve(version)
        }
      })
    })
  }

  async logMetric(
    eventName: string,
    properties?: Record<string, any>,
  ): Promise<void> {
    if (typeof chrome.windrise.logMetric !== 'function') {
      return
    }

    return new Promise<void>((resolve, reject) => {
      const callback = () => {
        if (chrome.runtime.lastError) {
          reject(new Error(chrome.runtime.lastError.message || 'Unknown error'))
        } else {
          resolve()
        }
      }

      if (properties) {
        chrome.windrise.logMetric(eventName, properties, callback)
      } else {
        chrome.windrise.logMetric(eventName, callback)
      }
    })
  }

  async getPref(name: string): Promise<PrefObject> {
    if (typeof chrome.windrise?.getPref !== 'function') {
      throw new Error('getPref API not available')
    }

    return new Promise<PrefObject>((resolve, reject) => {
      chrome.windrise.getPref(name, (pref: PrefObject) => {
        if (chrome.runtime.lastError) {
          reject(new Error(chrome.runtime.lastError.message || 'Unknown error'))
        } else {
          resolve(pref)
        }
      })
    })
  }

  async setPref(name: string, value: any, pageId?: string): Promise<boolean> {
    if (typeof chrome.windrise?.setPref !== 'function') {
      throw new Error('setPref API not available')
    }

    return new Promise<boolean>((resolve, reject) => {
      const callback = (success: boolean) => {
        if (chrome.runtime.lastError) {
          reject(new Error(chrome.runtime.lastError.message || 'Unknown error'))
        } else {
          resolve(success)
        }
      }

      if (pageId !== undefined) {
        chrome.windrise.setPref(name, value, pageId, callback)
      } else {
        chrome.windrise.setPref(name, value, callback)
      }
    })
  }

  async choosePath(options?: ChoosePathOptions): Promise<SelectedPath | null> {
    if (typeof chrome.windrise?.choosePath !== 'function') {
      throw new Error('choosePath API not available')
    }

    return new Promise<SelectedPath | null>((resolve, reject) => {
      const callback = (result: SelectedPath | null) => {
        if (chrome.runtime.lastError) {
          reject(new Error(chrome.runtime.lastError.message || 'Unknown error'))
        } else {
          resolve(result)
        }
      }

      if (options) {
        chrome.windrise.choosePath(options, callback)
      } else {
        chrome.windrise.choosePath(callback)
      }
    })
  }

  isAPIAvailable(method: string): boolean {
    return method in chrome.windrise
  }

  getAvailableAPIs(): string[] {
    return Object.keys(chrome.windrise).filter(
      (key) => typeof (chrome.windrise as any)[key] === 'function',
    )
  }
}

/** @public */
export const getBrowserOSAdapter = () => BrowserOSAdapter.getInstance()
