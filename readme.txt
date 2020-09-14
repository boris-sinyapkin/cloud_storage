#
# Http lib: httpx
# Hash: from hashlib import sha256
#
# sha256(...).hexdigest()
#
# Authentication
# WEB           -> generation code request      -> Telegram         Method: GET  URL: TELEGRAM_BOT_URL/code_gen
# Telegram      -> generate code             
# Telegram      -> send secret code             -> User
# User          -> paste secret code            -> WEB
# User          -> Click verify button on WEB 
# WEB           -> Send scode request           -> Telegram         Method: GET  URL: TELEGRAM_BOT_URL/auth_req
#                                                                   Response: { 'login' : ..., 'secret_code' : ... }
# WEB           -> check secret code hash vs. User secret code  
#
# Registration
# WEB       -> generate secret code
# User      -> paste secret code                    -> Telegram
# User      -> click verify button on WEB
# WEB       -> send generated secret code hash      -> Telegram   Method: GET URL: TELEGRAM_BOT_URL/reg, json={ 'login' : ..., 'secret_code' : ... }
# Telegram  -> verify secret codes (WEB vs User)                  Response: { 'status' : True / False }                                
#