import json
import httpx

from django.http            import HttpResponse, HttpResponseBadRequest, HttpResponseForbidden
from web_project.models     import UserProfile
from django.conf            import settings
from tg.views               import REG_REQUEST_URL

def httpbody_as_json(request) -> dict:
    return json.loads(request.body.decode('utf-8')) if request.body else None

# Login request handler
def AndroidLogin(request):
    if request.method == 'POST':
        payload = httpbody_as_json(request)
        try:
            login, sha256_pass = payload['login'], payload['pass']
        except:
            return HttpResponseBadRequest()
        
        user = UserProfile.objects.get(username=login, password=sha256_pass)
        if user is None:
            return HttpResponseForbidden()
        else:
            user.is_android_auth = True
            user.save()
            return HttpResponse(status=200)

    return HttpResponseBadRequest()

def AndroidVerify(request):
    pass
    # if request.method == 'POST':
    #     payload = httpbody_as_json(request)
    #     try:
    #         login, secret_code  = payload['login'], payload['secret_code']
    #     except:
    #         return HttpResponseBadRequest()
            
    #     response = httpx.get(settings.TELEGRAM_BOT_URL + REG_REQUEST_URL, params={ 'login' : str(login), 'secret_code' : secret_code })

    #     if json.loads(response.content.decode()).get('status') == True:
    #         login(self.request, user)
    #         return HttpResponse(status=200)
    #     else:
    #         return HttpResponseForbidden()

def AndroidFilelist(request):
    pass

def AndroidRemove(request):
    pass

def AndroidDownload(request):
    pass

def AndroidUpload(request):
    pass

def AndroidMkdir(request):
    pass