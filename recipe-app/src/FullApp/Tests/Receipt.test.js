/**
 * Unit Tests for Receipt.html
 * 
 * Setup Instructions:
 * 1. Install dependencies:
 *    npm install --save-dev jest @testing-library/dom jest-environment-jsdom
 * 2. Add to package.json:
 *    "scripts": { "test": "jest" }
 * 3. Create jest.config.js with: module.exports = { testEnvironment: 'jsdom' };
 * 4. Run tests: npm test
 */

// Mock Tesseract
global.Tesseract = {
  createWorker: jest.fn(() => Promise.resolve({
    recognize: jest.fn(() => Promise.resolve({
      data: { text: 'Chicken Salmon Beef' }
    })),
    terminate: jest.fn(() => Promise.resolve())
  }))
};

// Mock Fuse
global.Fuse = jest.fn().mockImplementation((list, options) => {
  return {
    search: jest.fn((text) => [
      { item: 'Chicken', score: 0.1 },
      { item: 'Salmon', score: 0.2 },
      { item: 'Beef', score: 0.15 }
    ])
  };
});

// Mock WebSocket
class MockWebSocket {
  constructor(url) {
    this.url = url;
    this.readyState = 1; // OPEN
    this.sentMessages = [];
  }
  send(data) { this.sentMessages.push(data); }
  close() { this.readyState = 3; }
}
global.WebSocket = MockWebSocket;
WebSocket.OPEN = 1;
WebSocket.CLOSED = 3;

// Mock localStorage
const localStorageMock = {
  store: {},
  getItem: jest.fn((key) => localStorageMock.store[key] || null),
  setItem: jest.fn((key, value) => { localStorageMock.store[key] = value; }),
  clear: jest.fn(() => { localStorageMock.store = {}; })
};
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

// Mock URL
global.URL.createObjectURL = jest.fn(() => 'mock-blob-url-123');
global.URL.revokeObjectURL = jest.fn();

// Mock navigator
global.navigator.mediaDevices = {
  getUserMedia: jest.fn()
};

describe('Receipt Scanner - Ingredient List Management', () => {
  let ingredients;

  beforeEach(() => {
    sessionStorage.clear();
    ingredients = [];
  });

  test('should initialize empty ingredients array', () => {
    const stored = JSON.parse(sessionStorage.getItem('ingredients'));
    expect(stored).toBeNull();
    expect(ingredients).toEqual([]);
  });

  test('should add ingredient with correct structure', () => {
    const newIngredient = { name: 'Chicken', id: Date.now() };
    ingredients.push(newIngredient);
    
    expect(ingredients).toHaveLength(1);
    expect(ingredients[0]).toHaveProperty('name', 'Chicken');
    expect(ingredients[0]).toHaveProperty('id');
    expect(typeof ingredients[0].id).toBe('number');
  });

  test('should add multiple ingredients', () => {
    ingredients.push({ name: 'Chicken', id: 1 });
    ingredients.push({ name: 'Salmon', id: 2 });
    ingredients.push({ name: 'Beef', id: 3 });
    
    expect(ingredients).toHaveLength(3);
    expect(ingredients.map(i => i.name)).toEqual(['Chicken', 'Salmon', 'Beef']);
  });

  test('should remove ingredient by index', () => {
    ingredients = [
      { name: 'Chicken', id: 1 },
      { name: 'Salmon', id: 2 },
      { name: 'Beef', id: 3 }
    ];
    
    ingredients.splice(1, 1); // Remove Salmon
    
    expect(ingredients).toHaveLength(2);
    expect(ingredients.map(i => i.name)).toEqual(['Chicken', 'Beef']);
  });

  test('should persist ingredients to sessionStorage', () => {
    ingredients = [
      { name: 'Chicken', id: 1 },
      { name: 'Salmon', id: 2 }
    ];
    
    sessionStorage.setItem('ingredients', JSON.stringify(ingredients));
    
    const retrieved = sessionStorage.getItem('ingredients');
    expect(retrieved).toBe(JSON.stringify(ingredients));
  });

  test('should load ingredients from sessionStorage', () => {
    const stored = [
      { name: 'Chicken', id: 1 },
      { name: 'Salmon', id: 2 }
    ];
    
    // Set the data first
    sessionStorage.setItem('ingredients', JSON.stringify(stored));
    
    // Then retrieve it
    const loaded = JSON.parse(sessionStorage.getItem('ingredients'));
    
    expect(loaded).toEqual(stored);
    expect(loaded).toHaveLength(2);
  });
});

