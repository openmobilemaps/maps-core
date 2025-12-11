const fs = require("node:fs");
const path = require("node:path");

// --- Parse CLI args ---
const args = process.argv.slice(2);
const srcArgs = args.filter((arg) => arg.startsWith("--src="));
const outArg = args.find((arg) => arg.startsWith("--out="));
const importPrefixArgs = args.filter((arg) =>
  arg.startsWith("--import-prefix=")
);

if (srcArgs.length === 0 || !outArg) {
  console.error(
    "❌ Usage: node generate-module.js --src=./src1 --import-prefix=@prefix1 --src=./src2 --import-prefix=@prefix2 --out=./dist/index.d.ts"
  );
  process.exit(1);
}

if (importPrefixArgs.length !== srcArgs.length) {
  console.error(
    "❌ Error: Number of import prefixes must match number of source directories"
  );
  process.exit(1);
}

const inputDirs = srcArgs.map((arg) => path.resolve(arg.split("=")[1]));
const importPrefixes = importPrefixArgs.map((arg) => arg.split("=")[1]);
const outputFile = outArg.split("=")[1];

// --- Core ---
const staticsMap = new Map();

function toPathFromBaseDir(baseDir, filePath) {
  return path.relative(baseDir, filePath);
}

function processFile(filePath, baseDir, importPrefix) {
  const content = fs.readFileSync(filePath, "utf8");
  const regex = /export\s+interface\s+(\w+)_statics\s*{/g;
  let match;
  while ((match = regex.exec(content)) !== null) {
    const X = match[1];
    if (!staticsMap.has(X)) {
      staticsMap.set(X, { file: filePath, baseDir, importPrefix });
    }
  }
}

function traverseDirectory(dir, baseDir, importPrefix) {
  const files = fs.readdirSync(dir);
  for (const file of files) {
    const fullPath = path.join(dir, file);
    const stat = fs.statSync(fullPath);

    if (stat.isDirectory()) {
      traverseDirectory(fullPath, baseDir, importPrefix);
    } else if (
      stat.isFile() &&
      fullPath.endsWith(".ts") &&
      fullPath !== outputFile
    ) {
      processFile(fullPath, baseDir, importPrefix);
    }
  }
}

// --- Run ---
for (let i = 0; i < inputDirs.length; i++) {
  traverseDirectory(inputDirs[i], inputDirs[i], importPrefixes[i]);
}

const sorted = Array.from(staticsMap.entries()).sort(([a], [b]) =>
  a.localeCompare(b)
);

// Group by import path
const importGroups = new Map();
for (const [X, { file, baseDir, importPrefix }] of sorted) {
  const pathFromBaseDir = toPathFromBaseDir(baseDir, file);
  const importPath = `${importPrefix}/${pathFromBaseDir}`;

  if (!importGroups.has(importPath)) {
    importGroups.set(importPath, []);
  }
  importGroups.get(importPath).push(X);
}

const lines = [];
// Generate grouped imports
for (const [importPath, types] of importGroups) {
  const typesList = types.map((X) => `${X}_statics`).join(", ");
  const importPathNoExt = importPath.replace(/(\.d\.ts|\.ts)$/, "");
  lines.push(`import type { ${typesList} } from '${importPathNoExt}';`);
}

lines.push("", "export interface MainModule {");
for (const [X] of sorted) {
  lines.push(`  ${X}: ${X}_statics;`);
}
lines.push("}");

lines.push(
  "",
  "declare function MainModuleFactory(options: { preinitializedWebGLContext: WebGL2RenderingContext }): Promise<MainModule>;",
  "export default MainModuleFactory;"
);

fs.writeFileSync(outputFile, `${lines.join("\n")}\n`, "utf8");

console.log(`✅ Wrote ${sorted.length} entries to ${outputFile}`);
