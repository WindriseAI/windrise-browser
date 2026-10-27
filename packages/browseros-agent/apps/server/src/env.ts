/**
 * @license
 * Copyright 2025 BrowserOS
 *
 * Telemetry configuration.
 *
 * Windrise ships with both collectors disabled: the values are literal empty
 * strings, so neither a build-time environment nor a runtime environment can
 * re-enable them. Runtime-only feature toggles should be read at their feature
 * boundary.
 */

// Telemetry is disabled in this build. Keep both values blank; do not replace
// them with credentials for another collector.
export const INLINED_ENV = {
  SENTRY_DSN: '',
  POSTHOG_API_KEY: '',
} as const

export const REQUIRED_FOR_PRODUCTION: readonly (keyof typeof INLINED_ENV)[] =
  []
