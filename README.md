# Node.js Native Addon with TypeScript

This project demonstrates how to create a Node.js native addon using C++ and TypeScript, built with cmake-js.

## Prerequisites

- Node.js (v14 or higher)
- npm
- CMake (3.1 or higher)
- C++ compiler (GCC, Clang, or MSVC)

## Installation

1. Clone the repository
2. Install dependencies:

```bash
npm install
```

## Building

To build the project, run:

```bash
npm run build
```

This will:

1. Clean the build and dist directories
2. Compile the C++ code using cmake-js
3. Compile the TypeScript code

## Testing

Confirm library is working in both CJS, EMS and unit tests are passing:

```bash
npm run test
```

## Project Structure

- `src/cpp/` - C++ source code for the native addon
  - `addon.cpp` - C++ to Node binding
  - `add.cpp` and `add.hpp` - Native C++ code
- `src/ts/` - TypeScript source code
  - `index.ts` - TypeScript interface for the native addon
- `modules/` - Assertion tests of dist for both CJS and ESM
- `CMakeLists.txt` - CMake configuration
- `tsconfig.json` - TypeScript configuration
- `package.json` - Project configuration and dependencies
