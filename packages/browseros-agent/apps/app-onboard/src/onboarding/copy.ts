/**
 * Onboarding copy in the two languages the first-run page ships with.
 *
 * The page follows the browser UI language (R726: 跟系统语言走): Chromium sets
 * `navigator.language` from the UI locale, so a Chinese browser gets 中文 and
 * every other locale gets English. The locale is read once at module load;
 * the first-run page never changes language while it is open.
 *
 * Without a DOM (unit tests render with `renderToStaticMarkup` under Bun) the
 * copy resolves to English, so the English strings stay the tested contract.
 * Native-supplied text (profile names, importer error messages) comes over the
 * chrome.send bridge already localized by Chromium and is shown as-is.
 */

type OnboardingLocale = 'en' | 'zh'

function detectOnboardingLocale(): OnboardingLocale {
  if (typeof document === 'undefined' || typeof navigator === 'undefined') {
    return 'en'
  }
  const preferred = navigator.languages?.[0] ?? navigator.language ?? ''
  return preferred.toLowerCase().startsWith('zh') ? 'zh' : 'en'
}

const en = {
  htmlLang: 'en',
  pageTitle: 'Welcome to Windrise Browser',
  brandName: 'Windrise Browser',
  railHeadline: 'The open source private browser for the web you use every day',
  railHeadlineAccent: 'private browser',
  railFeatures: [
    {
      title: 'Bring your browser',
      description: 'History, bookmarks, and saved logins.',
    },
    {
      title: 'Fast, focused browsing',
      description: 'Tabs, history, and bookmarks in one place.',
    },
    {
      title: 'Runs on your machine',
      description: 'Local-first and private.',
    },
  ],
  railFooter: 'Local-first. Private.',
  progressLabel: 'Onboarding progress',
  welcomeTitleLead: 'Welcome to ',
  welcomeTitleAccent: 'Windrise',
  welcomeTitleTail: '',
  welcomeBody:
    'Two quick steps to get set up: bring your browser over, then finish setup.',
  getStarted: 'Get started',
  skipForNow: 'Skip for now',
  importTitleLead: 'Bring ',
  importTitleAccent: 'everything',
  importTitleTail: ' with you',
  importBody:
    'Copy your history, bookmarks, and saved logins from your current browser. Everything stays on this machine.',
  lookingForProfiles: 'Looking for profiles',
  pickProfile: 'Pick a profile',
  refresh: 'Refresh',
  browserProfiles: 'Browser profiles',
  noProfiles: 'No profiles found.',
  showFewerProfiles: 'Show fewer profiles',
  showMoreProfiles: (count: number) => `Show ${count} more profiles`,
  nothingToCopy: 'Nothing to copy from this profile',
  selectWhatToCopy: 'Select what to copy',
  copyFrom: (browser: string) => `Copy from ${browser}`,
  macPasswordHint: 'macOS may ask for your Mac login password.',
  fromProfile: (profile: string) => `From ${profile}’s profile`,
  somethingWentWrong: 'Something went wrong.',
  importFailedFallback:
    "Windrise couldn't finish this import. Try again below, or refresh the profile list.",
  tryAgain: 'Try again',
  refreshProfiles: 'Refresh profiles',
  continue: 'Continue',
  whatToCopy: 'What to copy',
  categoriesSelected: (count: number) => `${count} categories selected`,
  alsoCopySetup: 'Also copy my browsing setup',
  importing: (item: string) => `Importing ${item}...`,
  importingData: 'data',
  itemsProgress: (done: number, total: number) => `${done} / ${total} items`,
  copiedItems: (count: number, source: string) =>
    `Copied ${count} items from ${source}.`,
  staysOnMachine: 'Stays on this machine. Never uploaded.',
  cardsSkipped: 'Payment cards were skipped.',
  noItemDetails: 'No item details reported',
  noCompletedItems: 'No completed items reported',
  sourceFallback: 'source',
  browserFallback: 'browser',
  setupFailedTitle: 'Setup didn’t finish',
  setupFailedBody: 'Please try again.',
  setupTitle: 'Finishing setup',
  setupBody: 'Preparing Windrise. This should only take a moment.',
  retry: 'Retry',
  keychainTitle: 'Import your Chrome logins',
  keychainBodyLead:
    'Windrise needs your permission to import your logins from Chrome.',
  keychainBodyAsk: 'When macOS asks, enter your Mac login password and click ',
  keychainAllow: 'Allow',
  keychainBack: 'Back',
  keychainBackLabel: 'Back to import options',
  keychainExample: 'Example macOS dialog',
  keychainNote: 'Allow gives access for this import.',
  keychainReminderLead:
    'If macOS asks, enter your Mac login password and click ',
  keychainPromptBody:
    'Windrise Helper wants to use your confidential information stored in “Chrome Safe Storage” in your keychain.',
  keychainPromptHint: 'To allow this, enter the “login” keychain password.',
  keychainPassword: 'Password:',
  keychainAlwaysAllow: 'Always Allow',
  keychainDeny: 'Deny',
  importItems: {
    history: 'History',
    bookmarks: 'Bookmarks',
    cookies: 'Signed-in sessions (cookies)',
    passwords: 'Saved passwords',
    searchEngines: 'Search engines',
    autofill: 'Autofill',
    extensions: 'Extensions',
  } as Record<string, string>,
  unknownData: 'Unknown data',
  noSupportedData: 'No supported data',
  listSeparator: ', ',
}