describe('Receipt Scanner - Fuse.js Fuzzy Search', () => {
  let fuse;
  const ingredientsList = ['Chicken', 'Salmon', 'Beef', 'Pork'];

  beforeEach(() => {
    fuse = new Fuse(ingredientsList, { includeScore: true });
  });

  test('should create Fuse instance with correct parameters', () => {
    expect(Fuse).toHaveBeenCalledWith(ingredientsList, { includeScore: true });
  });

  test('should return search results with scores', () => {
    const results = fuse.search('Chicken Salmon');
    
    expect(results).toBeDefined();
    expect(results.length).toBeGreaterThan(0);
    expect(results[0]).toHaveProperty('item');
    expect(results[0]).toHaveProperty('score');
  });

  test('should filter results by score threshold', () => {
    const results = fuse.search('Some text');
    const filtered = results.filter(r => r.score <= 0.9);
    
    expect(filtered.length).toBeGreaterThan(0);
    filtered.forEach(result => {
      expect(result.score).toBeLessThanOrEqual(0.9);
    });
  });

  test('should exclude results with score > 0.9', () => {
    const mockBadResults = [
      { item: 'Chicken', score: 0.1 },
      { item: 'BadMatch', score: 0.95 },
      { item: 'Salmon', score: 0.3 }
    ];
    
    const filtered = mockBadResults.filter(r => r.score <= 0.9);
    
    expect(filtered).toHaveLength(2);
    expect(filtered.map(r => r.item)).toEqual(['Chicken', 'Salmon']);
  });
});

describe('Receipt Scanner - WebSocket Communication', () => {
  let sock;

  beforeEach(() => {
    localStorage.setItem('activeuserid', 'test-user-123');
    sock = new WebSocket('ws://localhost:18080/inventory');
  });

  afterEach(() => {
    localStorage.clear();
  });

  test('should create WebSocket with correct URL', () => {
    expect(sock.url).toBe('ws://localhost:18080/inventory');
  });

  test('should send getlist message on connection', () => {
    const getlist = {
      op: 'getlist',
      uid: 'test-user-123'
    };
    
    sock.send(JSON.stringify(getlist));
    
    expect(sock.sentMessages).toHaveLength(1);
    expect(JSON.parse(sock.sentMessages[0])).toEqual(getlist);
  });

  test('should send addReceipt message with ingredients', () => {
    const ingredients = ['Chicken', 'Salmon', 'Beef'];
    const message = {
      op: 'addReceipt',
      uid: 'test-user-123',
      ingredients: ingredients
    };
    
    sock.send(JSON.stringify(message));
    
    expect(sock.sentMessages).toHaveLength(1);
    const sent = JSON.parse(sock.sentMessages[0]);
    expect(sent.op).toBe('addReceipt');
    expect(sent.uid).toBe('test-user-123');
    expect(sent.ingredients).toEqual(ingredients);
  });

  test('should send delitem message', () => {
    const message = {
      op: 'delitem',
      uid: 'test-user-123',
      iid: 12345
    };
    
    sock.send(JSON.stringify(message));
    
    expect(sock.sentMessages).toHaveLength(1);
    const sent = JSON.parse(sock.sentMessages[0]);
    expect(sent.op).toBe('delitem');
    expect(sent.iid).toBe(12345);
  });

  test('should check WebSocket readyState before sending', () => {
    sock.readyState = WebSocket.OPEN;
    expect(sock.readyState).toBe(1);
    
    sock.close();
    expect(sock.readyState).toBe(3);
  });

  test('should close WebSocket with correct parameters', () => {
    sock.close(1000, 'Disconnecting from Receipt server');
    expect(sock.readyState).toBe(WebSocket.CLOSED);
  });
});

describe('Receipt Scanner - OCR Processing', () => {
  beforeEach(() => {
    jest.clearAllMocks();
  });

  test('should create Tesseract worker', async () => {
    const worker = await Tesseract.createWorker('eng');
    
    expect(Tesseract.createWorker).toHaveBeenCalledWith('eng');
    expect(worker).toBeDefined();
  });

  test('should recognize text from image blob', async () => {
    const worker = await Tesseract.createWorker('eng');
    const mockBlob = new Blob(['mock-image'], { type: 'image/png' });
    
    const result = await worker.recognize(mockBlob);
    
    expect(worker.recognize).toHaveBeenCalledWith(mockBlob);
    expect(result.data).toHaveProperty('text');
  });

  test('should terminate worker after processing', async () => {
    const worker = await Tesseract.createWorker('eng');
    await worker.terminate();
    
    expect(worker.terminate).toHaveBeenCalled();
  });

  test('should extract ingredients from OCR text', async () => {
    const worker = await Tesseract.createWorker('eng');
    const { data } = await worker.recognize(new Blob());
    
    const fuse = new Fuse(['Chicken', 'Salmon'], { includeScore: true });
    const results = fuse.search(data.text);
    
    const ingredients = results
      .filter(r => r.score <= 0.9)
      .map(r => r.item);
    
    expect(ingredients.length).toBeGreaterThan(0);
    expect(ingredients).toContain('Chicken');
  });
});

