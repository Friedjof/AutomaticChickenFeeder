import { defineConfig } from 'vite';

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
  }
});
