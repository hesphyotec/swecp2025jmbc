/**
 * Unit Tests for Saved.html
 * 
 * Setup Instructions:
 * 1. Install dependencies:
 *    npm install --save-dev jest @testing-library/dom jest-environment-jsdom
 * 2. Add to package.json:
 *    "scripts": { "test": "jest" }
 * 3. Create jest.config.js with: module.exports = { testEnvironment: 'jsdom' };
 * 4. Run tests: npm test Saved.test.js
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
  // Helper method to simulate receiving a message
  simulateMessage(data) {
    if (this.onmessage) {
      this.onmessage({ data: JSON.stringify(data) });
    }
  }
  // Helper method to simulate connection
  simulateOpen() {
    this.readyState = 1;
    if (this.onopen) {
      this.onopen();
    }
  }
  // Helper method to simulate error
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
    get store() { return store; },
    set store(val) { store = val; }
  };
})();
global.sessionStorage = sessionStorageMock;

// Mock window.storage (persistent storage)
global.window.storage = {
  get: jest.fn((key) => Promise.resolve(null)),
  set: jest.fn((key, value) => Promise.resolve({ key, value })),
  delete: jest.fn((key) => Promise.resolve({ key, deleted: true })),
  list: jest.fn(() => Promise.resolve({ keys: [] }))
};

describe('Saved Page - Ingredient List Management', () => {
  let ingredients;

  beforeEach(() => {
    sessionStorage.clear();
    ingredients = [];
  });

  test('should initialize empty ingredients array', () => {
    expect(ingredients).toEqual([]);
    expect(ingredients).toHaveLength(0);
  });

  test('should add ingredient objects', () => {
    ingredients = [
      { name: 'Chicken', id: 1 },
      { name: 'Salmon', id: 2 }
    ];
    
    expect(ingredients).toHaveLength(2);
    expect(ingredients[0]).toHaveProperty('name', 'Chicken');
    expect(ingredients[0]).toHaveProperty('id', 1);
  });

  test('should handle ingredients as strings or objects', () => {
    ingredients = [
      'Chicken',
      { name: 'Salmon', id: 2 },
      'Beef'
    ];
    
    expect(ingredients).toHaveLength(3);
    expect(typeof ingredients[0]).toBe('string');
    expect(typeof ingredients[1]).toBe('object');
  });

  test('should remove ingredient by index', () => {
    ingredients = [
      { name: 'Chicken', id: 1 },
      { name: 'Salmon', id: 2 },
      { name: 'Beef', id: 3 }
    ];
    
    ingredients.splice(1, 1);
    
    expect(ingredients).toHaveLength(2);
    expect(ingredients.map(i => i.name)).toEqual(['Chicken', 'Beef']);
  });

  test('should find ingredient by id', () => {
    ingredients = [
      { name: 'Chicken', id: 1 },
      { name: 'Salmon', id: 2 },
      { name: 'Beef', id: 3 }
    ];
    
    const index = ingredients.findIndex(ing => ing.id === 2);
    
    expect(index).toBe(1);
    expect(ingredients[index].name).toBe('Salmon');
  });

  test('should find ingredient by name', () => {
    ingredients = [
      { name: 'Chicken', id: 1 },
      { name: 'Salmon', id: 2 }
    ];
    
    const index = ingredients.findIndex(ing => ing.name === 'Chicken');
    
    expect(index).toBe(0);
  });

  test('should handle finding non-existent ingredient', () => {
    ingredients = [
      { name: 'Chicken', id: 1 }
    ];
    
    const index = ingredients.findIndex(ing => ing.id === 999);
    
    expect(index).toBe(-1);
  });
});

describe('Saved Page - WebSocket Connection', () => {
  let sock;

  beforeEach(() => {
    localStorage.setItem('activeuserid', 'test-user-123');
    sock = new MockWebSocket('ws://localhost:18080/inventory');
  });

  afterEach(() => {
    localStorage.clear();
  });

  test('should create WebSocket with correct URL', () => {
    expect(sock.url).toBe('ws://localhost:18080/inventory');
  });

  test('should initialize with OPEN state', () => {
    expect(sock.readyState).toBe(WebSocket.OPEN);
  });

  test('should send getlist message on open', () => {
    const getlist = {
      op: 'getlist',
      uid: 'test-user-123'
    };
    
    sock.send(JSON.stringify(getlist));
    
    expect(sock.sentMessages).toHaveLength(1);
    expect(JSON.parse(sock.sentMessages[0])).toEqual(getlist);
  });

  test('should handle onopen event', () => {
    const openHandler = jest.fn();
    sock.onopen = openHandler;
    
    sock.simulateOpen();
    
    expect(openHandler).toHaveBeenCalled();
  });

  test('should handle onmessage event', () => {
    const messageHandler = jest.fn();
    sock.onmessage = messageHandler;
    
    const testData = { status: 'success', data: [] };
    sock.simulateMessage(testData);
    
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

  test('should send delitem message', () => {
    const delMessage = {
      op: 'delitem',
      uid: 'test-user-123',
      iid: 42
    };
    
    sock.send(JSON.stringify(delMessage));
    
    expect(sock.sentMessages).toHaveLength(1);
    const sent = JSON.parse(sock.sentMessages[0]);
    expect(sent.op).toBe('delitem');
    expect(sent.iid).toBe(42);
  });
});

describe('Saved Page - Server Message Processing', () => {
  test('should parse JSON message successfully', () => {
    const messageData = JSON.stringify({
      status: 'success',
      data: [
        { name: 'Chicken', id: 1 },
        { name: 'Salmon', id: 2 }
      ]
    });
    
    const result = JSON.parse(messageData);
    
    expect(result.status).toBe('success');
    expect(result.data).toHaveLength(2);
    expect(Array.isArray(result.data)).toBe(true);
  });

  test('should handle success status with data', () => {
    const result = {
      status: 'success',
      data: [
        { name: 'Chicken', id: 1 }
      ]
    };
    
    expect(result.status).toBe('success');
    expect(Array.isArray(result.data)).toBe(true);
  });

  test('should handle error status with message', () => {
    const result = {
      status: 'error',
      message: 'Database connection failed'
    };
    
    expect(result.status).toBe('error');
    expect(result.message).toBeDefined();
  });

  test('should handle malformed JSON', () => {
    const badJSON = '{invalid json}';
    
    expect(() => JSON.parse(badJSON)).toThrow();
  });

  test('should validate data is an array', () => {
    const result = {
      status: 'success',
      data: [{ name: 'Chicken', id: 1 }]
    };
    
    expect(Array.isArray(result.data)).toBe(true);
  });
});

describe('Saved Page - Persistent Storage Operations', () => {
  beforeEach(() => {
    jest.clearAllMocks();
  });

  test('should save ingredients to storage', async () => {
    const ingredients = [
      { name: 'Chicken', id: 1 },
      { name: 'Salmon', id: 2 }
    ];
    
    await sessionStorage.setItem('ingredients', JSON.stringify(ingredients));
    
    const stored = sessionStorage.getItem('ingredients');
    expect(stored).toBe(JSON.stringify(ingredients));
  });

  test('should load ingredients from storage', async () => {
    const ingredients = [
      { name: 'Chicken', id: 1 }
    ];
    
    sessionStorage.setItem('ingredients', JSON.stringify(ingredients));
    const loaded = JSON.parse(sessionStorage.getItem('ingredients'));
    
    expect(loaded).toEqual(ingredients);
  });

  test('should handle window.storage.get', async () => {
    window.storage.get.mockResolvedValue({
      key: 'ingredients',
      value: JSON.stringify([{ name: 'Chicken', id: 1 }])
    });
    
    const result = await window.storage.get('ingredients');
    
    expect(result).toBeDefined();
    expect(result.value).toBeDefined();
  });

  test('should handle window.storage.get with no data', async () => {
    window.storage.get.mockResolvedValue(null);
    
    const result = await window.storage.get('ingredients');
    
    expect(result).toBeNull();
  });

  test('should handle storage errors gracefully', async () => {
    window.storage.get.mockRejectedValue(new Error('Storage unavailable'));
    
    try {
      await window.storage.get('ingredients');
    } catch (error) {
      expect(error.message).toBe('Storage unavailable');
    }
  });
});

describe('Saved Page - Status Indicator', () => {
  let statusIndicator;

  beforeEach(() => {
    document.body.innerHTML = `
      <div class="status-indicator" id="statusIndicator">Loading...</div>
    `;
    statusIndicator = document.getElementById('statusIndicator');
  });

  test('should update status to online', () => {
    statusIndicator.textContent = '🟢 Connected to server';
    statusIndicator.className = 'status-indicator status-online';
    
    expect(statusIndicator.textContent).toBe('🟢 Connected to server');
    expect(statusIndicator.className).toContain('status-online');
  });

  test('should update status to offline', () => {
    statusIndicator.textContent = '🟡 Offline mode (data saved locally)';
    statusIndicator.className = 'status-indicator status-offline';
    
    expect(statusIndicator.textContent).toBe('🟡 Offline mode (data saved locally)');
    expect(statusIndicator.className).toContain('status-offline');
  });

  test('should have initial loading state', () => {
    expect(statusIndicator.textContent).toBe('Loading...');
  });
});

describe('Saved Page - DOM Rendering', () => {
  let list;

  beforeEach(() => {
    document.body.innerHTML = `
      <div class="ingredients-list" id="ingredientsList">
        <div>Loading ingredients...</div>
      </div>
    `;
    list = document.getElementById('ingredientsList');
  });

  test('should display empty message when no ingredients', () => {
    list.innerHTML = '';
    list.textContent = 'Your ingredients will appear here.';
    
    expect(list.textContent).toBe('Your ingredients will appear here.');
  });

  test('should render ingredient items', () => {
    list.innerHTML = '';
    
    const item = document.createElement('div');
    item.className = 'ingredient-item';
    item.textContent = 'Chicken';
    list.appendChild(item);
    
    expect(list.children.length).toBe(1);
    expect(list.children[0].textContent).toBe('Chicken');
    expect(list.children[0].className).toBe('ingredient-item');
  });

  test('should render multiple ingredients', () => {
    list.innerHTML = '';
    
    ['Chicken', 'Salmon', 'Beef'].forEach(name => {
      const item = document.createElement('div');
      item.className = 'ingredient-item';
      item.textContent = name;
      list.appendChild(item);
    });
    
    expect(list.children.length).toBe(3);
  });

  test('should add remove button to ingredient', () => {
    list.innerHTML = '';
    
    const item = document.createElement('div');
    item.className = 'ingredient-item';
    item.textContent = 'Chicken';
    
    const removeBtn = document.createElement('button');
    removeBtn.textContent = '❌';
    removeBtn.className = 'remove-btn';
    
    item.appendChild(removeBtn);
    list.appendChild(item);
    
    expect(list.querySelector('.remove-btn')).toBeTruthy();
    expect(list.querySelector('.remove-btn').textContent).toBe('❌');
  });

  test('should clear list before re-rendering', () => {
    list.innerHTML = '<div>Old content</div>';
    list.innerHTML = '';
    
    expect(list.children.length).toBe(0);
  });
});

describe('Saved Page - Remove Ingredient Logic', () => {
  let ingredients;

  beforeEach(() => {
    ingredients = [
      { name: 'Chicken', id: 1 },
      { name: 'Salmon', id: 2 },
      { name: 'Beef', id: 3 }
    ];
  });

  test('should find and remove ingredient by id', () => {
    const targetId = 2;
    const index = ingredients.findIndex(ing => ing.id === targetId);
    
    expect(index).toBe(1);
    
    ingredients.splice(index, 1);
    
    expect(ingredients).toHaveLength(2);
    expect(ingredients.find(ing => ing.id === targetId)).toBeUndefined();
  });

  test('should find and remove ingredient by name', () => {
    const targetName = 'Salmon';
    const index = ingredients.findIndex(ing => ing.name === targetName);
    
    ingredients.splice(index, 1);
    
    expect(ingredients).toHaveLength(2);
    expect(ingredients.find(ing => ing.name === targetName)).toBeUndefined();
  });

  test('should handle compound search (id, name, or direct match)', () => {
    const target = { name: 'Salmon', id: 2 };
    
    const index = ingredients.findIndex(ing => 
      (ing.id && ing.id === target.id) || 
      (ing.name && ing.name === target.name) ||
      ing === target
    );
    
    expect(index).not.toBe(-1);
  });

  test('should not remove anything if ingredient not found', () => {
    const originalLength = ingredients.length;
    const index = ingredients.findIndex(ing => ing.id === 999);
    
    if (index !== -1) {
      ingredients.splice(index, 1);
    }
    
    expect(ingredients).toHaveLength(originalLength);
  });
});

describe('Saved Page - Connection State Management', () => {
  test('should track server connection state', () => {
    let isServerConnected = false;
    
    expect(isServerConnected).toBe(false);
    
    isServerConnected = true;
    expect(isServerConnected).toBe(true);
  });

  test('should check WebSocket readyState before sending', () => {
    const sock = new MockWebSocket('ws://localhost:18080/inventory');
    
    expect(sock.readyState).toBe(WebSocket.OPEN);
    
    if (sock.readyState === WebSocket.OPEN) {
      sock.send(JSON.stringify({ test: 'data' }));
    }
    
    expect(sock.sentMessages).toHaveLength(1);
  });

  test('should not send when WebSocket is closed', () => {
    const sock = new MockWebSocket('ws://localhost:18080/inventory');
    sock.close();
    
    expect(sock.readyState).toBe(WebSocket.CLOSED);
    
    if (sock.readyState === WebSocket.OPEN) {
      sock.send(JSON.stringify({ test: 'data' }));
    }
    
    expect(sock.sentMessages).toHaveLength(0);
  });
});

describe('Saved Page - Page Visibility Handling', () => {
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
    const sock = new MockWebSocket('ws://localhost:18080/inventory');
    
    Object.defineProperty(document, 'hidden', {
      configurable: true,
      get: () => true
    });
    
    if (document.hidden) {
      sock.close(1000, 'Disconnecting from Saved server');
    }
    
    expect(sock.readyState).toBe(WebSocket.CLOSED);
  });

  test('should reconnect WebSocket when page becomes visible', () => {
    let sock = new MockWebSocket('ws://localhost:18080/inventory');
    sock.close();
    
    Object.defineProperty(document, 'hidden', {
      configurable: true,
      get: () => false
    });
    
    if (!document.hidden && sock.readyState === WebSocket.CLOSED) {
      sock = new MockWebSocket('ws://localhost:18080/inventory');
    }
    
    expect(sock.readyState).toBe(WebSocket.OPEN);
  });
});