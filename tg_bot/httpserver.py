

from http.server    import HTTPServer, BaseHTTPRequestHandler
from hashlib        import sha256
from urllib.parse   import urlparse, parse_qs

import json
import httpx
import random
import bot_db
import telebot

bot = telebot.TeleBot('1233665226:AAEh9GBqvei3oja-HHNbCktiZqegt79fr8s')

def json_to_bytes(j) -> bytes:
    return str.encode(json.dumps(j))

def int_to_bytes(x: int) -> bytes:
    return x.to_bytes((x.bit_length() + 7) // 8, 'big')

def int_to_hashstr(x : int) -> str:
    return sha256(int_to_bytes(x)).hexdigest()

class SimpleHTTPRequestHandler(BaseHTTPRequestHandler):

    def _set_headers(self):
        self.send_response(200)
        self.send_header('Content-type', 'application/json')
        self.end_headers()

    def do_GET(self):
        self._set_headers()
        # Authentication
        if self.path.startswith('/auth_req'):
            
            url_args = parse_qs(urlparse(self.path).query)
            content = {
                'login'       : url_args['login'][0],
                
            }
            login = content['login']
            

            hash_code_user = Database.get_hash_code(login)
             
            
            

            self.wfile.write(json_to_bytes({ 'secret_code' : hash_code_user }))
            pass



        elif self.path.startswith('/new_login'):
            login       = parse_qs(urlparse(self.path).query)['login'][0]
            Database.add_login(login)
            pass

        elif self.path.startswith('/code_gen'):
            # 1. Generate secret code
            # 2. Send it to user
            # 3. Save this secret code and login and send them to WEB server in /auth_req
            login       = parse_qs(urlparse(self.path).query)['login'][0]
            secret_code = random.getrandbits(16)
            hash_code   = int_to_hashstr(secret_code)
            Database.add_hash_code(login, hash_code)

            chat_id     = Database.get_chat_id(login)

            content = {
                    'login'         : login,
                    'secret_code'   : hash_code # hash string obtained with int_to_hashstr function
            }
            bot.send_message(chat_id, ("Secret code: %d" % secret_code))
            #
            pass

        elif self.path.startswith('/reg'):
            # 1. Recieve hashed secret code from WEB
            # 2. Take secret code from user through Telegram Bot
            # 3. Compare hashed secret code from user vs. secret code from this GET request 
            url_args = parse_qs(urlparse(self.path).query)
            content = {
                'login'       : url_args['login'][0],
                'secret_code' : url_args['secret_code'][0]
            }
            
            login = content['login']
            secret_code = content['secret_code']
            print(secret_code)
            if Database.check_reg(login, secret_code) == 1:
                registration_status = False
            else:
                registration_status = True
                chat_id = Database.get_chat_id(login)
                bot.send_message(chat_id,'Your account has been successfully linked')

            self.wfile.write(json_to_bytes({ 'status' : registration_status }))
            pass


Database = bot_db.BotDatabase()

httpd = HTTPServer(('0', 8001), SimpleHTTPRequestHandler)
httpd.serve_forever()