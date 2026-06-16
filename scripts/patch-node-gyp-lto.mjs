import { existsSync, readdirSync, readFileSync, writeFileSync } from "node:fs";
import { join, dirname } from "node:path";
import { execSync } from "node:child_process";
import { fileURLToPath } from "node:url";
import { createRequire } from "node:module";
import { homedir } from "node:os";

// This runs as the npm `install` lifecycle script. Defining `install` also stops
// npm from auto-running its implicit `node-gyp rebuild` (which it does whenever a
// binding.gyp exists and no install/preinstall script is defined). That implicit
// build runs *before* postinstall, so the LTO patch below must live here, ahead of
// it — the actual addon build then happens in the postinstall (ensure) script.
//
// Why patch at all: Node.js 26 release builds enable ThinLTO, so the common.gypi
// bundled with its headers adds LLVM flags (-flto=thin) and the LLD-only linker
// option /opt:lldltojobs to every native addon node-gyp builds. On Windows the
// addon is compiled with MSVC, whose link.exe rejects /opt:lldltojobs with fatal
// error LNK1117. Those flags are gated on enable_lto/enable_thin_lto variables that
// node-gyp writes into config.gypi *without* the overridable "%" marker, so they
// can't be turned off via -D/GYP_DEFINES. Instead we pre-fetch the headers and
// rewrite the LTO condition tests in the cached common.gypi to a sentinel value
// that no variable ever equals, so the LTO blocks never fire. No-op on Node
// versions / platforms that don't carry these flags.

const __filename = fileURLToPath(import.meta.url);
const __dirname = dirname(__filename);
const require = createRequire(import.meta.url);
const projectRoot = join(__dirname, "..");

function resolveNodeGypBin() {
  try {
    return join(
      dirname(require.resolve("node-gyp/package.json")),
      "bin",
      "node-gyp.js",
    );
  } catch {
    return null;
  }
}

// Recursively locate common.gypi files in the node-gyp header cache for the
// running Node version. The cache layout has varied across node-gyp releases
// (e.g. <devdir>/<version>/... vs <devdir>/Cache/<version>/...), so we search
// rather than assume a fixed path.
function findCommonGypi(dir, version, out, depth = 0) {
  if (depth > 6) return;
  let entries;
  try {
    entries = readdirSync(dir, { withFileTypes: true });
  } catch {
    return;
  }
  for (const entry of entries) {
    const full = join(dir, entry.name);
    if (entry.isDirectory()) {
      findCommonGypi(full, version, out, depth + 1);
    } else if (entry.name === "common.gypi" && full.includes(version)) {
      out.add(full);
    }
  }
}

function patchNodeGypLto() {
  if (process.platform !== "win32") return;

  const nodeGypBin = resolveNodeGypBin();
  if (!nodeGypBin) return;

  // Ensure the headers (incl. common.gypi) are present in the node-gyp cache so we
  // can patch them before the build downloads/uses them. node-gyp's later configure
  // step reuses this cache instead of re-downloading.
  try {
    execSync(`"${process.execPath}" "${nodeGypBin}" install`, {
      stdio: "inherit",
      cwd: projectRoot,
    });
  } catch (error) {
    console.warn(
      `Warning: 'node-gyp install' failed, continuing anyway: ${error.message}`,
    );
  }

  const version = process.versions.node;
  const roots = [
    process.env.npm_config_devdir,
    process.env.LOCALAPPDATA && join(process.env.LOCALAPPDATA, "node-gyp"),
    join(homedir(), "AppData", "Local", "node-gyp"),
    join(homedir(), ".node-gyp"),
  ].filter((p) => p && existsSync(p));

  const targets = new Set();
  for (const root of roots) {
    findCommonGypi(root, version, targets);
  }

  for (const file of targets) {
    try {
      const original = readFileSync(file, "utf8");
      const patched = original
        .replaceAll(
          'enable_thin_lto=="true"',
          'enable_thin_lto=="__gst_kit_lto_off__"',
        )
        .replaceAll('enable_lto=="true"', 'enable_lto=="__gst_kit_lto_off__"');
      if (patched !== original) {
        writeFileSync(file, patched);
        console.log(`Disabled incompatible node-gyp LTO flags in ${file}`);
      }
    } catch (error) {
      console.warn(`Warning: could not patch ${file}: ${error.message}`);
    }
  }
}

patchNodeGypLto();
