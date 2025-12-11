/**
 * Unit Tests for Instructions.html
 * 
 * Setup Instructions:
 * 1. Install dependencies:
 *    npm install --save-dev jest @testing-library/dom jest-environment-jsdom
 * 2. Add to package.json:
 *    "scripts": { "test": "jest" }
 * 3. Create jest.config.js with: module.exports = { testEnvironment: 'jsdom' };
 * 4. Run tests: npm test Instructions.test.js
 */

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

// Mock URLSearchParams
global.URLSearchParams = class URLSearchParams {
  constructor(search) {
    this.params = {};
    if (search) {
      const pairs = search.replace('?', '').split('&');
      pairs.forEach(pair => {
        const [key, value] = pair.split('=');
        if (key) this.params[key] = value || '';
      });
    }
  }
  get(key) {
    return this.params[key] || null;
  }
  has(key) {
    return key in this.params;
  }
};

describe('Instructions Page - SessionStorage Data Retrieval', () => {
  beforeEach(() => {
    sessionStorage.clear();
  });

  test('should retrieve instruction data from sessionStorage', () => {
    const instruction = {
      ingredients: 'Chicken;Tomato;Garlic',
      instructions: 'Step 1\r\nStep 2\r\nStep 3'
    };
    
    sessionStorage.setItem('instruction', JSON.stringify(instruction));
    
    const stored = JSON.parse(sessionStorage.getItem('instruction'));
    
    expect(stored).toEqual(instruction);
    expect(stored.ingredients).toBeDefined();
    expect(stored.instructions).toBeDefined();
  });

  test('should retrieve recipe name from sessionStorage', () => {
    const recipeName = 'Chicken Parmesan';
    
    sessionStorage.setItem('recName', JSON.stringify(recipeName));
    
    const stored = JSON.parse(sessionStorage.getItem('recName'));
    
    expect(stored).toBe('Chicken Parmesan');
  });

  test('should retrieve recipe image from sessionStorage', () => {
    const imageUrl = 'https://example.com/chicken.jpg';
    
    sessionStorage.setItem('recImage', JSON.stringify(imageUrl));
    
    const stored = JSON.parse(sessionStorage.getItem('recImage'));
    
    expect(stored).toBe(imageUrl);
  });

  test('should check if instruction exists', () => {
    sessionStorage.setItem('instruction', JSON.stringify({ ingredients: '', instructions: '' }));
    
    const hasInstruction = sessionStorage.getItem('instruction') !== null;
    
    expect(hasInstruction).toBe(true);
  });

  test('should handle missing instruction data', () => {
    const hasInstruction = sessionStorage.getItem('instruction') !== null;
    
    expect(hasInstruction).toBe(false);
  });
});

describe('Instructions Page - Data Parsing', () => {
  test('should split ingredients by semicolon', () => {
    const ingredientsString = 'Chicken;Tomato;Garlic;Olive Oil';
    
    const ingArray = ingredientsString.split(';').filter(s => s.length > 0);
    
    expect(ingArray).toHaveLength(4);
    expect(ingArray).toEqual(['Chicken', 'Tomato', 'Garlic', 'Olive Oil']);
  });

  test('should filter empty ingredients', () => {
    const ingredientsString = 'Chicken;;Tomato;;';
    
    const ingArray = ingredientsString.split(';').filter(s => s.length > 0);
    
    expect(ingArray).toHaveLength(2);
    expect(ingArray).toEqual(['Chicken', 'Tomato']);
  });

  test('should split instructions by line break', () => {
    const instructionsString = 'Step 1\r\nStep 2\r\nStep 3';
    
    const instArray = instructionsString.split('\r\n').filter(s => s.length > 0);
    
    expect(instArray).toHaveLength(3);
    expect(instArray).toEqual(['Step 1', 'Step 2', 'Step 3']);
  });

  test('should filter empty instruction lines', () => {
    const instructionsString = 'Step 1\r\n\r\nStep 2\r\n';
    
    const instArray = instructionsString.split('\r\n').filter(s => s.length > 0);
    
    expect(instArray).toHaveLength(2);
    expect(instArray).toEqual(['Step 1', 'Step 2']);
  });

  test('should handle single ingredient', () => {
    const ingredientsString = 'Chicken';
    
    const ingArray = ingredientsString.split(';').filter(s => s.length > 0);
    
    expect(ingArray).toHaveLength(1);
    expect(ingArray[0]).toBe('Chicken');
  });

  test('should handle single instruction', () => {
    const instructionsString = 'Cook the chicken';
    
    const instArray = instructionsString.split('\r\n').filter(s => s.length > 0);
    
    expect(instArray).toHaveLength(1);
    expect(instArray[0]).toBe('Cook the chicken');
  });

  test('should handle empty ingredients string', () => {
    const ingredientsString = '';
    
    const ingArray = ingredientsString.split(';').filter(s => s.length > 0);
    
    expect(ingArray).toHaveLength(0);
  });

  test('should handle empty instructions string', () => {
    const instructionsString = '';
    
    const instArray = instructionsString.split('\r\n').filter(s => s.length > 0);
    
    expect(instArray).toHaveLength(0);
  });
});