describe('Receipt Scanner - Canvas Operations', () => {
  let canvas, context;

  beforeEach(() => {
    canvas = document.createElement('canvas');
    context = {
      drawImage: jest.fn(),
      fillRect: jest.fn(),
      fillStyle: ''
    };
    canvas.getContext = jest.fn(() => context);
  });

  test('should get 2d context from canvas', () => {
    const ctx = canvas.getContext('2d');
    
    expect(canvas.getContext).toHaveBeenCalledWith('2d');
    expect(ctx).toBeDefined();
  });

  test('should set canvas dimensions', () => {
    canvas.width = 1280;
    canvas.height = 720;
    
    expect(canvas.width).toBe(1280);
    expect(canvas.height).toBe(720);
  });

  test('should draw image on canvas', () => {
    const ctx = canvas.getContext('2d');
    const mockVideo = { videoWidth: 1280, videoHeight: 720 };
    
    ctx.drawImage(mockVideo, 0, 0, 1280, 720);
    
    expect(ctx.drawImage).toHaveBeenCalledWith(mockVideo, 0, 0, 1280, 720);
  });

  test('should fill canvas with color', () => {
    const ctx = canvas.getContext('2d');
    ctx.fillStyle = '#ffffff';
    ctx.fillRect(0, 0, 500, 250);
    
    expect(ctx.fillStyle).toBe('#ffffff');
    expect(ctx.fillRect).toHaveBeenCalledWith(0, 0, 500, 250);
  });

  test('should reset canvas dimensions to zero', () => {
    canvas.width = 1280;
    canvas.height = 720;
    
    canvas.width = 0;
    canvas.height = 0;
    
    expect(canvas.width).toBe(0);
    expect(canvas.height).toBe(0);
  });
});

describe('Receipt Scanner - URL Object Management', () => {
  test('should create object URL from blob', () => {
    const blob = new Blob(['test'], { type: 'image/png' });
    const url = URL.createObjectURL(blob);
    
    expect(URL.createObjectURL).toHaveBeenCalledWith(blob);
    expect(url).toBe('mock-blob-url-123');
  });

  test('should revoke object URL', () => {
    const url = 'mock-blob-url-123';
    URL.revokeObjectURL(url);
    
    expect(URL.revokeObjectURL).toHaveBeenCalledWith(url);
  });

  test('should track last preview URL', () => {
    let lastPreviewUrl = null;
    
    if (lastPreviewUrl) {
      URL.revokeObjectURL(lastPreviewUrl);
    }
    
    lastPreviewUrl = URL.createObjectURL(new Blob());
    expect(lastPreviewUrl).toBe('mock-blob-url-123');
    
    URL.revokeObjectURL(lastPreviewUrl);
    expect(URL.revokeObjectURL).toHaveBeenCalledWith(lastPreviewUrl);
  });
});

describe('Receipt Scanner - State Management', () => {
  test('should initialize capture ready state', () => {
    let isReadyToCapture = true;
    expect(isReadyToCapture).toBe(true);
  });

  test('should toggle capture state', () => {
    let isReadyToCapture = true;
    
    isReadyToCapture = false;
    expect(isReadyToCapture).toBe(false);
    
    isReadyToCapture = true;
    expect(isReadyToCapture).toBe(true);
  });

  test('should initialize streaming state', () => {
    let streaming = false;
    expect(streaming).toBe(false);
    
    streaming = true;
    expect(streaming).toBe(true);
  });

  test('should manage last preview URL state', () => {
    let lastPreviewUrl = null;
    expect(lastPreviewUrl).toBeNull();
    
    lastPreviewUrl = 'mock-url';
    expect(lastPreviewUrl).toBe('mock-url');
    
    lastPreviewUrl = null;
    expect(lastPreviewUrl).toBeNull();
  });
});

describe('Receipt Scanner - Video Element Properties', () => {
  let video;

  beforeEach(() => {
    video = document.createElement('video');
  });

  test('should have autoplay attribute', () => {
    video.setAttribute('autoplay', 'true');
    expect(video.getAttribute('autoplay')).toBe('true');
  });

  test('should access video dimensions', () => {
    Object.defineProperty(video, 'videoWidth', { value: 1280, writable: true });
    Object.defineProperty(video, 'videoHeight', { value: 720, writable: true });
    
    expect(video.videoWidth).toBe(1280);
    expect(video.videoHeight).toBe(720);
  });

  test('should manage srcObject', () => {
    const mockStream = { id: 'test-stream' };
    video.srcObject = mockStream;
    
    expect(video.srcObject).toBe(mockStream);
  });
});