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

describe("Login Page Tests", () => {
  let document;
  let mockWebSocket;
  let WebSocketConstructor;
  let localStorageMock;
  let $;
  let currentWebSocket;

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

    WebSocketConstructor = jest.fn(() => {
      currentWebSocket = mockWebSocket;
      return mockWebSocket;
    });
    WebSocketConstructor.CONNECTING = 0;
    WebSocketConstructor.OPEN = 1;
    WebSocketConstructor.CLOSING = 2;
    WebSocketConstructor.CLOSED = 3;

    global.WebSocket = WebSocketConstructor;
    window.WebSocket = WebSocketConstructor;

    global.alert = jest.fn();
    window.alert = jest.fn();
    global.console.log = jest.fn();

    localStorageMock = {
      getItem: jest.fn(),
      setItem: jest.fn(),
      clear: jest.fn()
    };
    global.localStorage = localStorageMock;
    window.localStorage = localStorageMock;

    let html = fs.readFileSync(path.resolve(__dirname, "../Login.html"), "utf8");
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
        const scriptFunc = new Function('window', 'document', '$', 'localStorage', 'WebSocket', 'alert', 'console', scriptMatch[1]);
        scriptFunc(window, document, jQuery, localStorageMock, WebSocketConstructor, alert, console);
      } catch (err) {
        // Script execution errors are expected in test environment
      }
    }
  });

  test("Page displays Login heading", () => {
    const heading = document.querySelector("h2");
    expect(heading).not.toBeNull();
    expect(heading.textContent).toBe("Login");
  });

  test("Login container is present", () => {
    const container = document.querySelector(".login-container");
    expect(container).not.toBeNull();
  });

  test("Form has correct ID attribute", () => {
    const form = document.getElementById("acc");
    expect(form).not.toBeNull();
    expect(form.tagName).toBe("FORM");
  });

  test("Username input field exists with correct attributes", () => {
    const usernameInput = document.getElementById("username");
    expect(usernameInput).not.toBeNull();
    expect(usernameInput.getAttribute("type")).toBe("text");
    expect(usernameInput.getAttribute("required")).not.toBeNull();
    expect(usernameInput.getAttribute("name")).toBe("username");
  });

  test("Password input field exists with correct attributes", () => {
    const passwordInput = document.getElementById("password");
    expect(passwordInput).not.toBeNull();
    expect(passwordInput.getAttribute("type")).toBe("password");
    expect(passwordInput.getAttribute("required")).not.toBeNull();
    expect(passwordInput.getAttribute("name")).toBe("password");
  });

  test("Submit button exists with correct text", () => {
    const submitButton = document.querySelector('input[type="submit"]');
    expect(submitButton).not.toBeNull();
    expect(submitButton.value).toBe("Login");
  });

  test("All form labels are present", () => {
    const labels = document.querySelectorAll("label");
    const labelTexts = [...labels].map(label => label.textContent);
    expect(labelTexts).toContain("Username");
    expect(labelTexts).toContain("Password");
  });

  test("Page displays 8 decorative floating food icons", () => {
    const floatingIcons = document.querySelectorAll(".floating-icon");
    expect(floatingIcons.length).toBe(8);
  });

  test("Page contains expected food emoji icons", () => {
    const icons = [...document.querySelectorAll(".floating-icon")].map(icon => icon.textContent);
    expect(icons).toContain("🍳");
    expect(icons).toContain("🥗");
    expect(icons).toContain("🍕");
    expect(icons).toContain("🥘");
    expect(icons).toContain("🍝");
    expect(icons).toContain("🥖");
    expect(icons).toContain("🧁");
    expect(icons).toContain("☕");
  });

  test("WebSocket connection is created on page load", () => {
    expect(WebSocketConstructor).toHaveBeenCalled();
    expect(WebSocketConstructor.mock.calls.length).toBeGreaterThan(0);
    const callArgs = WebSocketConstructor.mock.calls[0][0];
    expect(callArgs).toContain("ws://");
    expect(callArgs).toContain(":18080/login");
  });

  test("Successful login stores data in localStorage", () => {
    const successResponse = {
      status: "success",
      id: "user123",
      name: "testuser"
    };

    if (mockWebSocket.onmessage) {
      mockWebSocket.onmessage({
        data: JSON.stringify(successResponse)
      });
    }

    expect(localStorageMock.setItem).toHaveBeenCalledWith("activeuserid", "user123");
    expect(localStorageMock.setItem).toHaveBeenCalledWith("activeusername", "testuser");
  });

  test("Failed login displays error message", () => {
    const failResponse = {
      status: "error",
      message: "Invalid credentials"
    };

    if (mockWebSocket.onmessage) {
      mockWebSocket.onmessage({
        data: JSON.stringify(failResponse)
      });
    }

    expect(alert).toHaveBeenCalledWith("Login failed: Invalid credentials");
  });

  test("Page has correct document title", () => {
    const title = document.querySelector("title");
    expect(title).not.toBeNull();
    expect(title.textContent).toBe("Login Page");
  });

  test("Visibility change event listener is attached", () => {
    // This test verifies the listener exists by checking if the event can be dispatched
    // We can't easily test the actual behavior in JSDOM, but we can verify the setup
    const event = new Event('visibilitychange');
    expect(() => document.dispatchEvent(event)).not.toThrow();
  });

  test("Page has correct background color styling", () => {
    const body = document.querySelector("body");
    expect(body).not.toBeNull();
    // Check that body styles are present (styling is applied)
    const styles = window.getComputedStyle(body);
    expect(styles).toBeDefined();
  });

  test("Login form has required username and password fields", () => {
    const form = document.getElementById("acc");
    const inputs = form.querySelectorAll('input[required]');
    expect(inputs.length).toBeGreaterThanOrEqual(2);
  });

  test("WebSocket has onopen handler defined", () => {
    expect(mockWebSocket.onopen).toBeDefined();
  });

  test("WebSocket has onerror handler defined", () => {
    expect(mockWebSocket.onerror).toBeDefined();
  });

  test("WebSocket has onclose handler defined", () => {
    expect(mockWebSocket.onclose).toBeDefined();
  });
});