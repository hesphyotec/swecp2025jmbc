/**
 * @jest-environment jsdom
 */
const fs = require("fs");
const path = require("path");

describe("Home Page Tests", () => {
  let document;

  beforeAll(() => {
    // Load the HTML file
    const html = fs.readFileSync(
      path.resolve(__dirname, "../home.html"),
      "utf8"
    );
    
    // Set it directly to the JSDOM document
    document = window.document;
    document.documentElement.innerHTML = html;
  });

  // TEST 1: Page has main heading
  test("Page displays Recipe Builder heading", () => {
    const heading = document.querySelector("h1");
    
    expect(heading).not.toBeNull();
    expect(heading.textContent).toBe("Recipe Builder");
  });

  // TEST 2: Sign In button exists and links correctly
  test("Sign In button exists and links to Login.html", () => {
    const signInButton = [...document.querySelectorAll("a")].find(a => 
      a.textContent.includes("Sign In")
    );
    
    expect(signInButton).toBeDefined();
    expect(signInButton.getAttribute("href")).toBe("Login.html");
    expect(signInButton.classList.contains("home-button")).toBe(true);
  });

  // TEST 3: Create Account button exists and links correctly
  test("Create Account button exists and links to signup.html", () => {
    const createAccountButton = [...document.querySelectorAll("a")].find(a => 
      a.textContent.includes("Create Account")
    );
    
    expect(createAccountButton).toBeDefined();
    expect(createAccountButton.getAttribute("href")).toBe("signup.html");
    expect(createAccountButton.classList.contains("home-button")).toBe(true);
  });

  // TEST 4: Both navigation buttons exist
  test("Both navigation buttons are present", () => {
    const buttons = document.querySelectorAll(".home-button");
    
    expect(buttons.length).toBe(2);
  });

  // TEST 5: Footer exists and contains expected text
  test("Footer contains Recipe Builder branding", () => {
    const footer = document.querySelector("footer");
    
    expect(footer).not.toBeNull();
    expect(footer.textContent).toContain("Recipe Builder");
    expect(footer.textContent).toContain("Your personal cooking companion");
  });

  // TEST 6: Footer has description text
  test("Footer contains descriptive text", () => {
    const footer = document.querySelector("footer");
    
    expect(footer).not.toBeNull();
    
    const paragraphs = footer.querySelectorAll("p");
    
    expect(paragraphs.length).toBeGreaterThanOrEqual(2);
    
    const footerText = footer.textContent;
    expect(footerText).toContain("Discover, create, and organize your favorite recipes");
  });

  // TEST 7: Page has floating food icons
  test("Page displays decorative floating food icons", () => {
    const floatingIcons = document.querySelectorAll(".floating-icon");
    
    expect(floatingIcons.length).toBe(8);
  });

  // TEST 8: Specific food emojis are present
  test("Page contains expected food emoji icons", () => {
    const icons = [...document.querySelectorAll(".floating-icon")].map(
      icon => icon.textContent
    );
    
    expect(icons).toContain("🍳");
    expect(icons).toContain("🥗");
    expect(icons).toContain("🍕");
    expect(icons).toContain("🥘");
    expect(icons).toContain("🍝");
    expect(icons).toContain("🥖");
    expect(icons).toContain("🧁");
    expect(icons).toContain("☕");
  });

  // TEST 9: Content is wrapped in content div
  test("Main content is contained within content div", () => {
    const contentDiv = document.querySelector(".content");
    
    expect(contentDiv).not.toBeNull();
    
    const heading = contentDiv.querySelector("h1");
    const buttonContainer = contentDiv.querySelector(".button-container");
    
    expect(heading).not.toBeNull();
    expect(buttonContainer).not.toBeNull();
  });

  // TEST 10: Button container has correct structure
  test("Button container holds both action buttons", () => {
    const buttonContainer = document.querySelector(".button-container");
    
    expect(buttonContainer).not.toBeNull();
    
    const buttons = buttonContainer.querySelectorAll(".home-button");
    expect(buttons.length).toBe(2);
  });

  // TEST 11: Page has correct title
  test("Page has correct document title", () => {
    const title = document.querySelector("title");
    
    expect(title).not.toBeNull();
    expect(title.textContent).toBe("Recipe Builder - Home");
  });

  // TEST 12: Body has correct background color style
  test("Body element exists with proper styling class", () => {
    const body = document.querySelector("body");
    
    expect(body).not.toBeNull();
    
    // Check if style tag exists with background-color definition
    const styleTag = document.querySelector("style");
    expect(styleTag).not.toBeNull();
    expect(styleTag.textContent).toContain("background-color: #ffddb4");
  });
});