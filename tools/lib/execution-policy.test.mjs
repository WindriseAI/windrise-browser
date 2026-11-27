import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import test from 'node:test';

import { assertExecutionAllowed, MARKER_NAME, resolveGitCommonDir } from './execution-policy.mjs';

test('allows execution when env flag and marker file are absent', () => {
  const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'wr-policy-'));
  try {
    fs.mkdirSync(path.join(tmp, '.git'));
    assert.equal(assertExecutionAllowed({ env: {}, cwd: tmp }), true);
  } finally {
    fs.rmSync(tmp, { recursive: true, force: true });
  }
});

test('blocks execution when WINDRISE_DISABLE_LOCAL_TESTS=1', () => {
  const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'wr-policy-'));
  try {
    assert.throws(
      () => assertExecutionAllowed({ env: { WINDRISE_DISABLE_LOCAL_TESTS: '1' }, cwd: tmp }),
      (err) => err?.code === 'LOCAL_TESTS_DISABLED',
    );
  } finally {
    fs.rmSync(tmp, { recursive: true, force: true });
  }
});

test('blocks execution when marker file exists in regular .git directory', () => {
  const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'wr-policy-'));
  try {
    const gitDir = path.join(tmp, '.git');
    const nested = path.join(tmp, 'packages', 'sub');
    fs.mkdirSync(gitDir);
    fs.mkdirSync(nested, { recursive: true });
    fs.writeFileSync(path.join(gitDir, MARKER_NAME), '1\n', 'utf8');
    assert.equal(resolveGitCommonDir(nested), gitDir);
    assert.throws(
      () => assertExecutionAllowed({ env: {}, cwd: nested }),
      (err) => err?.code === 'LOCAL_TESTS_DISABLED',
    );
  } finally {
    fs.rmSync(tmp, { recursive: true, force: true });
  }
});

test('blocks execution when marker file exists in shared worktree common git dir', () => {
  const tmp = fs.mkdtempSync(path.join(os.tmpdir(), 'wr-policy-wt-'));
  try {
    const commonGit = path.join(tmp, 'main', '.git');
    const worktreeGitDir = path.join(commonGit, 'worktrees', 'feat');
    const worktreeRoot = path.join(tmp, 'worktrees', 'feat');
    fs.mkdirSync(worktreeGitDir, { recursive: true });
    fs.mkdirSync(worktreeRoot, { recursive: true });
    fs.writeFileSync(path.join(worktreeGitDir, 'commondir'), '../..\n', 'utf8');
    fs.writeFileSync(path.join(commonGit, MARKER_NAME), '1\n', 'utf8');
    fs.writeFileSync(path.join(worktreeRoot, '.git'), `gitdir: ${worktreeGitDir}\n`, 'utf8');
    assert.equal(resolveGitCommonDir(worktreeRoot), commonGit);
    assert.throws(
      () => assertExecutionAllowed({ env: {}, cwd: worktreeRoot }),
      (err) => err?.code === 'LOCAL_TESTS_DISABLED',
    );
  } finally {
    fs.rmSync(tmp, { recursive: true, force: true });
  }
});
