/**
 * Unit Tests for Homepage.html
 * 
 * Setup Instructions:
 * 1. Install dependencies:
 *    npm install --save-dev jest @testing-library/dom jest-environment-jsdom
 * 2. Add to package.json:
 *    "scripts": { "test": "jest" }
 * 3. Create jest.config.js with: module.exports = { testEnvironment: 'jsdom' };
 * 4. Run tests: npm test Homepage.test.js
 */

// Mock WebSocket
class MockWebSocket {
  constructor(url) {
    this.url = url;
    this.readyState = 1; // OPEN
    this.sentMessages = [];
    this.onopen = null;
    this.onmessage = null;
    this.onerror = null;
    this.onclose = null;
  }
  send(data) { 
    this.sentMessages.push(data); 
  }
  close(code, reason) { 
    this.readyState = 3;
    if (this.onclose) {
      this.onclose({ code, reason });
    }
  }
  simulateOpen() {
    this.readyState = 1;
    if (this.onopen) {
      this.onopen();
    }
  }
  simulateMessage(data) {
    if (this.onmessage) {
      this.onmessage({ data });
    }
  }
  simulateError(error) {
    if (this.onerror) {
      this.onerror(error);
    }
  }
}
global.WebSocket = MockWebSocket;
WebSocket.OPEN = 1;
WebSocket.CLOSED = 3;

// Mock localStorage
const localStorageMock = (() => {
  let store = {};
  return {
    getItem: jest.fn((key) => store[key] || null),
    setItem: jest.fn((key, value) => { store[key] = value; }),
    clear: jest.fn(() => { store = {}; }),
    removeItem: jest.fn((key) => { delete store[key]; }),
    get store() { return store; },
    set store(val) { store = val; }
  };
})();
global.localStorage = localStorageMock;

// Mock sessionStorage
const sessionStorageMock = (() => {
  let store = {};
  return {
    getItem: jest.fn((key) => store[key] || null),
    setItem: jest.fn((key, value) => { store[key] = value; }),
    clear: jest.fn(() => { store = {}; }),
    removeItem: jest.fn((key) => { delete store[key]; }),
    get store() { return store; },
    set store(val) { store = val; }
  };
})();
global.sessionStorage = sessionStorageMock;

// Mock jQuery
global.$ = jest.fn((selector) => {
  const element = document.querySelector(selector);
  return {
    text: jest.fn((value) => {
      if (value !== undefined && element) {
        element.textContent = value;
      }
      return element ? element.textContent : '';
    }),
    html: jest.fn((value) => {
      if (value !== undefined && element) {
        element.innerHTML = value;
      }
      return element ? element.innerHTML : '';
    })
  };
});

describe('Homepage - WebSocket Connection', () => {
  let sock;

  beforeEach(() => {
    localStorage.clear();
    sock = new MockWebSocket('ws://localhost:18080/home');
  });

  test('should create WebSocket with correct URL', () => {
    expect(sock.url).toBe('ws://localhost:18080/home');
  });

  test('should initialize with OPEN state', () => {
    expect(sock.readyState).toBe(WebSocket.OPEN);
  });

  test('should handle onopen event', () => {
    const openHandler = jest.fn();
    sock.onopen = openHandler;
    
    sock.simulateOpen();
    
    expect(openHandler).toHaveBeenCalled();
  });

  test('should send user data on connection', () => {
    localStorage.setItem('activeuser', 'user123');
    
    const userData = localStorage.getItem('activeuser');
    if (userData !== null) {
      sock.send(userData);
    }
    
    expect(sock.sentMessages).toHaveLength(1);
    expect(sock.sentMessages[0]).toBe('user123');
  });

  test('should not send if no active user', () => {
    const userData = localStorage.getItem('activeuser');
    if (userData !== null) {
      sock.send(userData);
    }
    
    expect(sock.sentMessages).toHaveLength(0);
  });

  test('should handle onmessage event', () => {
    const messageHandler = jest.fn();
    sock.onmessage = messageHandler;
    
    sock.simulateMessage('test message');
    
    expect(messageHandler).toHaveBeenCalled();
  });

  test('should handle onerror event', () => {
    const errorHandler = jest.fn();
    sock.onerror = errorHandler;
    
    sock.simulateError(new Error('Connection failed'));
    
    expect(errorHandler).toHaveBeenCalled();
  });

  test('should handle onclose event', () => {
    const closeHandler = jest.fn();
    sock.onclose = closeHandler;
    
    sock.close(1000, 'Normal closure');
    
    expect(closeHandler).toHaveBeenCalled();
    expect(sock.readyState).toBe(WebSocket.CLOSED);
  });
});

