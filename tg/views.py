import random
import httpx
import json

from hashlib                      import sha256
from django.conf                  import settings
from .models                      import UserProfile
from django.http                  import HttpResponse, HttpResponseRedirect, HttpRequest, HttpResponseForbidden, HttpResponseBadRequest
from django.views.decorators.csrf import csrf_exempt
from django.views.generic         import FormView, TemplateView
from django                       import forms
from django.shortcuts             import redirect, resolve_url, render

def int_to_bytes(x: int) -> bytes:
    return x.to_bytes((x.bit_length() + 7) // 8, 'big')

def int_from_bytes(xbytes: bytes) -> int:
    return int.from_bytes(xbytes, 'big')

def int_to_hashstr(x : int) -> str:
    return sha256(int_to_bytes(x)).hexdigest()

AUTH_REQUEST_URL    = '/auth_req'
REG_REQUEST_URL     = '/reg'
GENCODE_REQUEST_URL = '/code_gen'

def HttpResponseForbiddenTg():
    return HttpResponseForbidden("You are not authorized on the server. \
                Firstly use your credentials to sign-in on the home page and then you \
                    will automatically forward to Telegram Authentication")
    

def verify_scode(username, scode : int) -> bool:
    response = httpx.get(settings.TELEGRAM_BOT_URL + REG_REQUEST_URL, 
                  params={  'login'     : str(username), 
                          'secret_code' : int_to_hashstr(scode) })

    return json.loads(response.content.decode()).get('status')

def is_telegram_authenticated(user) -> bool:
    try:
        if user.is_authenticated == True and user.is_telegram_auth == True:
            return True
        else:
            return False
    except AttributeError:
        return False

def try_to_get_current_user(request):
    try:
        return UserProfile.objects.get(username=request.user.username)
    except:
        return None
    
@csrf_exempt
def TgRequestHandler(request : HttpRequest, op : str):

    user = try_to_get_current_user(request)

    if user is None:
        return HttpResponseForbiddenTg()

    if request.method == 'GET':
        status_value = request.GET.get('value')
        # Verify, if redirection is needed
        if status_value == 'verify':
            if verify_scode(user.username, int(request.GET['scode'])) == True:
                return redirect('login')
            else:
                return HttpResponseForbidden("Telegram Registration Failure")
            
    elif request.method == "POST":
        pass

    return HttpResponseBadRequest()
        
# Create your views here.
class TgRegistrationView(TemplateView):
    template_name = 'telegram/tg_reg.html'
        
    def get_context_data(self, **kwargs):
        context = super().get_context_data(**kwargs)
        scode = random.getrandbits(16)
        context['scode']      = scode
        context['verify_url'] = f"status/?value=verify&scode={scode}"
        return context

class TgAuthForm(forms.Form): 
    secret_code = forms.CharField(max_length = 200) 
    def save(self):
        return int(self.cleaned_data['secret_code'])

class TgAuthView(FormView):
    template_name = 'telegram/tg_auth.html'
    form_class    = TgAuthForm

    def form_valid(self, form):
        user = try_to_get_current_user(self.request)

        if user is None:
            return HttpResponseForbiddenTg()
        
        # Request to Telegram Bot
        auth_response = httpx.get(settings.TELEGRAM_BOT_URL + "/auth_req")

        if int_to_hashstr(form.save()) == json.loads(auth_response.content.decode()).get('secret_code'):
            # Set up Telegram auth flag nad redirect to home page
            user.is_telegram_auth = True
            user.save()
        else:
            return HttpResponseForbidden("Telegram authentication failure")

        return redirect('home')

def TgSecretCodeRequest(request):
    user = try_to_get_current_user(request)

    if user is None:
        return HttpResponseForbiddenTg()

    response = httpx.get(settings.TELEGRAM_BOT_URL + GENCODE_REQUEST_URL, 
                            params={ 'login' : str(user.username) })
    return \
        HttpResponseRedirect(request.META.get('HTTP_REFERER')) \
            if response.status_code == httpx.codes.OK \
                else HttpResponseBadRequest()
