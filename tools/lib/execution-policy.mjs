import os from 'node:os';

export function assertExecutionAllowed({ hostname = os.hostname(), env = process.env } = {}) {
  if (String(hostname).toLowerCase() !== 'hqh' || env.WINDRISE_QUEUE_RUN === '1') return true;
  throw Object.assign(new Error('主电脑不跑测试，投排队单：hqh 禁止直接运行测试；由执行器注入 WINDRISE_QUEUE_RUN=1。'), { code: 'QUEUE_REQUIRED' });
}