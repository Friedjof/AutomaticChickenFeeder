# Chicken Feeder Web Interface (Vite)

Single Page App for the ESP32 chicken feeder, built with Vite and shipped as static files (ideal for gzipping into a `.h` for the ESP).

## Structure
```
web/
├── index.html          # Vite entry (loads src/main.js)
├── src/
│   ├── main.js         # Bootstraps the app
│   ├── app.js          # UI + API logic
│   └── style.css       # Styling
├── public/
│   └── mock/api.js     # Optional mock API (localStorage)
├── compose.yml         # Dev server via Docker (Vite)
├── vite.config.js
└── package.json
```

## Development (local)
```bash
cd web
npm install
npm run dev -- --host --port 8000
# open http://localhost:8000
```

### Development (Docker)
```bash
cd web
docker compose up        # starts Vite dev server on :8000 (installs deps in container)
```

## Mock API
- Add `?mock=1` to the URL to force the browser mock (`public/mock/api.js`, stored in localStorage).
- Without mock: the app calls `/api/*`. Use the Node mock from the repo root (`docker compose up` there) to provide `/api`.

## Build
```bash
cd web
npm run build            # outputs to web/dist
npm run preview          # optional static preview
```

Feeding guidance (for configuring schedules/portions): see `docs/feeding-guide.md`.

### ESP32 header generation
- `npm run build:esp` (runs Vite build, then `scripts/web-to-header.py dist -o lib/WebService/generated`)
- or `make web-headers` from repo root (does the same).

> Note: The root `docker compose` now serves `web/dist`, so run `npm run build` first if you use it.

## Expected API (real device or Node mock)
```
GET  /api/status
GET  /api/config
POST /api/config
POST /api/config/reset
POST /api/feed
```