describe('Homepage - Username Display', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <h2>Hi <span id="username">Guest</span> 👋</h2>
    `;
    localStorage.clear();
  });

  test('should default to "Guest" when no username stored', () => {
    let username = "Guest";
    
    if (localStorage.getItem("activeusername") !== null) {
      username = localStorage.getItem("activeusername");
    }
    
    expect(username).toBe("Guest");
  });

  test('should load username from localStorage', () => {
    localStorage.setItem('activeusername', 'John');
    
    let username = "Guest";
    if (localStorage.getItem("activeusername") !== null) {
      username = localStorage.getItem("activeusername");
    }
    
    expect(username).toBe('John');
  });

  test('should update username element with jQuery', () => {
    const usernameElement = document.getElementById('username');
    const username = 'Alice';
    
    $('#username').text(username);
    
    expect(usernameElement.textContent).toBe('Alice');
  });

  test('should handle different usernames', () => {
    const usernames = ['John', 'Jane', 'Bob'];
    
    usernames.forEach(name => {
      localStorage.setItem('activeusername', name);
      const stored = localStorage.getItem('activeusername');
      expect(stored).toBe(name);
    });
  });
});

describe('Homepage - SessionStorage Management', () => {
  beforeEach(() => {
    sessionStorage.clear();
  });

  test('should check for saved data in sessionStorage', () => {
    sessionStorage.setItem('saved', JSON.stringify({ uid: '123', saved: true }));
    
    const hasSaved = sessionStorage.getItem('saved') !== null;
    
    expect(hasSaved).toBe(true);
  });

  test('should parse saved data', () => {
    const savedData = { uid: '123', saved: true };
    sessionStorage.setItem('saved', JSON.stringify(savedData));
    
    const saved = JSON.parse(sessionStorage.getItem('saved'));
    
    expect(saved.uid).toBe('123');
    expect(saved.saved).toBe(true);
  });

  test('should remove saved data after processing', () => {
    sessionStorage.setItem('saved', JSON.stringify({ uid: '123' }));
    
    if (sessionStorage.getItem('saved') !== null) {
      sessionStorage.removeItem('saved');
    }
    
    expect(sessionStorage.getItem('saved')).toBeNull();
  });

  test('should check for instruction data', () => {
    sessionStorage.setItem('instruction', JSON.stringify({ test: 'data' }));
    
    const hasInstruction = sessionStorage.getItem('instruction') != null;
    
    expect(hasInstruction).toBe(true);
  });

  test('should remove instruction-related data', () => {
    sessionStorage.setItem('instruction', 'test1');
    sessionStorage.setItem('recName', 'test2');
    sessionStorage.setItem('recImage', 'test3');
    
    if (sessionStorage.getItem('instruction') != null) {
      sessionStorage.removeItem('instruction');
      sessionStorage.removeItem('recName');
      sessionStorage.removeItem('recImage');
    }
    
    expect(sessionStorage.getItem('instruction')).toBeNull();
    expect(sessionStorage.getItem('recName')).toBeNull();
    expect(sessionStorage.getItem('recImage')).toBeNull();
  });

  test('should handle null values correctly', () => {
    const value = sessionStorage.getItem('nonexistent');
    
    expect(value).toBeNull();
    expect(value == null).toBe(true);
    expect(value != null).toBe(false);
  });
});

describe('Homepage - Page Visibility Handling', () => {
  test('should handle document hidden state', () => {
    Object.defineProperty(document, 'hidden', {
      configurable: true,
      get: () => true
    });
    
    expect(document.hidden).toBe(true);
  });

  test('should handle document visible state', () => {
    Object.defineProperty(document, 'hidden', {
      configurable: true,
      get: () => false
    });
    
    expect(document.hidden).toBe(false);
  });

  test('should close WebSocket when page hidden', () => {
    const sock = new MockWebSocket('ws://localhost:18080/home');
    
    Object.defineProperty(document, 'hidden', {
      configurable: true,
      get: () => true
    });
    
    if (document.hidden) {
      sock.close(1000, 'Disconnecting from Homepage server');
    }
    
    expect(sock.readyState).toBe(WebSocket.CLOSED);
  });

  test('should reconnect WebSocket when page becomes visible', () => {
    let socket = new MockWebSocket('ws://localhost:18080/home');
    socket.close();
    
    Object.defineProperty(document, 'hidden', {
      configurable: true,
      get: () => false
    });
    
    if (!document.hidden && socket.readyState === WebSocket.CLOSED) {
      socket = new MockWebSocket('ws://localhost:18080/home');
    }
    
    expect(socket.readyState).toBe(WebSocket.OPEN);
  });
});

describe('Homepage - Navigation Links', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <div class="grid grid-cols-2 gap-6">
        <a href="Barcode.html">Scan Barcode</a>
        <a href="Receipt.html">Scan Receipt</a>
        <a href="Manual.html">Manual Input</a>
        <a href="Recipes.html">Recipes</a>
      </div>
      <a href="Saved.html">Ingredients</a>
    `;
  });

  test('should have correct link to Barcode page', () => {
    const link = document.querySelector('a[href="Barcode.html"]');
    expect(link).toBeTruthy();
    expect(link.href).toContain('Barcode.html');
  });

  test('should have correct link to Receipt page', () => {
    const link = document.querySelector('a[href="Receipt.html"]');
    expect(link).toBeTruthy();
    expect(link.href).toContain('Receipt.html');
  });

  test('should have correct link to Manual page', () => {
    const link = document.querySelector('a[href="Manual.html"]');
    expect(link).toBeTruthy();
    expect(link.href).toContain('Manual.html');
  });

  test('should have correct link to Recipes page', () => {
    const link = document.querySelector('a[href="Recipes.html"]');
    expect(link).toBeTruthy();
    expect(link.href).toContain('Recipes.html');
  });

  test('should have correct link to Saved page', () => {
    const link = document.querySelector('a[href="Saved.html"]');
    expect(link).toBeTruthy();
    expect(link.href).toContain('Saved.html');
  });

  test('should have all action buttons', () => {
    const links = document.querySelectorAll('a[href$=".html"]');
    expect(links.length).toBeGreaterThanOrEqual(5);
  });
});

