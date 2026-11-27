import fs from 'node:fs';
import path from 'node:path';

export const MARKER_NAME = 'windrise-no-local-tests';

export function resolveGitCommonDir(startDir = process.cwd()) {
  let current = path.resolve(startDir);
  while (true) {
    const dotGit = path.join(current, '.git');
    if (fs.existsSync(dotGit)) {
      const stat = fs.statSync(dotGit);
      if (stat.isDirectory()) return dotGit;
      if (stat.isFile()) {
        const text = fs.readFileSync(dotGit, 'utf8');
        for (const line of text.split(/\r?\n/)) {
          if (line.toLowerCase().startsWith('gitdir:')) {
            const raw = line.slice('gitdir:'.length).trim();
            const gitDir = path.isAbsolute(raw) ? raw : path.resolve(current, raw);
            const commonFile = path.join(gitDir, 'commondir');
            if (fs.existsSync(commonFile) && fs.statSync(commonFile).isFile()) {
              const commonRaw = fs.readFileSync(commonFile, 'utf8').trim();
              if (commonRaw) {
                return path.isAbsolute(commonRaw)
                  ? commonRaw
                  : path.resolve(gitDir, commonRaw);
              }
            }
            return gitDir;
          }
        }
      }
    }
    const parent = path.dirname(current);
    if (parent === current) return null;
    current = parent;
  }
}

export function assertExecutionAllowed({ env = process.env, cwd = process.cwd() } = {}) {
  if (env.WINDRISE_DISABLE_LOCAL_TESTS === '1') {
    throw Object.assign(
      new Error('Local test execution is disabled (WINDRISE_DISABLE_LOCAL_TESTS=1).'),
      { code: 'LOCAL_TESTS_DISABLED' },
    );
  }
  const commonDir = resolveGitCommonDir(cwd);
  if (commonDir) {
    const markerPath = path.join(commonDir, MARKER_NAME);
    if (fs.existsSync(markerPath)) {
      throw Object.assign(
        new Error(`Local test execution is disabled (${markerPath} is present).`),
        { code: 'LOCAL_TESTS_DISABLED' },
      );
    }
  }
  return true;
}
