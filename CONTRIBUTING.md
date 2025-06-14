# Contributing to Node.js C++ Addon Template

Thank you for your interest in contributing to this project! This document provides guidelines and instructions for contributing.

## Code of Conduct

By participating in this project, you agree to abide by our Code of Conduct.

## Development Setup

1. Fork the repository
1. Clone your fork:

```bash
git clone https://github.com/Repugraf/gst-kit.git
```

1. Install dependencies:

```bash
npm install
```

## Development Workflow

1. Create a new branch for your feature/fix:

```bash
git checkout -b feature/your-feature-name
```

1. Make your changes

1. Run tests:

   ```bash
   npm test
   ```

1. Commit your changes with a descriptive commit message
1. Push to your fork
1. Create a Pull Request

## Building

The project uses cmake-js for building the native addon and Rollup for TypeScript bundling:

```bash
npm run build
```

## Testing

Run the test suite:

```bash
npm test
```

This will:

- Build the native addon
- Run unit tests
- Test both CommonJS and ESM modules

## Pull Request Process

1. Update the README.md with details of changes if needed
2. Update the documentation if needed
3. The PR will be merged once you have the sign-off of at least one maintainer

## Style Guide

- Use TypeScript for all JavaScript/TypeScript code
- Follow the existing code style
- Write tests for new features
- Update documentation for new features

## Questions?

Feel free to open an issue for any questions you might have.
