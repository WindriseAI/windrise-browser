import { execFileSync } from 'node:child_process';
import fs from 'node:fs';
import path from 'node:path';
import { pathToFileURL } from 'node:url';

import { resolveGitCommonDir } from '../tools/lib/execution-policy.mjs';

export const PATTERNS_FILENAME = 'windrise-privacy-patterns';

const USER_HOME_PATH_RE =
  /(?:[a-zA-Z]:(?:\\\\|\\|\/)|(?:^|[^a-zA-Z0-9_.-])\/)(Users|home)(?:\\\\|\\|\/)([A-Za-z0-9._-]+)(?:$|[^A-Za-z0-9._-])/g;

const PRIVATE_WORKSPACE_DRIVE_RE =
  /[d-zD-Z]:(?:\\\\|\\|\/)+(?:work|worktrees|wr-tmp|wr-pc\d+)(?:$|[^a-zA-Z0-9_-])/;

const CREDENTIAL_PREFIX_RE =
  /\b(?:AKIA[0-9A-Z]{16}|ghp_[A-Za-z0-9]{20,}|github_pat_[A-Za-z0-9_]{20,}|glpat-[A-Za-z0-9_-]{20,}|sk-ant-[A-Za-z0-9_-]{16,}|sk-proj-[A-Za-z0-9_-]{16,}|xox[baprs]-[A-Za-z0-9-]{16,}|AIza[0-9A-Za-z_-]{35})\b|-----BEGIN (?:RSA |EC |OPENSSH |PGP )?PRIVATE KEY-----/;

const ALLOWED_GENERIC_USERS = new Set([
  'runner',
  'runneradmin',
  'example',
  'alice',
  'user',
  'someone',
  'amara',
  'me',
  'shared',
  'neo',
  'owner',
  'dev',
  'synthetic',
  'you',
  'test',
  'windows',
  'shadowfax',
  'x',
]);

const ALLOWED_HOME_ROUTE_SEGMENTS = new Set(['chat', 'personalize', 'agents']);

const SKIP_EXTENSIONS = new Set([
  '.png',
  '.jpg',
  '.jpeg',
  '.gif',
  '.webp',
  '.ico',
  '.svg',
  '.woff',
  '.woff2',
  '.ttf',
  '.otf',
  '.eot',
  '.mp4',
  '.webm',
  '.zip',
  '.gz',
  '.tar',
  '.7z',
  '.exe',
  '.dll',
  '.so',
  '.dylib',
  '.pdb',
  '.pak',
  '.bin',
  '.lock',
  '.sum',
  '.pyc',
  '.sqlite',
  '.db',
]);

function escapeRegExp(text) {
  return text.replace(/[.*+?^${}()|[\]\\]/g, '\\$&');
}

function isTestFixturePath(relPath) {
  const normalized = relPath.replace(/\\/g, '/').toLowerCase();
  return (
    /(^|\/)(__tests__|tests|fixtures|testdata)(\/|$)/.test(normalized) ||
    /(\.|_)(test|spec)\.[a-z0-9]+$/.test(normalized)
  );
}

function parsePatternLine(line, sourceLabel, index) {
  const trimmed = line.trim();
  if (!trimmed || trimmed.startsWith('#')) return null;

  if (trimmed.startsWith('/')) {
    const lastSlash = trimmed.lastIndexOf('/');
    if (lastSlash > 0) {
      const body = trimmed.slice(1, lastSlash);
      const flags = trimmed.slice(lastSlash + 1);
      if (/^[gimsuvy]*$/.test(flags)) {
        return {
          id: `local-pattern:${path.basename(sourceLabel)}:${index + 1}`,
          regex: new RegExp(body, flags.replace(/g/g, '')),
        };
      }
    }
  }

  return {
    id: `local-pattern:${path.basename(sourceLabel)}:${index + 1}`,
    regex: new RegExp(escapeRegExp(trimmed), 'i'),
  };
}

export function resolveLocalPatternFiles({
  rootDir = process.cwd(),
  env = process.env,
  patternFiles,
} = {}) {
  const candidates = [];
  if (Array.isArray(patternFiles)) {
    for (const file of patternFiles) {
      if (file) candidates.push(path.resolve(rootDir, file));
    }
  } else {
    const envFile = env.WINDRISE_PRIVACY_PATTERNS_FILE?.trim();
    if (envFile) {
      candidates.push(path.resolve(rootDir, envFile));
    }
    const commonDir = resolveGitCommonDir(rootDir);
    if (commonDir) {
      candidates.push(path.join(commonDir, PATTERNS_FILENAME));
    }
  }

  const seen = new Set();
  const existing = [];
  for (const file of candidates) {
    const resolved = path.resolve(file);
    if (seen.has(resolved)) continue;
    seen.add(resolved);
    if (fs.existsSync(resolved) && fs.statSync(resolved).isFile()) {
      existing.push(resolved);
    }
  }
  return existing;
}

