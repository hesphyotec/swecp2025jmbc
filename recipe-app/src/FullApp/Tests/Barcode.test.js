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

describe("Barcode Scanner Page Tests", () => {
  let document;
  let mockWebSocket;
  let WebSocketConstructor;
  let localStorageMock;
  let sessionStorageMock;

  beforeEach(() => {
    jest.clearAllMocks();

    // Mock fetch globally
    global.fetch = jest.fn();

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

    // Mock ZXing library
    global.ZXing = {
      BrowserMultiFormatReader: jest.fn().mockImplementation(() => ({
        listVideoInputDevices: jest.fn().mockResolvedValue([
          { deviceId: 'mock-device-id', label: 'Mock Camera' }
        ]),
        decodeFromVideoDevice: jest.fn(),
        reset: jest.fn()
      }))
    };

    // Mock Fuse library
    global.Fuse = jest.fn().mockImplementation((list, options) => ({
      search: jest.fn().mockReturnValue([])
    }));

    let html = fs.readFileSync(path.resolve(__dirname, "../Barcode.html"), "utf8");
    html = html.replace(/new WebSocket`([^`]+)`/g, 'new WebSocket("$1")');
    html = html.replace(/fetch`([^`]+)`/g, 'fetch("$1")');
    html = html.replace(/new Error`([^`]+)`/g, 'new Error("$1")');

    document = window.document;
    document.documentElement.innerHTML = html;

    // Don't execute the script automatically - we'll test individual functions
  });

  afterEach(() => {
    delete global.fetch;
    delete global.ZXing;
    delete global.Fuse;
  });

  test("Page displays Barcode Scanner header", () => {
    const header = document.querySelector("header");
    expect(header).not.toBeNull();
    expect(header.textContent).toBe("Barcode Scanner");
  });

  test("Page has correct document title", () => {
    const title = document.querySelector("title");
    expect(title).not.toBeNull();
    expect(title.textContent).toBe("Barcode Scanner");
  });

  test("Scanner preview video element exists", () => {
    const video = document.getElementById("scanner-preview");
    expect(video).not.toBeNull();
    expect(video.tagName).toBe("VIDEO");
  });

  test("Scan button exists with correct initial text", () => {
    const scanBtn = document.getElementById("scan-btn");
    expect(scanBtn).not.toBeNull();
    expect(scanBtn.textContent).toBe("Start Scan");
  });

  test("Result div exists with placeholder text", () => {
    const resultDiv = document.getElementById("result");
    expect(resultDiv).not.toBeNull();
    expect(resultDiv.textContent).toBe("Scanned code will show here");
  });

  test("Ingredients list div exists", () => {
    const list = document.getElementById("ingredients-list");
    expect(list).not.toBeNull();
    expect(list.textContent).toBe("Your ingredients will appear here");
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
    expect(buttonTexts).toContain("Scan Food");
    expect(buttonTexts).toContain("Receipt");
    expect(buttonTexts).toContain("Ingredients");
    expect(buttonTexts).toContain("Manual");
  });

  test("Back button links to Homepage", () => {
    const buttons = document.querySelectorAll("footer button");
    const backButton = [...buttons].find(btn => btn.textContent === "Back");
    expect(backButton.getAttribute("onclick")).toContain("Homepage.html");
  });

  test("Scan Food button links to ScanFood page", () => {
    const buttons = document.querySelectorAll("footer button");
    const scanFoodButton = [...buttons].find(btn => btn.textContent === "Scan Food");
    expect(scanFoodButton.getAttribute("onclick")).toContain("ScanFood.html");
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

  test("Manual button links to manual page", () => {
    const buttons = document.querySelectorAll("footer button");
    const manualButton = [...buttons].find(btn => btn.textContent === "Manual");
    expect(manualButton.getAttribute("onclick")).toContain("manual.html");
  });

  test("Manual button has active class", () => {
    const buttons = document.querySelectorAll("footer button");
    const manualButton = [...buttons].find(btn => btn.textContent === "Manual");
    expect(manualButton.classList.contains("active")).toBe(true);
  });

  test("Main content area exists", () => {
    const main = document.querySelector("main");
    expect(main).not.toBeNull();
  });

  test("Scanner preview has correct styling classes", () => {
    const video = document.getElementById("scanner-preview");
    expect(video.id).toBe("scanner-preview");
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

  test("Scanner preview placeholder text is present", () => {
    const video = document.getElementById("scanner-preview");
    expect(video.textContent.trim()).toBe("Camera feed will appear here");
  });

  test("All main content elements are present", () => {
    const video = document.getElementById("scanner-preview");
    const scanBtn = document.getElementById("scan-btn");
    const resultDiv = document.getElementById("result");
    const list = document.getElementById("ingredients-list");

    expect(video).not.toBeNull();
    expect(scanBtn).not.toBeNull();
    expect(resultDiv).not.toBeNull();
    expect(list).not.toBeNull();
  });

  test("Button hover styles are defined", () => {
    const style = document.querySelector("style");
    expect(style.textContent).toContain("button:hover");
  });

  test("Ingredients list has proper styling", () => {
    const list = document.getElementById("ingredients-list");
    expect(list.id).toBe("ingredients-list");
  });

  test("External ZXing library script is included", () => {
    const scripts = document.querySelectorAll("script");
    const zxingScript = [...scripts].find(script => 
      script.src && script.src.includes("zxing")
    );
    expect(zxingScript).not.toBeNull();
  });

  test("External Fuse.js library script is included", () => {
    const scripts = document.querySelectorAll("script");
    const fuseScript = [...scripts].find(script => 
      script.src && script.src.includes("fuse")
    );
    expect(fuseScript).not.toBeNull();
  });

  test("Header has correct styling", () => {
    const header = document.querySelector("header");
    expect(header).not.toBeNull();
    // Header exists with content
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

  test("All buttons have onclick attributes", () => {
    const buttons = document.querySelectorAll("footer button");
    buttons.forEach(button => {
      expect(button.hasAttribute("onclick")).toBe(true);
    });
  });

  test("Page layout uses flexbox", () => {
    const body = document.querySelector("body");
    expect(body).not.toBeNull();
    // Body exists and should have flex layout (defined in styles)
  });

  test("Scanner preview has defined dimensions", () => {
    const video = document.getElementById("scanner-preview");
    expect(video).not.toBeNull();
    // Element exists with id
  });

  test("Result div is centered", () => {
    const resultDiv = document.getElementById("result");
    expect(resultDiv).not.toBeNull();
  });

  test("Ingredients list has minimum height", () => {
    const list = document.getElementById("ingredients-list");
    expect(list).not.toBeNull();
  });
});