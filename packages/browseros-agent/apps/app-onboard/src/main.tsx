import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
import { App } from './App'
import { copy } from './onboarding/copy'
import './styles.css'

// Follow the browser UI language for the tab title and the lang attribute
// (the static index.html ships English).
document.documentElement.lang = copy.htmlLang
document.title = copy.pageTitle

const root = document.getElementById('root')
if (!root) throw new Error('Root element not found')

createRoot(root).render(
  <StrictMode>
    <App />
  </StrictMode>,
)
