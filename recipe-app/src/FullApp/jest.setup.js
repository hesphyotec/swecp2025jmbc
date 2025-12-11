require('@testing-library/jest-dom');

// Mock console methods to reduce test output noise (optional)
global.console = {
  ...console,
  log: jest.fn(),
  error: jest.fn(),
  warn: jest.fn(),
};