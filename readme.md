# Web Server

A simple HTTP web server written in C that serves static files from the `hosted/` directory.

## Features

- Serves any file from the `hosted/` directory
- Automatically handles directory requests (looks for `index.html`)
- Tries common file extensions (e.g., `/test` looks for `test.html`)
- Proper HTTP headers with correct MIME types
- Graceful shutdown with Ctrl+C

## Building

```bash
tmake build
```

## Running

```bash
./bin/my_program
```

The server will start on `http://localhost:8080`

## How to Use

1. Put your HTML, CSS, images, and other files in the `hosted/` directory
2. Run the server
3. Open `http://localhost:8080` in your browser

For example:
- `http://localhost:8080/` → serves `hosted/index.html`
- `http://localhost:8080/about` → tries `hosted/about.html`
- `http://localhost:8080/test/` → serves `hosted/test/index.html`
- `http://localhost:8080/style.css` → serves `hosted/style.css`