type OnboardingCopy = typeof en

// 中文：浏览器中文名「风桥浏览器」（R726，11-21，晚于 11-12 的「不要说风桥浏览器」）。
// 句子短、动词在前（U080：用户只看第一句、只点最亮的那颗），全角标点。
const zh: OnboardingCopy = {
  htmlLang: 'zh-CN',
  pageTitle: '欢迎使用风桥浏览器',
  brandName: '风桥浏览器',
  railHeadline: '开源、私密，天天用的网页都在这里',
  railHeadlineAccent: '私密',
  railFeatures: [
    {
      title: '旧浏览器一键搬过来',
      description: '历史记录、书签、保存的登录。',
    },
    { title: '干净好用', description: '标签、历史、书签都在一处。' },
    { title: '只在你的电脑上', description: '数据留在本机，不上传。' },
  ],
  railFooter: '数据留在本机。',
  progressLabel: '设置进度',
  welcomeTitleLead: '欢迎使用',
  welcomeTitleAccent: '风桥浏览器',
  welcomeTitleTail: '',
  welcomeBody: '两步就好：先把旧浏览器的东西搬过来，再完成设置。',
  getStarted: '开始',
  skipForNow: '先跳过',
  importTitleLead: '把',
  importTitleAccent: '旧浏览器',
  importTitleTail: '搬过来',
  importBody: '复制历史记录、书签和保存的登录。全部留在这台电脑上。',
  lookingForProfiles: '正在找浏览器账号',
  pickProfile: '选一个浏览器账号',
  refresh: '刷新',
  browserProfiles: '浏览器账号',
  noProfiles: '没有找到浏览器账号。',
  showFewerProfiles: '收起',
  showMoreProfiles: (count: number) => `再显示 ${count} 个`,
  nothingToCopy: '这个账号没有可复制的内容',
  selectWhatToCopy: '先勾选要复制的内容',
  copyFrom: (browser: string) => `从 ${browser} 复制`,
  macPasswordHint: 'macOS 可能会让你输入电脑登录密码。',
  fromProfile: (profile: string) => `来自「${profile}」`,
  somethingWentWrong: '没有复制成功。',
  importFailedFallback: '这次没复制完。点「再试一次」，或者刷新账号列表。',
  tryAgain: '再试一次',
  refreshProfiles: '刷新账号列表',
  continue: '继续',
  whatToCopy: '复制哪些',
  categoriesSelected: (count: number) => `已选 ${count} 项`,
  alsoCopySetup: '也复制我的浏览器设置',
  importing: (item: string) => `正在复制${item}…`,
  importingData: '数据',
  itemsProgress: (done: number, total: number) => `${done} / ${total} 项`,
  copiedItems: (count: number, source: string) =>
    `已从 ${source} 复制 ${count} 项。`,
  staysOnMachine: '只留在这台电脑上，不会上传。',
  cardsSkipped: '银行卡信息没有复制。',
  noItemDetails: '没有收到明细',
  noCompletedItems: '没有复制完成的项目',
  sourceFallback: '旧浏览器',
  browserFallback: '浏览器',
  setupFailedTitle: '设置没有完成',
  setupFailedBody: '请再试一次。',
  setupTitle: '正在完成设置',
  setupBody: '正在准备风桥浏览器，马上就好。',
  retry: '再试一次',
  keychainTitle: '导入 Chrome 里保存的登录',
  keychainBodyLead: '导入登录需要你的允许。',
  keychainBodyAsk: 'macOS 弹窗时，输入电脑登录密码，再点',
  keychainAllow: '允许',
  keychainBack: '返回',
  keychainBackLabel: '返回导入选项',
  keychainExample: 'macOS 弹窗示例',
  keychainNote: '「允许」只对这一次导入有效。',
  keychainReminderLead: '如果 macOS 弹窗，输入电脑登录密码，再点',
  keychainPromptBody:
    '“Windrise Helper”想要使用您存储在钥匙串的“Chrome Safe Storage”中的机密信息。',
  keychainPromptHint: '若要允许，请输入“登录”钥匙串密码。',
  keychainPassword: '密码：',
  keychainAlwaysAllow: '始终允许',
  keychainDeny: '拒绝',
  importItems: {
    history: '历史记录',
    bookmarks: '书签',
    cookies: '登录状态（Cookie）',
    passwords: '保存的密码',
    searchEngines: '搜索引擎',
    autofill: '自动填充',
    extensions: '扩展程序',
  },
  unknownData: '未知数据',
  noSupportedData: '没有可复制的内容',
  listSeparator: '、',
}

/** Copy for the current browser UI language; English when no DOM is present. */
export const copy: OnboardingCopy = detectOnboardingLocale() === 'zh' ? zh : en
