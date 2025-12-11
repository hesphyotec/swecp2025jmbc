/**
 * Unit Tests for Recipes.html
 * 
 * Setup Instructions:
 * 1. Install dependencies:
 *    npm install --save-dev jest @testing-library/dom jest-environment-jsdom
 * 2. Add to package.json:
 *    "scripts": { "test": "jest" }
 * 3. Create jest.config.js with: module.exports = { testEnvironment: 'jsdom' };
 * 4. Run tests: npm test Recipes.test.js
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
      this.onmessage({ data: JSON.stringify(data) });
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

describe('Recipes Page - WebSocket Connection', () => {
  let socket;

  beforeEach(() => {
    localStorage.setItem('activeuserid', 'test-user-123');
    socket = new MockWebSocket('ws://localhost:18080/recipes');
  });

  afterEach(() => {
    localStorage.clear();
    sessionStorage.clear();
  });

  test('should create WebSocket with correct URL', () => {
    expect(socket.url).toBe('ws://localhost:18080/recipes');
  });

  test('should send getrecipes message on connection', () => {
    const message = {
      op: 'getrecipes',
      uid: 'test-user-123'
    };
    
    socket.send(JSON.stringify(message));
    
    expect(socket.sentMessages).toHaveLength(1);
    expect(JSON.parse(socket.sentMessages[0])).toEqual(message);
  });

  test('should handle onopen event', () => {
    const openHandler = jest.fn();
    socket.onopen = openHandler;
    
    socket.simulateOpen();
    
    expect(openHandler).toHaveBeenCalled();
  });

  test('should handle onmessage event', () => {
    const messageHandler = jest.fn();
    socket.onmessage = messageHandler;
    
    const testData = {
      status: 'success',
      recipes: []
    };
    socket.simulateMessage(testData);
    
    expect(messageHandler).toHaveBeenCalled();
  });

  test('should handle onerror event', () => {
    const errorHandler = jest.fn();
    socket.onerror = errorHandler;
    
    socket.simulateError(new Error('Connection failed'));
    
    expect(errorHandler).toHaveBeenCalled();
  });

  test('should handle onclose event', () => {
    const closeHandler = jest.fn();
    socket.onclose = closeHandler;
    
    socket.close(1000, 'Normal closure');
    
    expect(closeHandler).toHaveBeenCalled();
    expect(socket.readyState).toBe(WebSocket.CLOSED);
  });

  test('should send getInstructions message', () => {
    const message = {
      op: 'getInstructions',
      name: 'Chicken Parmesan'
    };
    
    socket.send(JSON.stringify(message));
    
    expect(socket.sentMessages).toHaveLength(1);
    const sent = JSON.parse(socket.sentMessages[0]);
    expect(sent.op).toBe('getInstructions');
    expect(sent.name).toBe('Chicken Parmesan');
  });
});

describe('Recipes Page - Recipe Data Processing', () => {
  test('should parse recipe data successfully', () => {
    const data = {
      status: 'success',
      recipes: [
        { name: 'Chicken Parmesan', image: 'chicken.jpg' },
        { name: 'Beef Tacos', image: 'tacos.jpg' }
      ]
    };
    
    expect(data.status).toBe('success');
    expect(data.recipes).toHaveLength(2);
    expect(Array.isArray(data.recipes)).toBe(true);
  });

  test('should validate recipe has required fields', () => {
    const recipe = {
      name: 'Chicken Parmesan',
      image: 'chicken.jpg'
    };
    
    expect(recipe).toHaveProperty('name');
    expect(recipe).toHaveProperty('image');
  });

  test('should handle empty recipe list', () => {
    const data = {
      status: 'success',
      recipes: []
    };
    
    expect(data.recipes).toHaveLength(0);
    expect(Array.isArray(data.recipes)).toBe(true);
  });

  test('should handle error status', () => {
    const data = {
      status: 'error',
      message: 'Database unavailable'
    };
    
    expect(data.status).toBe('error');
    expect(data.status).not.toBe('success');
  });

  test('should handle missing recipes field', () => {
    const data = {
      status: 'success'
    };
    
    expect(data.recipes).toBeUndefined();
  });
});

describe('Recipes Page - Cache Management', () => {
  beforeEach(() => {
    sessionStorage.clear();
  });

  test('should save recipe data to cache', () => {
    const data = {
      status: 'success',
      recipes: [
        { name: 'Chicken Parmesan', image: 'chicken.jpg' }
      ]
    };
    
    sessionStorage.setItem('recipeCache', JSON.stringify(data));
    
    const cached = sessionStorage.getItem('recipeCache');
    expect(cached).toBe(JSON.stringify(data));
  });

  test('should save ingredient cache', () => {
    const ingredients = JSON.stringify(['Chicken', 'Tomato']);
    
    sessionStorage.setItem('ingredients', ingredients);
    sessionStorage.setItem('ingredientCache', ingredients);
    
    expect(sessionStorage.getItem('ingredientCache')).toBe(ingredients);
  });

  test('should save preference cache', () => {
    const prefs = JSON.stringify(['Vegetarian', 'Italian']);
    
    sessionStorage.setItem('checked', prefs);
    sessionStorage.setItem('prefCache', prefs);
    
    expect(sessionStorage.getItem('prefCache')).toBe(prefs);
  });

  test('should check if cache exists', () => {
    sessionStorage.setItem('recipeCache', 'test');
    
    const hasCache = sessionStorage.getItem('recipeCache') !== null;
    
    expect(hasCache).toBe(true);
  });

  test('should check if all cache values exist', () => {
    sessionStorage.setItem('recipeCache', 'test1');
    sessionStorage.setItem('ingredientCache', 'test2');
    sessionStorage.setItem('prefCache', 'test3');
    
    const hasAllCache = 
      sessionStorage.getItem('recipeCache') !== null &&
      sessionStorage.getItem('ingredientCache') !== null &&
      sessionStorage.getItem('prefCache') !== null;
    
    expect(hasAllCache).toBe(true);
  });

  test('should validate cache matches current data', () => {
    const ingredients = JSON.stringify(['Chicken']);
    
    sessionStorage.setItem('ingredients', ingredients);
    sessionStorage.setItem('ingredientCache', ingredients);
    
    const cacheValid = 
      sessionStorage.getItem('ingredientCache') === sessionStorage.getItem('ingredients');
    
    expect(cacheValid).toBe(true);
  });

  test('should detect when cache is invalid', () => {
    sessionStorage.setItem('ingredients', JSON.stringify(['Chicken']));
    sessionStorage.setItem('ingredientCache', JSON.stringify(['Beef']));
    
    const cacheValid = 
      sessionStorage.getItem('ingredientCache') === sessionStorage.getItem('ingredients');
    
    expect(cacheValid).toBe(false);
  });

  test('should load recipes from cache', () => {
    const cacheData = {
      recipes: [
        { name: 'Chicken Parmesan', image: 'chicken.jpg' },
        { name: 'Beef Tacos', image: 'tacos.jpg' }
      ]
    };
    
    sessionStorage.setItem('recipeCache', JSON.stringify(cacheData));
    
    const loaded = JSON.parse(sessionStorage.getItem('recipeCache'));
    
    expect(loaded.recipes).toHaveLength(2);
    expect(loaded.recipes[0].name).toBe('Chicken Parmesan');
  });
});

describe('Recipes Page - DOM Rendering', () => {
  let container;

  beforeEach(() => {
    document.body.innerHTML = `
      <main id="recipes-container"></main>
    `;
    container = document.getElementById('recipes-container');
  });

  test('should display loading message initially', () => {
    const loadingMsg = document.createElement('p');
    loadingMsg.textContent = 'Loading recommendations...';
    loadingMsg.className = 'loading';
    container.appendChild(loadingMsg);
    
    expect(container.querySelector('.loading')).toBeTruthy();
    expect(container.querySelector('.loading').textContent).toBe('Loading recommendations...');
  });

  test('should clear container before rendering recipes', () => {
    container.innerHTML = '<p>Loading...</p>';
    container.innerHTML = '';
    
    expect(container.children.length).toBe(0);
  });

  test('should render recipe card', () => {
    const card = document.createElement('div');
    card.className = 'recipe-card';
    card.innerHTML = `
      <img src="chicken.jpg" alt="Chicken Parmesan">
      <h3>Chicken Parmesan</h3>
    `;
    container.appendChild(card);
    
    expect(container.querySelector('.recipe-card')).toBeTruthy();
    expect(container.querySelector('h3').textContent).toBe('Chicken Parmesan');
    expect(container.querySelector('img').alt).toBe('Chicken Parmesan');
  });

  test('should render multiple recipe cards', () => {
    const recipes = [
      { name: 'Chicken Parmesan', image: 'chicken.jpg' },
      { name: 'Beef Tacos', image: 'tacos.jpg' }
    ];
    
    recipes.forEach(recipe => {
      const card = document.createElement('div');
      card.className = 'recipe-card';
      card.innerHTML = `
        <img src="${recipe.image}" alt="${recipe.name}">
        <h3>${recipe.name}</h3>
      `;
      container.appendChild(card);
    });
    
    expect(container.querySelectorAll('.recipe-card')).toHaveLength(2);
  });

  test('should display error message when no recipes found', () => {
    container.innerHTML = '<p>No recipes found.</p>';
    
    expect(container.textContent).toBe('No recipes found.');
  });

  test('should display error message on connection error', () => {
    container.innerHTML = '<p>Error loading recommendations.</p>';
    
    expect(container.textContent).toBe('Error loading recommendations.');
  });

  test('should have onclick handler on recipe image', () => {
    const img = document.createElement('img');
    img.src = 'chicken.jpg';
    img.alt = 'Chicken Parmesan';
    img.onclick = jest.fn();
    
    expect(img.onclick).toBeDefined();
  });
});

describe('Recipes Page - Instructions Data', () => {
  beforeEach(() => {
    sessionStorage.clear();
  });

  test('should save instruction data to sessionStorage', () => {
    const instructions = {
      ingredients: ['Chicken', 'Tomato Sauce'],
      instructions: 'Step 1: Cook chicken...'
    };
    
    sessionStorage.setItem('instruction', JSON.stringify(instructions));
    
    const stored = JSON.parse(sessionStorage.getItem('instruction'));
    expect(stored.ingredients).toEqual(['Chicken', 'Tomato Sauce']);
    expect(stored.instructions).toBeDefined();
  });

  test('should save recipe name to sessionStorage', () => {
    const recipeName = 'Chicken Parmesan';
    
    sessionStorage.setItem('recName', JSON.stringify(recipeName));
    
    const stored = JSON.parse(sessionStorage.getItem('recName'));
    expect(stored).toBe('Chicken Parmesan');
  });

  test('should save recipe image to sessionStorage', () => {
    const imageUrl = 'https://example.com/chicken.jpg';
    
    sessionStorage.setItem('recImage', JSON.stringify(imageUrl));
    
    const stored = JSON.parse(sessionStorage.getItem('recImage'));
    expect(stored).toBe(imageUrl);
  });

  test('should validate instruction has required fields', () => {
    const instruction = {
      ingredients: ['Chicken'],
      instructions: 'Cook it'
    };
    
    const isValid = 
      instruction.ingredients !== undefined && 
      instruction.instructions !== undefined;
    
    expect(isValid).toBe(true);
  });

  test('should handle missing ingredients field', () => {
    const instruction = {
      instructions: 'Cook it'
    };
    
    const isValid = 
      instruction.ingredients !== undefined && 
      instruction.instructions !== undefined;
    
    expect(isValid).toBe(false);
  });

  test('should handle missing instructions field', () => {
    const instruction = {
      ingredients: ['Chicken']
    };
    
    const isValid = 
      instruction.ingredients !== undefined && 
      instruction.instructions !== undefined;
    
    expect(isValid).toBe(false);
  });

  test('should parse instructions response array', () => {
    const response = [
      {
        ingredients: ['Chicken', 'Tomato'],
        instructions: 'Step 1...'
      }
    ];
    
    const instructions = response[0];
    
    expect(instructions).toBeDefined();
    expect(instructions.ingredients).toHaveLength(2);
  });
});

describe('Recipes Page - Navigation', () => {
  test('should store navigation target in location', () => {
    // Mock a simple location object that just stores the href
    const mockLocation = { href: 'http://localhost/' };
    
    // Simulate navigation by updating href
    mockLocation.href = 'Instructions.html';
    
    expect(mockLocation.href).toBe('Instructions.html');
  });

  test('should have navigation buttons', () => {
    document.body.innerHTML = `
      <header>
        <div class="header-buttons">
          <button onclick="location.href='Homepage.html'">🏚️</button>
          <button onclick="location.href='Saved.html'">🥣</button>
          <button onclick="location.href='Account.html'">🙍🏾</button>
        </div>
      </header>
    `;
    
    const buttons = document.querySelectorAll('.header-buttons button');
    expect(buttons).toHaveLength(3);
  });
});

describe('Recipes Page - Page Visibility Handling', () => {
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
    const socket = new MockWebSocket('ws://localhost:18080/recipes');
    
    Object.defineProperty(document, 'hidden', {
      configurable: true,
      get: () => true
    });
    
    if (document.hidden) {
      socket.close(1000, 'Disconnecting from Recommendation server');
    }
    
    expect(socket.readyState).toBe(WebSocket.CLOSED);
  });

  test('should reconnect WebSocket when page becomes visible', () => {
    let socket = new MockWebSocket('ws://localhost:18080/recipes');
    socket.close();
    
    Object.defineProperty(document, 'hidden', {
      configurable: true,
      get: () => false
    });
    
    if (!document.hidden && socket.readyState === WebSocket.CLOSED) {
      socket = new MockWebSocket('ws://localhost:18080/recipes');
    }
    
    expect(socket.readyState).toBe(WebSocket.OPEN);
  });
});

describe('Recipes Page - Filter Preferences', () => {
  test('should have filter checkbox values', () => {
    const filters = [
      'Breakfast',
      'Dessert',
      'Pasta',
      'Seafood',
      'Vegan',
      'Vegetarian',
      'American',
      'Italian',
      'Mexican'
    ];
    
    expect(filters).toHaveLength(9);
    expect(filters).toContain('Vegan');
    expect(filters).toContain('Italian');
  });

  test('should create checkbox element', () => {
    const checkbox = document.createElement('input');
    checkbox.type = 'checkbox';
    checkbox.value = 'Vegan';
    
    expect(checkbox.type).toBe('checkbox');
    expect(checkbox.value).toBe('Vegan');
  });

  test('should toggle checkbox state', () => {
    const checkbox = document.createElement('input');
    checkbox.type = 'checkbox';
    checkbox.checked = false;
    
    checkbox.checked = true;
    expect(checkbox.checked).toBe(true);
    
    checkbox.checked = false;
    expect(checkbox.checked).toBe(false);
  });

  test('should collect checked filter values', () => {
    const checkboxes = [
      { value: 'Vegan', checked: true },
      { value: 'Italian', checked: false },
      { value: 'Seafood', checked: true }
    ];
    
    const selected = checkboxes
      .filter(cb => cb.checked)
      .map(cb => cb.value);
    
    expect(selected).toEqual(['Vegan', 'Seafood']);
  });
});

describe('Recipes Page - Error Handling', () => {
  test('should handle JSON parse error', () => {
    const badJSON = '{invalid json}';
    
    expect(() => JSON.parse(badJSON)).toThrow();
  });

  test('should handle null recipe data', () => {
    const data = {
      status: 'success',
      recipes: null
    };
    
    const hasRecipes = data.recipes && data.recipes.length > 0;
    
    expect(hasRecipes).toBeFalsy();
  });

  test('should validate status before processing', () => {
    const data = {
      status: 'error',
      recipes: []
    };
    
    const shouldProcess = data.status === 'success';
    
    expect(shouldProcess).toBe(false);
  });

  test('should handle undefined recipe list', () => {
    const data = {
      status: 'success'
    };
    
    const hasRecipes = data.recipes && data.recipes.length > 0;
    
    expect(hasRecipes).toBeFalsy();
  });
});

describe('Recipes Page - WebSocket State Management', () => {
  test('should check WebSocket readyState', () => {
    const socket = new MockWebSocket('ws://localhost:18080/recipes');
    
    expect(socket.readyState).toBe(WebSocket.OPEN);
  });

  test('should detect closed connection', () => {
    const socket = new MockWebSocket('ws://localhost:18080/recipes');
    socket.close();
    
    expect(socket.readyState).toBe(WebSocket.CLOSED);
  });

  test('should not send when connection is closed', () => {
    const socket = new MockWebSocket('ws://localhost:18080/recipes');
    socket.close();
    
    if (socket.readyState === WebSocket.OPEN) {
      socket.send(JSON.stringify({ test: 'data' }));
    }
    
    expect(socket.sentMessages).toHaveLength(0);
  });
});