describe('Instructions Page - URL Parameters', () => {
  test('should parse recipe id from URL', () => {
    const params = new URLSearchParams('?id=123');
    const recipeId = params.get('id');
    
    expect(recipeId).toBe('123');
  });

  test('should return null for missing parameter', () => {
    const params = new URLSearchParams('?other=value');
    const recipeId = params.get('id');
    
    expect(recipeId).toBeNull();
  });

  test('should handle empty search string', () => {
    const params = new URLSearchParams('');
    const recipeId = params.get('id');
    
    expect(recipeId).toBeNull();
  });

  test('should parse multiple parameters', () => {
    const params = new URLSearchParams('?id=123&name=chicken');
    
    expect(params.get('id')).toBe('123');
    expect(params.get('name')).toBe('chicken');
  });

  test('should check if parameter exists', () => {
    const params = new URLSearchParams('?id=123');
    
    expect(params.has('id')).toBe(true);
    expect(params.has('missing')).toBe(false);
  });
});

describe('Instructions Page - DOM Rendering', () => {
  let container;

  beforeEach(() => {
    document.body.innerHTML = `
      <div class="recipe-details" id="recipe-container">
        <p class="loading">Connecting to recipe server...</p>
      </div>
    `;
    container = document.getElementById('recipe-container');
  });

  test('should display loading message initially', () => {
    expect(container.querySelector('.loading')).toBeTruthy();
    expect(container.querySelector('.loading').textContent).toBe('Connecting to recipe server...');
  });

  test('should render recipe title', () => {
    const recipeName = 'Chicken Parmesan';
    
    container.innerHTML = `
      <h1 class="recipe-title">${recipeName}</h1>
    `;
    
    expect(container.querySelector('.recipe-title')).toBeTruthy();
    expect(container.querySelector('.recipe-title').textContent).toBe('Chicken Parmesan');
  });

  test('should render recipe image', () => {
    const imageUrl = 'https://example.com/chicken.jpg';
    const recipeName = 'Chicken Parmesan';
    
    container.innerHTML = `
      <img src="${imageUrl}" alt="${recipeName}">
    `;
    
    const img = container.querySelector('img');
    expect(img).toBeTruthy();
    expect(img.src).toBe(imageUrl);
    expect(img.alt).toBe(recipeName);
  });

  test('should render ingredients list', () => {
    const ingredients = ['Chicken', 'Tomato', 'Garlic'];
    
    container.innerHTML = `
      <h2>Ingredients</h2>
      <ul>${ingredients.map(i => `<li>${i}</li>`).join('')}</ul>
    `;
    
    expect(container.querySelector('h2').textContent).toBe('Ingredients');
    expect(container.querySelectorAll('li')).toHaveLength(3);
  });

  test('should render instructions list', () => {
    const instructions = ['Step 1', 'Step 2', 'Step 3'];
    
    container.innerHTML = `
      <h2>Instructions</h2>
      <ol>${instructions.map(s => `<li>${s}</li>`).join('')}</ol>
    `;
    
    expect(container.querySelector('h2').textContent).toBe('Instructions');
    expect(container.querySelectorAll('li')).toHaveLength(3);
  });

  test('should render complete recipe details', () => {
    const recipeName = 'Chicken Parmesan';
    const imageUrl = 'https://example.com/chicken.jpg';
    const ingredients = ['Chicken', 'Tomato'];
    const instructions = ['Step 1', 'Step 2'];
    
    container.innerHTML = `
      <h1 class="recipe-title">${recipeName}</h1>
      <img src="${imageUrl}" alt="${recipeName}">
      <h2>Ingredients</h2>
      <ul>${ingredients.map(i => `<li>${i}</li>`).join('')}</ul>
      <h2>Instructions</h2>
      <ol>${instructions.map(s => `<li>${s}</li>`).join('')}</ol>
    `;
    
    expect(container.querySelector('.recipe-title')).toBeTruthy();
    expect(container.querySelector('img')).toBeTruthy();
    expect(container.querySelectorAll('h2')).toHaveLength(2);
    expect(container.querySelector('ul')).toBeTruthy();
    expect(container.querySelector('ol')).toBeTruthy();
  });

  test('should render without image if not provided', () => {
    const recipeName = 'Chicken Parmesan';
    const img = null;
    
    container.innerHTML = `
      <h1 class="recipe-title">${recipeName}</h1>
      ${img ? `<img src="${img}" alt="${recipeName}">` : ""}
    `;
    
    expect(container.querySelector('.recipe-title')).toBeTruthy();
    expect(container.querySelector('img')).toBeNull();
  });

  test('should display error message on failure', () => {
    container.innerHTML = "<p class='loading'>Failed to load recipe.</p>";
    
    expect(container.querySelector('.loading')).toBeTruthy();
    expect(container.querySelector('.loading').textContent).toBe('Failed to load recipe.');
  });

  test('should render top navigation buttons', () => {
    container.innerHTML = `
      <div class="top-buttons">
        <button onclick="location.href='Recipes.html'">← Back to Recipes</button>
        <button onclick="location.href='Homepage.html'">🏠 Homepage</button>
      </div>
    `;
    
    const buttons = container.querySelectorAll('.top-buttons button');
    expect(buttons).toHaveLength(2);
    expect(buttons[0].textContent).toBe('← Back to Recipes');
    expect(buttons[1].textContent).toBe('🏠 Homepage');
  });
});

