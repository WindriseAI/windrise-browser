import { assertExecutionAllowed } from './execution-policy.mjs';

try {
  assertExecutionAllowed();
} catch (error) {
  console.error(error.message);
  process.exit(1);
}