export function loadLocalRules(options = {}) {
  const rules = [];
  for (const file of resolveLocalPatternFiles(options)) {
    const content = fs.readFileSync(file, 'utf8');
    const lines = content.split(/\r?\n/);
    for (let i = 0; i < lines.length; i += 1) {
      const parsed = parsePatternLine(lines[i], file, i);
      if (parsed) rules.push(parsed);
    }
  }
  return rules;
}

export function shouldSkipPath(relPath) {
  const ext = path.extname(relPath).toLowerCase();
  return SKIP_EXTENSIONS.has(ext);
}

function scanPublicLine(relPath, line, lineNumber, findings) {
  USER_HOME_PATH_RE.lastIndex = 0;
  let match = USER_HOME_PATH_RE.exec(line);
  while (match !== null) {
    const rootSegment = match[1].toLowerCase();
    const user = match[2].toLowerCase();
    const isAllowedRoute = rootSegment === 'home' && ALLOWED_HOME_ROUTE_SEGMENTS.has(user);
    if (!ALLOWED_GENERIC_USERS.has(user) && !isAllowedRoute) {
      findings.push({ path: relPath, line: lineNumber, ruleId: 'user-home-path' });
      break;
    }
    match = USER_HOME_PATH_RE.exec(line);
  }

  if (PRIVATE_WORKSPACE_DRIVE_RE.test(line)) {
    findings.push({ path: relPath, line: lineNumber, ruleId: 'private-workspace-drive' });
  }

  if (!isTestFixturePath(relPath) && CREDENTIAL_PREFIX_RE.test(line)) {
    findings.push({ path: relPath, line: lineNumber, ruleId: 'credential-prefix' });
  }
}

export function scanText(relPath, content, { localRules = [] } = {}) {
  const findings = [];
  scanPublicLine(relPath, relPath, 0, findings);
  for (const rule of localRules) {
    if (rule.regex.test(relPath)) {
      findings.push({ path: relPath, line: 0, ruleId: rule.id });
    }
  }

  const lines = content.split(/\r?\n/);
  for (let idx = 0; idx < lines.length; idx += 1) {
    const line = lines[idx];
    scanPublicLine(relPath, line, idx + 1, findings);
    for (const rule of localRules) {
      if (rule.regex.test(line)) {
        findings.push({ path: relPath, line: idx + 1, ruleId: rule.id });
      }
    }
  }
  return findings;
}

export function scanFiles(rootDir, relPaths, { localRules } = {}) {
  const resolvedLocalRules = localRules ?? loadLocalRules({ rootDir });
  const findings = [];
  for (const relPath of relPaths) {
    const normalized = relPath.replace(/\\/g, '/');
    if (!normalized || shouldSkipPath(normalized)) continue;
    const fullPath = path.resolve(rootDir, normalized);
    if (!fs.existsSync(fullPath)) continue;
    const stat = fs.statSync(fullPath);
    if (!stat.isFile()) continue;
    const buf = fs.readFileSync(fullPath);
    if (buf.includes(0)) continue;
    findings.push(...scanText(normalized, buf.toString('utf8'), { localRules: resolvedLocalRules }));
  }
  return findings;
}

function readGitPathList(rootDir, args) {
  const out = execFileSync('git', args, { cwd: rootDir, encoding: 'utf8', maxBuffer: 64 * 1024 * 1024 });
  return out.split('\0').filter(Boolean);
}

export function listTrackedFiles(rootDir) {
  return readGitPathList(rootDir, ['ls-files', '-z']);
}

export function listStagedFiles(rootDir) {
  return readGitPathList(rootDir, ['diff', '--cached', '--name-only', '--diff-filter=ACMR', '-z']);
}

export function main(argv = process.argv.slice(2), rootDir = process.cwd(), options = {}) {
  let targetPaths;
  if (argv.includes('--staged')) {
    targetPaths = listStagedFiles(rootDir);
  } else if (argv.length === 0 || argv.includes('--all')) {
    targetPaths = listTrackedFiles(rootDir);
  } else {
    targetPaths = argv.filter((arg) => !arg.startsWith('-'));
  }

  const localRules = loadLocalRules({ rootDir, ...options });
  const findings = scanFiles(rootDir, targetPaths, { localRules });
  if (findings.length > 0) {
    for (const item of findings) {
      const loc = item.line > 0 ? `${item.path}:${item.line}` : item.path;
      console.error(`privacy-guard failed: ${loc} [${item.ruleId}]`);
    }
    return 1;
  }
  console.log(`privacy-guard passed (${targetPaths.length} files checked, ${localRules.length} local rules).`);
  return 0;
}

if (process.argv[1] && import.meta.url === pathToFileURL(fs.realpathSync(process.argv[1])).href) {
  process.exitCode = main();
}
