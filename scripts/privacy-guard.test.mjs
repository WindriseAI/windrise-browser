import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import test from 'node:test';

import { loadLocalRules, PATTERNS_FILENAME, scanFiles, scanText, shouldSkipPath } from './privacy-guard.mjs';

test('public structural rules flag personal home and drive workspace paths', () => {
  const samples = [
    ['user-home-path', ['C:', 'Users', 'bob', 'project'].join('\\')],
    ['user-home-path', ['', 'Users', 'jdoe', '.config', 'tool'].join('/')],
    ['user-home-path', ['', 'home', 'contractor', 'src', 'app'].join('/')],
    ['private-workspace-drive', ['D:', 'work', 'sample'].join('\\')],
    ['private-workspace-drive', ['F:', 'worktrees', 'feature-1'].join('/')],
    ['private-workspace-drive', ['E:', 'wr-tmp', 'staging'].join('/')],
    ['private-workspace-drive', ['G:', 'wr-pc9', 'build'].join('/')],
  ];

  for (const [expectedRule, content] of samples) {
    const hits = scanText('docs/notes.md', content);
    assert.ok(
      hits.some((item) => item.ruleId === expectedRule),
      `Expected ${expectedRule} to match: ${content}`,
    );
  }
});

test('public structural rules flag real credential prefixes outside test fixtures', () => {
  const fakeAwsKey = 'AKIA' + 'IOSFODNN7EXAMPLE';
  assert.ok(
    scanText('src/config.ts', `const key = "${fakeAwsKey}";`).some(
      (item) => item.ruleId === 'credential-prefix',
    ),
  );
  assert.deepEqual(scanText('src/config_test.py', `FAKE_KEY = "${fakeAwsKey}"`), []);
});

test('allows generic placeholder usernames, routes, and public paths', () => {
  const safeContent = [
    'CHROMIUM_SRC=C:/src/chromium/src',
    'HOME_WIN=C:/Users/runner/.gitconfig',
    'HOME_EX=/Users/example/project',
    'HOME_ALICE=/Users/alice/workspace',
    'HOME_USER=/home/user/out.pdf',
    'APP_ROUTE=/home/chat',
    'color contrast WCAG AA level',
    'https://github.com/WindriseAI/windrise-browser',
    'export WINDRISE_DISABLE_LOCAL_TESTS=1',
  ].join('\n');

  assert.deepEqual(scanText('docs/example.md', safeContent), []);
  assert.equal(shouldSkipPath('assets/logo.svg'), true);
  assert.equal(shouldSkipPath('packages/browseros/uv.lock'), true);
  assert.equal(shouldSkipPath('packages/browseros/tools/windrise/build.py'), false);
});

test('loads local patterns from git common dir and WINDRISE_PRIVACY_PATTERNS_FILE', () => {
  const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'wr-privacy-'));
  try {
    const commonGit = path.join(tmp, 'main-repo', '.git');
    const wtGit = path.join(commonGit, 'worktrees', 'wt1');
    const wtRoot = path.join(tmp, 'wt1');
    fs.mkdirSync(wtGit, { recursive: true });
    fs.mkdirSync(wtRoot, { recursive: true });
    fs.writeFileSync(path.join(wtGit, 'commondir'), '../..\n', 'utf8');
    fs.writeFileSync(path.join(wtRoot, '.git'), `gitdir: ${wtGit}\n`, 'utf8');

    fs.writeFileSync(
      path.join(commonGit, PATTERNS_FILENAME),
      ['# comment line', '', 'secret-host-99', '/corp\\.invalid/i'].join('\n'),
      'utf8',
    );

    const extraFile = path.join(tmp, 'extra-patterns.txt');
    fs.writeFileSync(extraFile, 'internal-codename-7\n', 'utf8');

    const localRules = loadLocalRules({
      rootDir: wtRoot,
      env: { WINDRISE_PRIVACY_PATTERNS_FILE: extraFile },
    });
    assert.equal(localRules.length, 3);

    const target = path.join(wtRoot, 'notes.md');
    fs.writeFileSync(target, 'host=secret-host-99\nurl=https://corp.invalid\ntag=internal-codename-7\n', 'utf8');
    const hits = scanFiles(wtRoot, ['notes.md'], { localRules });
    assert.equal(hits.length, 3);
  } finally {
    fs.rmSync(tmp, { recursive: true, force: true });
  }
});
