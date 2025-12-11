module.exports = {
  testEnvironment: 'jsdom',
  setupFilesAfterEnv: ['<rootDir>/jest.setup.js'],
  testMatch: ['**/__tests__/**/*.js', '**/Tests/**/*.js', '**/?(*.)+(spec|test).js'],
  moduleFileExtensions: ['js', 'json'],
  transform: {
    '^.+\\.jsx?$': 'babel-jest',
  },
  collectCoverageFrom: [
    '**/*.{js,jsx}',
    '!**/node_modules/**',
    '!**/coverage/**',
  ],
};