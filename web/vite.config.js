import { defineConfig } from 'vite';
import { readFileSync, existsSync } from 'fs';
import { resolve } from 'path';

const versionFile = resolve(__dirname, '..', 'VERSION');
const versionMatch = existsSync(versionFile)
  ? (readFileSync(versionFile, 'utf8').match(/v[0-9.]+/) || [])[0]
  : null;
const appVersion = versionMatch || 'v1.0.0';

export default defineConfig({
  base: './',
  server: {
    host: true,
    port: 8000
  },
  preview: {
    host: true,
    port: 8000
  },
  build: {
    outDir: 'dist',
    assetsDir: 'assets',
    target: 'es2018'
  },
  define: {
    __APP_VERSION__: JSON.stringify(appVersion)
  }
});
