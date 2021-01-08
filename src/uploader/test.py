#!/usr/bin/env python
import os
import datetime
import http.server as server

class HTTPRequestHandler(server.SimpleHTTPRequestHandler):
    def do_POST(self):
        filename = str(int(datetime.datetime.now().timestamp())) + ".jpg"
        file_length = int(self.headers['Content-Length'])

        with open(filename, 'wb') as output_file:
            output_file.write(self.rfile.read(file_length))

        self.send_response(201, 'Created')
        self.end_headers()

        reply_body = 'Saved "%s"\n' % filename
        self.wfile.write(reply_body.encode('utf-8'))

if __name__ == '__main__':
    server.test(HandlerClass=HTTPRequestHandler)