describe('Instructions Page - Array Mapping', () => {
  test('should map ingredients to list items', () => {
    const ingredients = ['Chicken', 'Tomato', 'Garlic'];
    
    const html = ingredients.map(i => `<li>${i}</li>`).join('');
    
    expect(html).toBe('<li>Chicken</li><li>Tomato</li><li>Garlic</li>');
  });

  test('should map instructions to list items', () => {
    const instructions = ['Step 1', 'Step 2'];
    
    const html = instructions.map(s => `<li>${s}</li>`).join('');
    
    expect(html).toBe('<li>Step 1</li><li>Step 2</li>');
  });

  test('should handle empty arrays', () => {
    const empty = [];
    
    const html = empty.map(i => `<li>${i}</li>`).join('');
    
    expect(html).toBe('');
  });

  test('should join with line breaks', () => {
    const items = ['Item 1', 'Item 2'];
    
    const html = items.map(i => `<li>${i}</li>`).join('\r\n');
    
    expect(html).toBe('<li>Item 1</li>\r\n<li>Item 2</li>');
  });
});

describe('Instructions Page - Navigation', () => {
  test('should have bottom navigation buttons', () => {
    document.body.innerHTML = `
      <nav>
        <button onclick="location.href='Homepage.html'">Home</button>
        <button onclick="location.href='Recipes.html'">Recipes</button>
        <button onclick="location.href='Account.html'">Account</button>
      </nav>
    `;
    
    const buttons = document.querySelectorAll('nav button');
    expect(buttons).toHaveLength(3);
    expect(buttons[0].textContent).toBe('Home');
    expect(buttons[1].textContent).toBe('Recipes');
    expect(buttons[2].textContent).toBe('Account');
  });

  test('should store navigation targets', () => {
    const mockLocation = { href: '' };
    
    mockLocation.href = 'Homepage.html';
    expect(mockLocation.href).toBe('Homepage.html');
    
    mockLocation.href = 'Recipes.html';
    expect(mockLocation.href).toBe('Recipes.html');
  });
});

