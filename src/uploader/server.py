#!/usr/bin/env python
import os
import http.server as server
import requests
from datetime import datetime, timedelta

IMGUR_CLIENT_ID = os.environ['IMGUR_CLIENT_ID']
IMGUR_CLIENT_SECRET = os.environ['IMGUR_CLIENT_SECRET']
IMGUR_REFRESH_TOKEN = os.environ['IMGUR_REFRESH_TOKEN']
IMGUR_ACCESS_TOKEN = False

def p(s):
    print(s, flush=True)

class HTTPRequestHandler(server.SimpleHTTPRequestHandler):
    def get_access_token(self):
        global IMGUR_ACCESS_TOKEN
        now = datetime.now() - timedelta(seconds=90)

        if not IMGUR_ACCESS_TOKEN or IMGUR_ACCESS_TOKEN['expires'] < now:
            url = "https://api.imgur.com/oauth2/token/"
            data = {
                "client_id": IMGUR_CLIENT_ID,
                "client_secret": IMGUR_CLIENT_SECRET,
                "refresh_token": IMGUR_REFRESH_TOKEN,
                "grant_type": "refresh_token",
            }

            r = requests.post(url, json=data)
            IMGUR_ACCESS_TOKEN = r.json()
            IMGUR_ACCESS_TOKEN['expires'] = datetime.now() + timedelta(seconds=IMGUR_ACCESS_TOKEN['expires_in'])

        return IMGUR_ACCESS_TOKEN['access_token']

    def do_POST(self):
        filename = '/tmp/{}.jpg'.format(str(int(datetime.now().timestamp())))
        file_length = int(self.headers['Content-Length'])

        p("- Received req with filename {} ({})".format(filename, file_length))

        with open(filename, 'wb') as output_file:
            output_file.write(self.rfile.read(file_length))

        self.send_response(201, 'Created')
        self.end_headers()

        reply_body = 'Saved "%s"\n' % filename
        self.wfile.write(reply_body.encode('utf-8'))

        # kinda wonky to write to disk then re-upload
        # but k.i.s.s. for now

        access_token = self.get_access_token()

        headers = {
            'Authorization': 'Bearer {}'.format(access_token)
        }

        post_data = {
            'image': open(filename, 'rb'),
            'album': 'tycequJ1z0dl4Bx',
            'type': 'file',
            'name': filename,
        }

        r = requests.post('https://api.imgur.com/3/upload', headers=headers, files=post_data)
        p(r.text)

        if r.status_code == 201 or r.status_code == 200:
            r = requests.post('https://api.imgur.com/3/album/tycequJ1z0dl4Bx/add', headers=headers, data = {
                "deletehashes[]": r.json()['data']['deletehash']
            })
            p(r.text)

if __name__ == '__main__':
    server.test(HandlerClass=HTTPRequestHandler)