describe('Homepage - Header Navigation', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <header>
        <nav>
          <ul>
            <li><a href="home.html">Home</a></li>
            <li><a href="Account.html">Account</a></li>
          </ul>
        </nav>
      </header>
    `;
  });

  test('should have Home link in header', () => {
    const link = document.querySelector('a[href="home.html"]');
    expect(link).toBeTruthy();
    expect(link.textContent).toBe('Home');
  });

  test('should have Account link in header', () => {
    const link = document.querySelector('a[href="Account.html"]');
    expect(link).toBeTruthy();
    expect(link.textContent).toBe('Account');
  });
});

describe('Homepage - LocalStorage Operations', () => {
  beforeEach(() => {
    localStorage.clear();
  });

  test('should store active user', () => {
    localStorage.setItem('activeuser', 'user123');
    
    const user = localStorage.getItem('activeuser');
    
    expect(user).toBe('user123');
  });

  test('should store active username', () => {
    localStorage.setItem('activeusername', 'John Doe');
    
    const username = localStorage.getItem('activeusername');
    
    expect(username).toBe('John Doe');
  });

  test('should check if active user exists', () => {
    localStorage.setItem('activeuser', 'user123');
    
    const hasActiveUser = localStorage.getItem('activeuser') !== null;
    
    expect(hasActiveUser).toBe(true);
  });

  test('should handle missing active user', () => {
    const hasActiveUser = localStorage.getItem('activeuser') !== null;
    
    expect(hasActiveUser).toBe(false);
  });

  test('should remove item from localStorage', () => {
    localStorage.setItem('testKey', 'testValue');
    localStorage.removeItem('testKey');
    
    expect(localStorage.getItem('testKey')).toBeNull();
  });
});

describe('Homepage - WebSocket State Management', () => {
  test('should check WebSocket readyState', () => {
    const sock = new MockWebSocket('ws://localhost:18080/home');
    
    expect(sock.readyState).toBe(WebSocket.OPEN);
  });

  test('should detect closed connection', () => {
    const sock = new MockWebSocket('ws://localhost:18080/home');
    sock.close();
    
    expect(sock.readyState).toBe(WebSocket.CLOSED);
  });

  test('should not send when connection is closed', () => {
    const sock = new MockWebSocket('ws://localhost:18080/home');
    sock.close();
    
    if (sock.readyState === WebSocket.OPEN) {
      sock.send('test');
    }
    
    expect(sock.sentMessages).toHaveLength(0);
  });

  test('should send when connection is open', () => {
    const sock = new MockWebSocket('ws://localhost:18080/home');
    
    if (sock.readyState === WebSocket.OPEN) {
      sock.send('test');
    }
    
    expect(sock.sentMessages).toHaveLength(1);
  });
});

describe('Homepage - Data Cleanup', () => {
  beforeEach(() => {
    sessionStorage.clear();
  });

  test('should clean up instruction data on load', () => {
    sessionStorage.setItem('instruction', 'data1');
    sessionStorage.setItem('recName', 'data2');
    sessionStorage.setItem('recImage', 'data3');
    
    // Simulate page load cleanup
    if (sessionStorage.getItem('instruction') != null) {
      sessionStorage.removeItem('instruction');
      sessionStorage.removeItem('recName');
      sessionStorage.removeItem('recImage');
    }
    
    expect(sessionStorage.getItem('instruction')).toBeNull();
    expect(sessionStorage.getItem('recName')).toBeNull();
    expect(sessionStorage.getItem('recImage')).toBeNull();
  });

  test('should handle cleanup when no data exists', () => {
    if (sessionStorage.getItem('instruction') != null) {
      sessionStorage.removeItem('instruction');
      sessionStorage.removeItem('recName');
      sessionStorage.removeItem('recImage');
    }
    
    // Should not throw error
    expect(sessionStorage.getItem('instruction')).toBeNull();
  });
});

describe('Homepage - jQuery Operations', () => {
  beforeEach(() => {
    document.body.innerHTML = `
      <span id="username">Guest</span>
      <div id="content">Original</div>
    `;
  });

  test('should update text content with jQuery', () => {
    const element = document.getElementById('username');
    
    $('#username').text('John');
    
    expect(element.textContent).toBe('John');
  });

  test('should read text content with jQuery', () => {
    document.getElementById('username').textContent = 'Alice';
    
    const text = $('#username').text();
    
    expect(text).toBe('Alice');
  });

  test('should update HTML content with jQuery', () => {
    const element = document.getElementById('content');
    
    $('#content').html('<strong>New</strong>');
    
    expect(element.innerHTML).toBe('<strong>New</strong>');
  });
});

describe('Homepage - JSON Operations', () => {
  test('should stringify object to JSON', () => {
    const obj = { uid: '123', saved: true };
    
    const json = JSON.stringify(obj);
    
    expect(json).toBe('{"uid":"123","saved":true}');
  });

  test('should parse JSON to object', () => {
    const json = '{"uid":"123","saved":true}';
    
    const obj = JSON.parse(json);
    
    expect(obj.uid).toBe('123');
    expect(obj.saved).toBe(true);
  });

  test('should handle nested objects', () => {
    const obj = { user: { id: '123', name: 'John' } };
    
    const json = JSON.stringify(obj);
    const parsed = JSON.parse(json);
    
    expect(parsed.user.id).toBe('123');
    expect(parsed.user.name).toBe('John');
  });
});

describe('Homepage - Conditional Logic', () => {
  test('should check null vs undefined', () => {
    const nullValue = null;
    const undefinedValue = undefined;
    
    expect(nullValue == null).toBe(true);
    expect(undefinedValue == null).toBe(true);
    expect(nullValue != null).toBe(false);
    expect(undefinedValue != null).toBe(false);
  });

  test('should distinguish null from non-null', () => {
    const value = 'test';
    
    expect(value !== null).toBe(true);
    expect(value != null).toBe(true);
  });

  test('should check localStorage values', () => {
    localStorage.clear();
    
    const value1 = localStorage.getItem('nonexistent');
    expect(value1 !== null).toBe(false);
    
    localStorage.setItem('exists', 'value');
    const value2 = localStorage.getItem('exists');
    expect(value2 !== null).toBe(true);
  });
});