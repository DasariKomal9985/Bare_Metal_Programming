from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import urlparse, parse_qs
import time

HOST = "0.0.0.0"
PORT = 8080
latest = "N/A"

class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        global latest
        p = urlparse(self.path)

        if p.path == "/update":
            q = parse_qs(p.query)
            if "pot" in q:
                latest = q["pot"][0]
                print("[STM32]", latest)
            self.send_response(200)
            self.end_headers()
            self.wfile.write(b"OK")

        elif p.path == "/page":
            self.send_response(200)
            self.send_header("Content-type","text/html")
            self.end_headers()
            self.wfile.write(f"""
            <html>
            <head><meta http-equiv="refresh" content="2"></head>
            <body>
            <h1>STM32 ADC</h1>
            <h2>Pot = {latest} %</h2>
            <p>{time.strftime('%H:%M:%S')}</p>
            </body>
            </html>
            """.encode())

        else:
            self.send_response(404)
            self.end_headers()

print("Server running on port", PORT)
HTTPServer((HOST, PORT), Handler).serve_forever()
