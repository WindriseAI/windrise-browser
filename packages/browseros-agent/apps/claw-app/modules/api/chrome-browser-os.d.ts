declare namespace chrome.windrise {
  interface PrefObject {
    key: string
    type: string
    value: unknown
  }

  function getPref(name: string, callback: (pref: PrefObject) => void): void
}
