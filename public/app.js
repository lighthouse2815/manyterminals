(function () {
  'use strict';

  // State
  const terminals = new Map(); // id -> { id, title, term, fitAddon, element, isMaximized, resizeObserver }
  let focusedTerminalId = null;
  let broadcastMode = false;
  let currentLayout = 'auto'; // 'auto', '1', '2', '3', '4'
  let termCounter = 1;
  let ws = null;
  let initialLoaded = false;

  // DOM Elements
  const gridContainer = document.getElementById('grid-container');
  const emptyState = document.getElementById('empty-state');
  const emptyAddBtn = document.getElementById('empty-add-btn');
  const addTerminalBtn = document.getElementById('add-terminal-btn');
  const broadcastToggle = document.getElementById('broadcast-toggle');
  const broadcastBanner = document.getElementById('broadcast-banner');
  const broadcastOffBtn = document.getElementById('broadcast-off-btn');
  const termCounterBadge = document.getElementById('term-counter');
  const connectionStatus = document.getElementById('connection-status');
  const segButtons = document.querySelectorAll('.seg-btn');

  // Terminal Theme (OneDark / Cyberpunk Modern Dark)
  const TERMINAL_THEME = {
    background: '#181a1f',
    foreground: '#abb2bf',
    cursor: '#528bff',
    cursorAccent: '#181a1f',
    selectionBackground: '#3e4451',
    selectionForeground: '#ffffff',
    black: '#282c34',
    red: '#e06c75',
    green: '#98c379',
    yellow: '#e5c07b',
    blue: '#61afef',
    magenta: '#c678dd',
    cyan: '#56b6c2',
    white: '#abb2bf',
    brightBlack: '#5c6370',
    brightRed: '#e06c75',
    brightGreen: '#98c379',
    brightYellow: '#e5c07b',
    brightBlue: '#61afef',
    brightMagenta: '#c678dd',
    brightCyan: '#56b6c2',
    brightWhite: '#ffffff'
  };

  // Connect WebSocket
  function initWebSocket() {
    const protocol = location.protocol === 'https:' ? 'wss:' : 'ws:';
    const wsUrl = `${protocol}//${location.host}`;
    ws = new WebSocket(wsUrl);

    ws.onopen = () => {
      console.log('Connected to ManyTerminals PTY server');
      connectionStatus.className = 'status-badge connected';
      connectionStatus.innerHTML = '<span class="dot"></span> Connected';

      if (!initialLoaded) {
        initialLoaded = true;
        // User requested: Start with 1 terminal pane
        createTerminal();
      }
    };

    ws.onclose = () => {
      console.warn('Disconnected from PTY server, attempting reconnect in 2s...');
      connectionStatus.className = 'status-badge disconnected';
      connectionStatus.innerHTML = '<span class="dot"></span> Disconnected';
      setTimeout(initWebSocket, 2000);
    };

    ws.onerror = (err) => {
      console.error('WebSocket error:', err);
    };

    ws.onmessage = (event) => {
      try {
        const msg = JSON.parse(event.data);
        handleServerMessage(msg);
      } catch (e) {
        console.error('Failed to parse message:', e);
      }
    };
  }

  // Handle messages from server
  function handleServerMessage(msg) {
    const item = terminals.get(msg.id);

    switch (msg.type) {
      case 'created': {
        if (item) {
          const cwdSpan = item.element.querySelector('.pane-cwd');
          if (cwdSpan && msg.cwd) {
            cwdSpan.textContent = msg.cwd;
            cwdSpan.title = msg.cwd;
          }
          const statusDot = item.element.querySelector('.pane-status-dot');
          if (statusDot) statusDot.className = 'pane-status-dot';
        }
        break;
      }

      case 'data': {
        if (item && item.term) {
          item.term.write(msg.data);
        }
        break;
      }

      case 'exit': {
        if (item) {
          const statusDot = item.element.querySelector('.pane-status-dot');
          if (statusDot) statusDot.className = 'pane-status-dot exited';
          if (item.term) {
            item.term.write(`\r\n\x1b[90m[Process exited with code ${msg.exitCode || 0}]\x1b[0m\r\n`);
          }
        }
        break;
      }
    }
  }

  // Create a new Terminal Pane
  function createTerminal(title) {
    const id = `term-${Date.now()}-${Math.random().toString(36).substring(2, 6)}`;
    const termTitle = title || `Agent ${termCounter++}`;

    // Create DOM structure
    const pane = document.createElement('div');
    pane.className = 'terminal-pane';
    pane.id = `pane-${id}`;
    pane.dataset.id = id;

    pane.innerHTML = `
      <div class="pane-header">
        <div class="pane-title-group" title="Click to rename">
          <span class="pane-status-dot"></span>
          <span class="pane-title">${escapeHtml(termTitle)}</span>
          <span class="pane-cwd">~</span>
        </div>
        <div class="pane-actions">
          <button class="pane-btn btn-zoom" title="Maximize/Restore (Alt+Z)">
            <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5">
              <path d="M15 3h6v6M9 21H3v-6M21 3l-7 7M3 21l7-7"/>
            </svg>
          </button>
          <button class="pane-btn btn-clear" title="Clear Buffer">
            <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
              <path d="M18 6L6 18M6 6l12 12"/>
            </svg>
          </button>
          <button class="pane-btn btn-restart" title="Restart Shell">
            <svg width="12" height="12" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
              <path d="M21.5 2v6h-6M21.34 15.57a10 10 0 1 1-.57-8.38l5.67-5.67"/>
            </svg>
          </button>
          <button class="pane-btn btn-close" title="Close Terminal (Alt+W)">
            <svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5">
              <line x1="18" y1="6" x2="6" y2="18"></line>
              <line x1="6" y1="6" x2="18" y2="18"></line>
            </svg>
          </button>
        </div>
      </div>
      <div class="pane-body"></div>
    `;

    gridContainer.appendChild(pane);

    // Initialize xterm
    const bodyEl = pane.querySelector('.pane-body');
    const term = new Terminal({
      cursorBlink: true,
      cursorStyle: 'block',
      fontFamily: "'JetBrains Mono', 'Fira Code', 'DejaVu Sans Mono', monospace, 'Noto Color Emoji'",
      fontSize: 13,
      lineHeight: 1.25,
      theme: TERMINAL_THEME,
      allowTransparency: false,
      convertEol: true,
      scrollback: 5000
    });

    const fitAddon = new FitAddon.FitAddon();
    term.loadAddon(fitAddon);

    if (window.WebLinksAddon && window.WebLinksAddon.WebLinksAddon) {
      term.loadAddon(new window.WebLinksAddon.WebLinksAddon());
    }

    term.open(bodyEl);

    // ResizeObserver on pane body to auto-fit on any size change
    const resizeObserver = new ResizeObserver(() => {
      requestAnimationFrame(() => {
        try {
          fitAddon.fit();
          if (ws && ws.readyState === WebSocket.OPEN) {
            ws.send(JSON.stringify({
              type: 'resize',
              id,
              cols: term.cols,
              rows: term.rows
            }));
          }
        } catch (e) {}
      });
    });
    resizeObserver.observe(bodyEl);

    // Send keystrokes
    term.onData((data) => {
      if (!ws || ws.readyState !== WebSocket.OPEN) return;
      if (broadcastMode) {
        ws.send(JSON.stringify({ type: 'broadcast', data }));
      } else {
        ws.send(JSON.stringify({ type: 'data', id, data }));
      }
    });

    // Handle Title Editing
    const titleGroup = pane.querySelector('.pane-title-group');
    const titleSpan = pane.querySelector('.pane-title');
    titleGroup.addEventListener('click', (e) => {
      if (pane.querySelector('.pane-title-input')) return;
      const current = titleSpan.textContent;
      const input = document.createElement('input');
      input.type = 'text';
      input.className = 'pane-title-input';
      input.value = current;
      titleSpan.replaceWith(input);
      input.focus();
      input.select();

      const finishEdit = () => {
        const val = input.value.trim() || current;
        titleSpan.textContent = val;
        if (input.parentNode) input.replaceWith(titleSpan);
        const item = terminals.get(id);
        if (item) item.title = val;
      };

      input.addEventListener('blur', finishEdit);
      input.addEventListener('keydown', (ev) => {
        if (ev.key === 'Enter') finishEdit();
        if (ev.key === 'Escape') {
          titleSpan.textContent = current;
          if (input.parentNode) input.replaceWith(titleSpan);
        }
      });
    });

    // Pane Controls
    pane.querySelector('.btn-zoom').addEventListener('click', () => toggleZoom(id));
    pane.querySelector('.btn-clear').addEventListener('click', () => term.clear());
    pane.querySelector('.btn-restart').addEventListener('click', () => restartTerminal(id));
    pane.querySelector('.btn-close').addEventListener('click', () => closeTerminal(id));

    // Focus tracking
    pane.addEventListener('mousedown', () => setFocus(id));
    term.textarea.addEventListener('focus', () => setFocus(id));

    // Store in Map
    const item = {
      id,
      title: termTitle,
      term,
      fitAddon,
      element: pane,
      isMaximized: false,
      resizeObserver
    };
    terminals.set(id, item);

    // Initial fit & create on server
    setTimeout(() => {
      fitAddon.fit();
      if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({
          type: 'create',
          id,
          title: termTitle,
          cols: term.cols,
          rows: term.rows
        }));
      }
      setFocus(id);
    }, 50);

    updateLayout();
    return id;
  }

  // Set focus on a terminal pane
  function setFocus(id) {
    if (focusedTerminalId === id) return;
    focusedTerminalId = id;
    for (const [tId, item] of terminals.entries()) {
      if (tId === id) {
        item.element.classList.add('is-focused');
        item.term.focus();
      } else {
        item.element.classList.remove('is-focused');
      }
    }
  }

  // Toggle Zoom / Maximize on a single pane
  function toggleZoom(id) {
    const item = terminals.get(id);
    if (!item) return;

    if (item.isMaximized) {
      item.isMaximized = false;
      item.element.classList.remove('is-maximized');
      item.element.querySelector('.btn-zoom').classList.remove('is-active');
    } else {
      // Un-maximize any other pane first
      for (const other of terminals.values()) {
        if (other.isMaximized) {
          other.isMaximized = false;
          other.element.classList.remove('is-maximized');
          other.element.querySelector('.btn-zoom').classList.remove('is-active');
        }
      }
      item.isMaximized = true;
      item.element.classList.add('is-maximized');
      item.element.querySelector('.btn-zoom').classList.add('is-active');
    }

    setFocus(id);
    setTimeout(() => {
      item.fitAddon.fit();
      if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({
          type: 'resize',
          id,
          cols: item.term.cols,
          rows: item.term.rows
        }));
      }
    }, 50);
  }

  // Restart a terminal shell
  function restartTerminal(id) {
    const item = terminals.get(id);
    if (!item) return;
    item.term.write('\r\n\x1b[33m[Restarting shell...]\x1b[0m\r\n');
    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify({
        type: 'restart',
        id,
        cols: item.term.cols,
        rows: item.term.rows
      }));
    }
  }

  // Close a terminal pane
  function closeTerminal(id) {
    const item = terminals.get(id);
    if (!item) return;

    if (ws && ws.readyState === WebSocket.OPEN) {
      ws.send(JSON.stringify({ type: 'kill', id }));
    }

    try {
      item.resizeObserver.disconnect();
      item.fitAddon.dispose();
      item.term.dispose();
    } catch (e) {}

    item.element.remove();
    terminals.delete(id);

    if (focusedTerminalId === id) {
      const remaining = Array.from(terminals.keys());
      if (remaining.length > 0) {
        setFocus(remaining[remaining.length - 1]);
      } else {
        focusedTerminalId = null;
      }
    }

    updateLayout();
  }

  // Update layout and counts
  function updateLayout() {
    const count = terminals.size;
    gridContainer.dataset.count = Math.min(count, 9);
    termCounterBadge.textContent = `${count} ${count === 1 ? 'Terminal' : 'Terminals'}`;

    if (count === 0) {
      emptyState.classList.remove('hidden');
    } else {
      emptyState.classList.add('hidden');
    }

    // Refit all terminals after DOM change
    setTimeout(() => {
      for (const item of terminals.values()) {
        try {
          item.fitAddon.fit();
        } catch (e) {}
      }
    }, 100);
  }

  // Set grid layout mode
  function setLayoutMode(mode) {
    currentLayout = mode;
    gridContainer.className = '';
    if (mode === 'auto') {
      gridContainer.classList.add('grid-auto');
    } else {
      gridContainer.classList.add(`grid-col-${mode}`);
    }

    segButtons.forEach((btn) => {
      btn.classList.toggle('active', btn.dataset.layout === mode);
    });

    // Unmaximize any zoomed panes when switching layout
    for (const item of terminals.values()) {
      if (item.isMaximized) {
        toggleZoom(item.id);
      }
    }

    updateLayout();
  }

  // Toggle Broadcast Mode
  function toggleBroadcast() {
    broadcastMode = !broadcastMode;
    document.body.classList.toggle('broadcast-active', broadcastMode);
    broadcastToggle.classList.toggle('active', broadcastMode);
    broadcastBanner.classList.toggle('hidden', !broadcastMode);
  }

  // Helpers
  function escapeHtml(str) {
    return str.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;').replace(/"/g, '&quot;');
  }

  // Event Listeners
  addTerminalBtn.addEventListener('click', () => createTerminal());
  emptyAddBtn.addEventListener('click', () => createTerminal());

  broadcastToggle.addEventListener('click', toggleBroadcast);
  broadcastOffBtn.addEventListener('click', toggleBroadcast);

  segButtons.forEach((btn) => {
    btn.addEventListener('click', () => {
      setLayoutMode(btn.dataset.layout);
    });
  });

  // Global Keyboard Shortcuts
  window.addEventListener('keydown', (e) => {
    // Alt + N: New terminal
    if (e.altKey && (e.key === 'n' || e.key === 'N')) {
      e.preventDefault();
      createTerminal();
      return;
    }

    // Alt + B: Toggle broadcast
    if (e.altKey && (e.key === 'b' || e.key === 'B')) {
      e.preventDefault();
      toggleBroadcast();
      return;
    }

    // Alt + Z: Toggle zoom for focused pane
    if (e.altKey && (e.key === 'z' || e.key === 'Z')) {
      e.preventDefault();
      if (focusedTerminalId) {
        toggleZoom(focusedTerminalId);
      }
      return;
    }

    // Alt + W: Close focused pane
    if (e.altKey && (e.key === 'w' || e.key === 'W')) {
      e.preventDefault();
      if (focusedTerminalId) {
        closeTerminal(focusedTerminalId);
      }
      return;
    }

    // Escape: un-zoom if any pane is maximized
    if (e.key === 'Escape') {
      for (const item of terminals.values()) {
        if (item.isMaximized) {
          toggleZoom(item.id);
          return;
        }
      }
    }
  });

  // Window resize handler
  window.addEventListener('resize', () => {
    for (const item of terminals.values()) {
      try {
        item.fitAddon.fit();
      } catch (e) {}
    }
  });

  // Start app
  initWebSocket();
})();