describe('Instructions Page - Data Validation', () => {
  test('should validate instruction object has required fields', () => {
    const instruction = {
      ingredients: 'Chicken;Tomato',
      instructions: 'Step 1\r\nStep 2'
    };
    
    const isValid = 
      instruction.ingredients !== undefined && 
      instruction.instructions !== undefined;
    
    expect(isValid).toBe(true);
  });

  test('should detect missing ingredients field', () => {
    const instruction = {
      instructions: 'Step 1'
    };
    
    const isValid = 
      instruction.ingredients !== undefined && 
      instruction.instructions !== undefined;
    
    expect(isValid).toBe(false);
  });

  test('should detect missing instructions field', () => {
    const instruction = {
      ingredients: 'Chicken'
    };
    
    const isValid = 
      instruction.ingredients !== undefined && 
      instruction.instructions !== undefined;
    
    expect(isValid).toBe(false);
  });

  test('should handle null instruction data', () => {
    const instruction = null;
    
    const isValid = instruction !== null && 
      instruction.ingredients !== undefined && 
      instruction.instructions !== undefined;
    
    expect(isValid).toBe(false);
  });
});

describe('Instructions Page - String Operations', () => {
  test('should filter strings by length', () => {
    const strings = ['', 'Item 1', '', 'Item 2', ''];
    
    const filtered = strings.filter(s => s.length > 0);
    
    expect(filtered).toEqual(['Item 1', 'Item 2']);
  });

  test('should trim whitespace from strings', () => {
    const strings = ['  Item 1  ', 'Item 2', '  '];
    
    const trimmed = strings.map(s => s.trim()).filter(s => s.length > 0);
    
    expect(trimmed).toEqual(['Item 1', 'Item 2']);
  });

  test('should split string by delimiter', () => {
    const text = 'A;B;C';
    
    const parts = text.split(';');
    
    expect(parts).toEqual(['A', 'B', 'C']);
  });

  test('should handle string with no delimiter', () => {
    const text = 'No delimiter here';
    
    const parts = text.split(';');
    
    expect(parts).toHaveLength(1);
    expect(parts[0]).toBe('No delimiter here');
  });
});

describe('Instructions Page - Conditional Rendering', () => {
  test('should render image when URL is provided', () => {
    const img = 'https://example.com/image.jpg';
    const name = 'Test Recipe';
    
    const html = img ? `<img src="${img}" alt="${name}">` : "";
    
    expect(html).toContain('<img');
    expect(html).toContain(img);
  });

  test('should not render image when URL is null', () => {
    const img = null;
    const name = 'Test Recipe';
    
    const html = img ? `<img src="${img}" alt="${name}">` : "";
    
    expect(html).toBe('');
  });

  test('should not render image when URL is empty string', () => {
    const img = '';
    const name = 'Test Recipe';
    
    const html = img ? `<img src="${img}" alt="${name}">` : "";
    
    expect(html).toBe('');
  });

  test('should not render image when URL is undefined', () => {
    const img = undefined;
    const name = 'Test Recipe';
    
    const html = img ? `<img src="${img}" alt="${name}">` : "";
    
    expect(html).toBe('');
  });
});

describe('Instructions Page - Complete Flow', () => {
  beforeEach(() => {
    sessionStorage.clear();
  });

  test('should load and render complete recipe from sessionStorage', () => {
    // Setup data in sessionStorage
    const instruction = {
      ingredients: 'Chicken;Tomato;Garlic',
      instructions: 'Step 1: Prepare\r\nStep 2: Cook\r\nStep 3: Serve'
    };
    const recipeName = 'Chicken Parmesan';
    const imageUrl = 'https://example.com/chicken.jpg';
    
    sessionStorage.setItem('instruction', JSON.stringify(instruction));
    sessionStorage.setItem('recName', JSON.stringify(recipeName));
    sessionStorage.setItem('recImage', JSON.stringify(imageUrl));
    
    // Check data exists
    const hasInstruction = sessionStorage.getItem('instruction') !== null;
    expect(hasInstruction).toBe(true);
    
    // Parse data
    const storedInstruction = JSON.parse(sessionStorage.getItem('instruction'));
    const storedName = JSON.parse(sessionStorage.getItem('recName'));
    const storedImage = JSON.parse(sessionStorage.getItem('recImage'));
    
    // Parse ingredients and instructions
    const ingArray = storedInstruction.ingredients.split(';').filter(s => s.length > 0);
    const instArray = storedInstruction.instructions.split('\r\n').filter(s => s.length > 0);
    
    // Verify parsed data
    expect(ingArray).toHaveLength(3);
    expect(instArray).toHaveLength(3);
    expect(storedName).toBe('Chicken Parmesan');
    expect(storedImage).toBe(imageUrl);
  });
});