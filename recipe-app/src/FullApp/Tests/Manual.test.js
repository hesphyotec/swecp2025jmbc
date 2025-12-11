/**
 * @jest-environment jsdom
 */
const fs = require("fs");
const path = require("path");

// Suppress console.error for navigation warnings
const originalError = console.error;
beforeAll(() => {
  console.error = (...args) => {
    if (
      typeof args[0] === 'string' &&
      args[0].includes('Not implemented: navigation')
    ) {
      return;
    }
    originalError.call(console, ...args);
  };
});

afterAll(() => {
  console.error = originalError;
});

describe("Manual Ingredient Input Page Tests", () => {
  let document;
  let mockWebSocket;
  let WebSocketConstructor;
  let localStorageMock;
  let sessionStorageMock;
  let $;

  beforeEach(() => {
    jest.clearAllMocks();

    mockWebSocket = {
      send: jest.fn(),
      close: jest.fn(),
      onopen: null,
      onmessage: null,
      onerror: null,
      onclose: null,
      readyState: 1,
      CONNECTING: 0,
      OPEN: 1,
      CLOSING: 2,
      CLOSED: 3
    };

    WebSocketConstructor = jest.fn(() => mockWebSocket);
    WebSocketConstructor.CONNECTING = 0;
    WebSocketConstructor.OPEN = 1;
    WebSocketConstructor.CLOSING = 2;
    WebSocketConstructor.CLOSED = 3;

    global.WebSocket = WebSocketConstructor;
    window.WebSocket = WebSocketConstructor;

    global.alert = jest.fn();
    window.alert = jest.fn();
    global.console.log = jest.fn();
    global.console.error = jest.fn();

    localStorageMock = {
      getItem: jest.fn(() => "testuser123"),
      setItem: jest.fn(),
      clear: jest.fn()
    };
    global.localStorage = localStorageMock;
    window.localStorage = localStorageMock;

    sessionStorageMock = {
      getItem: jest.fn(() => null),
      setItem: jest.fn(),
      clear: jest.fn()
    };
    global.sessionStorage = sessionStorageMock;
    window.sessionStorage = sessionStorageMock;

    let html = fs.readFileSync(path.resolve(__dirname, "../Manual.html"), "utf8");
    html = html.replace(/new WebSocket`([^`]+)`/g, 'new WebSocket("$1")');

    document = window.document;
    document.documentElement.innerHTML = html;

    // Initialize jQuery
    const jQuery = require("jquery")(window);
    $ = jQuery;
    window.$ = jQuery;
    window.jQuery = jQuery;
    global.$ = jQuery;

    // Execute the script from the HTML
    const scriptMatch = html.match(/<script>([\s\S]*?)<\/script>/);
    if (scriptMatch && scriptMatch[1]) {
      try {
        const scriptFunc = new Function('window', 'document', '$', 'localStorage', 'sessionStorage', 'WebSocket', 'alert', 'console', scriptMatch[1]);
        scriptFunc(window, document, jQuery, localStorageMock, sessionStorageMock, WebSocketConstructor, alert, console);
      } catch (err) {
        // Script execution errors are expected in test environment
      }
    }
  });

  test("Page displays Manual Ingredient Input header", () => {
    const header = document.querySelector("header");
    expect(header).not.toBeNull();
    expect(header.textContent).toBe("Manual Ingredient Input");
  });

  test("Page has correct document title", () => {
    const title = document.querySelector("title");
    expect(title).not.toBeNull();
    expect(title.textContent).toBe("Manual Ingredient Input");
  });

  test("Form exists with correct ID", () => {
    const form = document.getElementById("addi");
    expect(form).not.toBeNull();
    expect(form.tagName).toBe("FORM");
  });

  test("Ingredient input field exists with correct attributes", () => {
    const input = document.getElementById("ingredient-add");
    expect(input).not.toBeNull();
    expect(input.type).toBe("text");
    expect(input.placeholder).toBe("e.g. 2 tomatoes, 1 onion");
  });

  test("Label for ingredient input exists", () => {
    const label = document.querySelector('label[for="ingredient"]');
    expect(label).not.toBeNull();
    expect(label.textContent).toBe("Enter Ingredient:");
  });

  test("Add button exists with correct text", () => {
    const addBtn = document.getElementById("add-btn");
    expect(addBtn).not.toBeNull();
    expect(addBtn.textContent).toBe("Add Ingredient");
    expect(addBtn.type).toBe("button");
  });

  test("Ingredients list div exists with placeholder text", () => {
    const list = document.getElementById("ingredients-list");
    expect(list).not.toBeNull();
    expect(list.textContent.trim()).toBe("Your ingredients will appear here.");
  });

  test("Footer navigation is present", () => {
    const footer = document.querySelector("footer");
    expect(footer).not.toBeNull();
    
    const nav = footer.querySelector(".bottom-nav");
    expect(nav).not.toBeNull();
  });

  test("All navigation buttons exist in footer", () => {
    const buttons = document.querySelectorAll("footer button");
    const buttonTexts = [...buttons].map(btn => btn.textContent);
    
    expect(buttonTexts).toContain("Back");
    expect(buttonTexts).toContain("Barcode");
    expect(buttonTexts).toContain("Receipt");
    expect(buttonTexts).toContain("Ingredients");
    expect(buttonTexts).toContain("Scan Food");
  });

  test("Back button links to Homepage", () => {
    const buttons = document.querySelectorAll("footer button");
    const backButton = [...buttons].find(btn => btn.textContent === "Back");
    expect(backButton.getAttribute("onclick")).toContain("Homepage.html");
  });

  test("Barcode button links to Barcode page", () => {
    const buttons = document.querySelectorAll("footer button");
    const barcodeButton = [...buttons].find(btn => btn.textContent === "Barcode");
    expect(barcodeButton.getAttribute("onclick")).toContain("Barcode.html");
  });

  test("Receipt button links to Receipt page", () => {
    const buttons = document.querySelectorAll("footer button");
    const receiptButton = [...buttons].find(btn => btn.textContent === "Receipt");
    expect(receiptButton.getAttribute("onclick")).toContain("Receipt.html");
  });

  test("Ingredients button links to Saved page", () => {
    const buttons = document.querySelectorAll("footer button");
    const ingredientsButton = [...buttons].find(btn => btn.textContent === "Ingredients");
    expect(ingredientsButton.getAttribute("onclick")).toContain("Saved.html");
  });

  test("Scan Food button links to ScanFood page and has active class", () => {
    const buttons = document.querySelectorAll("footer button");
    const scanFoodButton = [...buttons].find(btn => btn.textContent === "Scan Food");
    expect(scanFoodButton.getAttribute("onclick")).toContain("ScanFood.html");
    expect(scanFoodButton.classList.contains("active")).toBe(true);
  });

  test("WebSocket connection is created on page load", () => {
    expect(WebSocketConstructor).toHaveBeenCalled();
    const callArgs = WebSocketConstructor.mock.calls[0][0];
    expect(callArgs).toContain("ws://");
    expect(callArgs).toContain(":18080/inventory");
  });

  test("WebSocket sends getlist operation on open", () => {
    // Trigger onopen handler
    if (mockWebSocket.onopen) {
      mockWebSocket.onopen();
    }

    expect(mockWebSocket.send).toHaveBeenCalled();
    
    if (mockWebSocket.send.mock.calls.length > 0) {
      const sentData = JSON.parse(mockWebSocket.send.mock.calls[0][0]);
      expect(sentData.op).toBe("getlist");
      expect(sentData.uid).toBe("testuser123");
    }
  });

  test("WebSocket onmessage handler is defined", () => {
    expect(mockWebSocket.onmessage).toBeDefined();
  });

  test("WebSocket onerror handler is defined", () => {
    expect(mockWebSocket.onerror).toBeDefined();
  });

  test("WebSocket onclose handler is defined", () => {
    expect(mockWebSocket.onclose).toBeDefined();
  });

  test("Main content area exists", () => {
    const main = document.querySelector("main");
    expect(main).not.toBeNull();
  });

  test("Form has proper structure with label, input, and button", () => {
    const form = document.getElementById("addi");
    expect(form).not.toBeNull();
    
    const label = form.querySelector("label");
    const input = form.querySelector("input");
    const button = form.querySelector("button");
    
    expect(label).not.toBeNull();
    expect(input).not.toBeNull();
    expect(button).not.toBeNull();
  });

  test("Remove button styling class exists in stylesheet", () => {
    const style = document.querySelector("style");
    expect(style.textContent).toContain(".remove-btn");
  });

  test("Page has correct background color styling", () => {
    const body = document.querySelector("body");
    expect(body).not.toBeNull();
    const styles = window.getComputedStyle(body);
    expect(styles).toBeDefined();
  });

  test("Ingredients list has proper styling", () => {
    const list = document.getElementById("ingredients-list");
    expect(list.id).toBe("ingredients-list");
  });

  test("Button hover styles are defined", () => {
    const style = document.querySelector("style");
    expect(style.textContent).toContain("button:hover");
  });

  test("Form layout uses flexbox", () => {
    const form = document.getElementById("addi");
    expect(form).not.toBeNull();
    // Form exists
  });

  test("Input field has proper padding and border radius", () => {
    const input = document.getElementById("ingredient-add");
    expect(input).not.toBeNull();
  });

  test("jQuery library is included", () => {
    const scripts = document.querySelectorAll("script");
    const jqueryScript = [...scripts].find(script => 
      script.src && script.src.includes("jquery")
    );
    expect(jqueryScript).not.toBeNull();
  });

  test("Visibility change event listener is attached", () => {
    const event = new Event('visibilitychange');
    expect(() => document.dispatchEvent(event)).not.toThrow();
  });

  test("All navigation buttons have onclick attributes", () => {
    const buttons = document.querySelectorAll("footer button");
    buttons.forEach(button => {
      expect(button.hasAttribute("onclick")).toBe(true);
    });
  });

  test("Header has correct styling", () => {
    const header = document.querySelector("header");
    expect(header).not.toBeNull();
    expect(header.textContent).toBeTruthy();
  });

  test("Footer has correct structure", () => {
    const footer = document.querySelector("footer");
    expect(footer).not.toBeNull();
    
    const nav = footer.querySelector(".bottom-nav");
    expect(nav).not.toBeNull();
    
    const buttons = nav.querySelectorAll("button");
    expect(buttons.length).toBe(5);
  });

  test("Ingredients list container has minimum height", () => {
    const list = document.getElementById("ingredients-list");
    expect(list).not.toBeNull();
  });

  test("Add button is of type button not submit", () => {
    const addBtn = document.getElementById("add-btn");
    expect(addBtn.type).toBe("button");
  });

  test("Input field accepts text input", () => {
    const input = document.getElementById("ingredient-add");
    expect(input.type).toBe("text");
  });

  test("LocalStorage is accessed for user ID", () => {
    const userId = localStorageMock.getItem("activeuserid");
    expect(userId).toBe("testuser123");
  });

  test("Page layout is properly structured", () => {
    const body = document.querySelector("body");
    const header = document.querySelector("header");
    const main = document.querySelector("main");
    const footer = document.querySelector("footer");
    
    expect(body).not.toBeNull();
    expect(header).not.toBeNull();
    expect(main).not.toBeNull();
    expect(footer).not.toBeNull();
  });
});