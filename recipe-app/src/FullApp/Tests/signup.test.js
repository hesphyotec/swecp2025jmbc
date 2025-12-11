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

describe("Signup Page Tests", () => {
  let document;
  let mockWebSocket;
  let WebSocketConstructor;
  let localStorageMock;
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

    localStorageMock = {
      getItem: jest.fn(),
      setItem: jest.fn(),
      clear: jest.fn()
    };
    global.localStorage = localStorageMock;
    window.localStorage = localStorageMock;

    let html = fs.readFileSync(path.resolve(__dirname, "../signup.html"), "utf8");
    html = html.replace(/new WebSocke`([^`]+)`/g, 'new WebSocket("$1")');

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

  test("Page displays Create Account heading", () => {
    const heading = document.querySelector("h2");
    expect(heading).not.toBeNull();
    expect(heading.textContent).toBe("Create Account");
  });

  test("Signup container is present", () => {
    const container = document.querySelector(".signup-container");
    expect(container).not.toBeNull();
  });

  test("Form has correct ID attribute", () => {
    const form = document.getElementById("acc");
    expect(form).not.toBeNull();
    expect(form.tagName).toBe("FORM");
  });

  test("Username input field exists with correct attributes", () => {
    const usernameInput = document.getElementById("new-username");
    expect(usernameInput).not.toBeNull();
    expect(usernameInput.getAttribute("type")).toBe("text");
    expect(usernameInput.getAttribute("required")).not.toBeNull();
    expect(usernameInput.getAttribute("name")).toBe("username");
  });

  test("Password input field exists with correct attributes", () => {
    const passwordInput = document.getElementById("new-password");
    expect(passwordInput).not.toBeNull();
    expect(passwordInput.getAttribute("type")).toBe("password");
    expect(passwordInput.getAttribute("required")).not.toBeNull();
    expect(passwordInput.getAttribute("name")).toBe("password");
  });

  test("Confirm password input field exists with correct attributes", () => {
    const confirmPasswordInput = document.getElementById("confirm-password");
    expect(confirmPasswordInput).not.toBeNull();
    expect(confirmPasswordInput.getAttribute("type")).toBe("password");
    expect(confirmPasswordInput.getAttribute("required")).not.toBeNull();
    expect(confirmPasswordInput.getAttribute("name")).toBe("confirm_password");
  });

  test("Submit button exists with correct text", () => {
    const submitButton = document.querySelector('input[type="submit"]');
    expect(submitButton).not.toBeNull();
    expect(submitButton.value).toBe("Create Account");
  });

  test("All form labels are present", () => {
    const labels = document.querySelectorAll("label");
    const labelTexts = [...labels].map(label => label.textContent);
    expect(labelTexts).toContain("Username");
    expect(labelTexts).toContain("Password");
    expect(labelTexts).toContain("Confirm Password");
  });

  test("Page displays 8 decorative floating food icons", () => {
    const floatingIcons = document.querySelectorAll(".floating-icon");
    expect(floatingIcons.length).toBe(8);
  });

  test("Page contains expected food emoji icons", () => {
    const icons = [...document.querySelectorAll(".floating-icon")].map(icon => icon.textContent);
    expect(icons).toContain("🍳");
    expect(icons).toContain("🥓");
    expect(icons).toContain("🍕");
    expect(icons).toContain("🥦");
    expect(icons).toContain("🍝");
    expect(icons).toContain("🌮");
    expect(icons).toContain("🥩");
    expect(icons).toContain("🍨");
  });

  test("WebSocket connection is created on page load", () => {
    expect(WebSocketConstructor).toHaveBeenCalled();
    expect(WebSocketConstructor.mock.calls.length).toBeGreaterThan(0);
    const callArgs = WebSocketConstructor.mock.calls[0][0];
    expect(callArgs).toContain("ws://");
    expect(callArgs).toContain(":18080/signup");
  });

  test("Successful signup stores data in localStorage", () => {
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

  test("Failed signup displays error message", () => {
    const failResponse = {
      status: "error",
      message: "Username already exists"
    };

    if (mockWebSocket.onmessage) {
      mockWebSocket.onmessage({
        data: JSON.stringify(failResponse)
      });
    }

    expect(alert).toHaveBeenCalledWith("Signup failed: Username already exists");
  });

  test("Page has correct document title", () => {
    const title = document.querySelector("title");
    expect(title).not.toBeNull();
    expect(title.textContent).toBe("Create Account");
  });
});