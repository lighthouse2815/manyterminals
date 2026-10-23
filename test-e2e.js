const http = require('http');
const { WebSocket } = require('ws');
const { spawn } = require('child_process');

const PORT = 38921;

async function runTests() {
  console.log('--- Starting ManyTerminals E2E Test ---');

  // 1. Start server
  const serverProcess = spawn('node', ['server.js'], {
    stdio: 'inherit',
    env: Object.assign({}, process.env, { PORT: String(PORT) })
  });

  // Cleanup on exit
  const cleanup = () => {
    try {
      serverProcess.kill('SIGTERM');
    } catch (e) {}
  };
  process.on('exit', cleanup);
  process.on('SIGINT', cleanup);

  // Wait for server to start
  await new Promise((resolve) => setTimeout(resolve, 800));

  // 2. Test HTTP endpoints
  const endpoints = [
    '/health',
    '/',
    '/style.css',
    '/app.js',
    '/vendor/xterm.css',
    '/vendor/xterm.js',
    '/vendor/addon-fit.js'
  ];

  for (const ep of endpoints) {
    const res = await new Promise((resolve, reject) => {
      http.get(`http://127.0.0.1:${PORT}${ep}`, (r) => {
        let data = '';
        r.on('data', chunk => data += chunk);
        r.on('end', () => resolve({ statusCode: r.statusCode, data }));
      }).on('error', reject);
    });

    if (res.statusCode !== 200) {
      throw new Error(`Endpoint ${ep} returned status ${res.statusCode}`);
    }
    console.log(`✓ Endpoint ${ep} OK (200)`);
  }

  // 3. Test WebSocket PTY interaction
  console.log('Testing WebSocket PTY session...');
  const ws = new WebSocket(`ws://127.0.0.1:${PORT}`);

  await new Promise((resolve, reject) => {
    ws.on('open', resolve);
    ws.on('error', reject);
  });

  console.log('✓ WebSocket connected');

  let outputBuffer = '';
  const testPtyId = 'test-term-1';

  const testPassed = await new Promise((resolve, reject) => {
    const timeout = setTimeout(() => {
      reject(new Error(`Timeout waiting for PTY output. Received so far:\n${outputBuffer}`));
    }, 5000);

    ws.on('message', (raw) => {
      const msg = JSON.parse(raw);
      if (msg.type === 'data' && msg.id === testPtyId) {
        outputBuffer += msg.data;
        if (outputBuffer.includes('HELLO_MANYTERMINALS')) {
          clearTimeout(timeout);
          resolve(true);
        }
      }
    });

    // Send create command
    ws.send(JSON.stringify({
      type: 'create',
      id: testPtyId,
      cols: 80,
      rows: 24,
      title: 'Test Terminal'
    }));

    // Wait a brief moment for bash prompt, then send command
    setTimeout(() => {
      ws.send(JSON.stringify({
        type: 'data',
        id: testPtyId,
        data: 'echo HELLO_MANYTERMINALS\n'
      }));
    }, 400);
  });

  if (testPassed) {
    console.log('✓ PTY echoed back command correctly!');
    console.log('✓ Test output verified: HELLO_MANYTERMINALS found in PTY output stream.');
  }

  // 4. Test kill command
  ws.send(JSON.stringify({ type: 'kill', id: testPtyId }));
  ws.close();

  serverProcess.kill('SIGTERM');
  console.log('\n🎉 ALL TESTS PASSED SUCCESSFULLY! 🎉\n');
  process.exit(0);
}

runTests().catch((err) => {
  console.error('Test failed:', err);
  process.exit(1);
});
