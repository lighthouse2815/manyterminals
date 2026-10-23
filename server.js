const http = require('http');
const fs = require('fs');
const path = require('path');
const os = require('os');
const { WebSocketServer } = require('ws');
const pty = require('node-pty');

const PORT = process.env.PORT || 38921;
const SHELL = process.env.SHELL || '/bin/bash';
const DEFAULT_CWD = os.homedir();

// MIME types for static files
const MIME_TYPES = {
  '.html': 'text/html; charset=utf-8',
  '.css': 'text/css; charset=utf-8',
  '.js': 'application/javascript; charset=utf-8',
  '.json': 'application/json; charset=utf-8',
  '.svg': 'image/svg+xml',
  '.png': 'image/png',
  '.ico': 'image/x-icon'
};

const vendorFiles = {
  '/vendor/xterm.css': path.join(__dirname, 'node_modules/xterm/css/xterm.css'),
  '/vendor/xterm.js': path.join(__dirname, 'node_modules/xterm/lib/xterm.js'),
  '/vendor/addon-fit.js': path.join(__dirname, 'node_modules/@xterm/addon-fit/lib/addon-fit.js'),
  '/vendor/addon-web-links.js': path.join(__dirname, 'node_modules/@xterm/addon-web-links/lib/addon-web-links.js')
};

const server = http.createServer((req, res) => {
  const url = new URL(req.url, `http://${req.headers.host}`);
  let pathname = url.pathname;

  if (pathname === '/health') {
    res.writeHead(200, { 'Content-Type': 'application/json' });
    res.end(JSON.stringify({ status: 'ok', pid: process.pid, defaultCwd: DEFAULT_CWD }));
    return;
  }

  // Check vendor files
  if (vendorFiles[pathname]) {
    const filePath = vendorFiles[pathname];
    fs.readFile(filePath, (err, data) => {
      if (err) {
        res.writeHead(404);
        res.end('Vendor file not found');
        return;
      }
      const ext = path.extname(filePath);
      res.writeHead(200, { 'Content-Type': MIME_TYPES[ext] || 'application/octet-stream' });
      res.end(data);
    });
    return;
  }

  // Serve static files from public/
  if (pathname === '/') pathname = '/index.html';
  const safePath = path.normalize(pathname).replace(/^(\.\.[\/\\])+/, '');
  const filePath = path.join(__dirname, 'public', safePath);

  fs.stat(filePath, (err, stats) => {
    if (err || !stats.isFile()) {
      res.writeHead(404, { 'Content-Type': 'text/plain' });
      res.end('404 Not Found');
      return;
    }

    const ext = path.extname(filePath);
    res.writeHead(200, {
      'Content-Type': MIME_TYPES[ext] || 'application/octet-stream',
      'Cache-Control': 'no-cache'
    });
    fs.createReadStream(filePath).pipe(res);
  });
});

// WebSocket Server
const wss = new WebSocketServer({ server });
const sessions = new Map(); // id -> { id, ptyProcess, title, cwd }

function createTerminal(ws, { id, cols = 80, rows = 24, cwd = DEFAULT_CWD, title = 'Terminal' }) {
  // If an existing session with this id exists, kill it first
  if (sessions.has(id)) {
    const existing = sessions.get(id);
    try {
      existing.ptyProcess.kill();
    } catch (e) {}
    sessions.delete(id);
  }

  const effectiveCwd = (fs.existsSync(cwd) && fs.statSync(cwd).isDirectory()) ? cwd : DEFAULT_CWD;

  const env = Object.assign({}, process.env, {
    TERM: 'xterm-256color',
    COLORTERM: 'truecolor',
    MANYTERMINALS: '1'
  });

  const ptyProcess = pty.spawn(SHELL, [], {
    name: 'xterm-256color',
    cols: Math.max(10, cols),
    rows: Math.max(5, rows),
    cwd: effectiveCwd,
    env
  });

  const session = { id, ptyProcess, title, cwd: effectiveCwd };
  sessions.set(id, session);

  ptyProcess.onData((data) => {
    if (ws.readyState === ws.OPEN) {
      ws.send(JSON.stringify({ type: 'data', id, data }));
    }
  });

  ptyProcess.onExit(({ exitCode, signal }) => {
    if (ws.readyState === ws.OPEN) {
      ws.send(JSON.stringify({ type: 'exit', id, exitCode, signal }));
    }
    sessions.delete(id);
  });

  if (ws.readyState === ws.OPEN) {
    ws.send(JSON.stringify({
      type: 'created',
      id,
      title,
      cwd: effectiveCwd,
      shell: SHELL,
      pid: ptyProcess.pid
    }));
  }

  return session;
}

wss.on('connection', (ws) => {
  ws.on('message', (raw) => {
    let msg;
    try {
      msg = JSON.parse(raw);
    } catch (err) {
      console.error('Invalid JSON message:', err);
      return;
    }

    switch (msg.type) {
      case 'create': {
        const id = msg.id || `term-${Date.now()}-${Math.random().toString(36).substr(2, 5)}`;
        createTerminal(ws, {
          id,
          cols: msg.cols,
          rows: msg.rows,
          cwd: msg.cwd || DEFAULT_CWD,
          title: msg.title || 'Terminal'
        });
        break;
      }

      case 'data': {
        const session = sessions.get(msg.id);
        if (session && session.ptyProcess) {
          session.ptyProcess.write(msg.data);
        }
        break;
      }

      case 'broadcast': {
        // Send data to all active terminals
        for (const session of sessions.values()) {
          try {
            session.ptyProcess.write(msg.data);
          } catch (e) {}
        }
        break;
      }

      case 'resize': {
        const session = sessions.get(msg.id);
        if (session && session.ptyProcess && msg.cols && msg.rows) {
          try {
            session.ptyProcess.resize(Math.max(5, msg.cols), Math.max(2, msg.rows));
          } catch (e) {}
        }
        break;
      }

      case 'restart': {
        const session = sessions.get(msg.id);
        const cwd = session ? session.cwd : DEFAULT_CWD;
        const title = session ? session.title : 'Terminal';
        createTerminal(ws, {
          id: msg.id,
          cols: msg.cols,
          rows: msg.rows,
          cwd,
          title
        });
        break;
      }

      case 'kill': {
        const session = sessions.get(msg.id);
        if (session) {
          try {
            session.ptyProcess.kill();
          } catch (e) {}
          sessions.delete(msg.id);
        }
        break;
      }
    }
  });

  ws.on('close', () => {
    // Keep sessions running in case of page refresh, or cleanup if intended
  });
});

// Clean up child processes on server shutdown
function cleanup() {
  console.log('\nShutting down ManyTerminals server...');
  for (const [id, session] of sessions.entries()) {
    try {
      session.ptyProcess.kill();
    } catch (e) {}
  }
  process.exit(0);
}

process.on('SIGINT', cleanup);
process.on('SIGTERM', cleanup);

server.listen(PORT, '127.0.0.1', () => {
  console.log(`ManyTerminals server running at http://127.0.0.1:${PORT}`);
  console.log(`Default shell: ${SHELL}`);
  console.log(`Default directory: ${DEFAULT_CWD}`);
});
