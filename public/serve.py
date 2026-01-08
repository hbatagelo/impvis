# public/serve.py
import http.server
import socketserver
import mimetypes

mimetypes.add_type('application/json', '.wasm.map')

class Handler(http.server.SimpleHTTPRequestHandler):
    def end_headers(self):
        # Enable COOP/COEP for shared array buffers
        self.send_header('Cross-Origin-Opener-Policy', 'same-origin')
        self.send_header('Cross-Origin-Embedder-Policy', 'require-corp')
        super().end_headers()

PORT = 8000
with socketserver.TCPServer(("", PORT), Handler) as httpd:
    print(f"Serving at http://localhost:{PORT}")
    httpd.serve_forever()