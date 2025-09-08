#!/usr/bin/env node

import { execSync } from "node:child_process";
import { existsSync, mkdirSync } from "node:fs";
import { join, dirname, resolve } from "node:path";
import { fileURLToPath } from "node:url";
import { cpus } from "node:os";

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);
const projectRoot = resolve(__dirname, "..");

const config = {
  buildDir: join(projectRoot, "build"),
  sourceDir: projectRoot,
  nodeAddonApi: join(projectRoot, "node_modules", "node-addon-api"),
  buildType: process.env.CMAKE_BUILD_TYPE || "Release",
  verbose: process.argv.includes("--verbose") || process.env.VERBOSE === "1",
  version: process.version,
  libPath: process.execPath,
  includePath: join(dirname(dirname(process.execPath)), "include", "node"),
};

console.log(`Building for ${process.platform}/${process.arch}`);

const runCommand = (command, args, options = {}) => {
  // Properly quote arguments that contain spaces
  const quotedArgs = args.map(arg => (arg.includes(" ") ? `"${arg}"` : arg));
  const fullCommand = `${command} ${quotedArgs.join(" ")}`;

  if (config.verbose) console.log(`Running: ${fullCommand}`);

  try {
    execSync(fullCommand, {
      stdio: options.stdio || "inherit",
      cwd: options.cwd || config.sourceDir,
      encoding: "utf8",
      ...options,
    });
  } catch (error) {
    throw new Error(`Command failed: ${fullCommand}\n${error.message}`);
  }
};

const getCMakeArgs = () => {
  const args = [
    "-S",
    config.sourceDir,
    "-B",
    config.buildDir,
    `-DCMAKE_BUILD_TYPE=${config.buildType}`,
    `-DNAPI_INCLUDE_DIR=${config.includePath}`,
    `-DNODE_ADDON_API_DIR=${config.nodeAddonApi}`,
  ];

  return args;
};

const getBuildArgs = () => {
  const args = ["--build", config.buildDir, "--config", config.buildType];

  args.push("--parallel", cpus().length.toString());

  if (config.verbose) args.push("--verbose");

  return args;
};

const ensureBuildDirectory = () => {
  if (!existsSync(config.buildDir)) {
    console.log("Creating build directory...");
    mkdirSync(config.buildDir, { recursive: true });
  }
};

const checkDependencies = () => {
  console.log("Checking dependencies...");

  if (!existsSync(config.nodeAddonApi))
    throw new Error(
      "node-addon-api not found. Please run: npm install (or equivalent for your runtime)"
    );

  try {
    runCommand("cmake", ["--version"], { stdio: "pipe" });
  } catch (error) {
    throw new Error("CMake not found. Please install CMake.");
  }

  console.log("✓ Dependencies check passed");
};

const configureBuild = () => {
  console.log("Configuring build...");
  const args = getCMakeArgs();

  if (config.verbose) console.log("CMake configure arguments:", args);

  runCommand("cmake", args);
  console.log("✓ Build configured");
};

const build = () => {
  console.log("Building...");
  const args = getBuildArgs();

  if (config.verbose) console.log("CMake build arguments:", args);

  runCommand("cmake", args);
  console.log("✓ Build completed");
};

// Handle CLI arguments
if (process.argv.includes("--help") || process.argv.includes("-h")) {
  console.log(`
Usage: node scripts/build-native.mjs [options]

Options:
  --verbose         Enable verbose output
  --help, -h        Show this help message

Environment Variables:
  CMAKE_BUILD_TYPE  Set build type (Debug|Release) [default: Release]
  VERBOSE           Enable verbose output (set to 1)
`);
  process.exit(0);
}

try {
  console.log("🔨 Building N-API native addon...");

  ensureBuildDirectory();
  checkDependencies();
  configureBuild();
  build();

  console.log("🎉 Native build completed successfully!");
} catch (error) {
  console.error("❌ Build failed:", error.message);

  if (config.verbose) console.error(error.stack);

  process.exit(1);
}
