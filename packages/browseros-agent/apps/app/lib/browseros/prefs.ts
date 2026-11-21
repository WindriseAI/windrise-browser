/** @public */
export const BROWSEROS_PREFS = {
  MCP_PORT: 'windrise.server.mcp_port',
  THIRD_PARTY_LLM_PROVIDERS: 'windrise.third_party_llm.providers',
  PROXY_PORT: 'windrise.server.proxy_port',
  SERVER_PORT: 'windrise.server.server_port',
  ALLOW_REMOTE_MCP: 'windrise.server.allow_remote_in_mcp',
  RESTART_SERVER: 'windrise.server.restart_requested',
  SHOW_LLM_CHAT: 'browseros.show_llm_chat',
  SHOW_TOOLBAR_LABELS: 'windrise.show_toolbar_labels',
  VERTICAL_TABS_ENABLED: 'windrise.vertical_tabs_enabled',
  INSTALL_ID: 'browseros.metrics_install_id',
} as const
