import random
import httpx
import json

from hashlib                      import sha256, md5
from django.conf                  import settings
from web_project.models           import UserProfile
from django.http                  import HttpResponse, HttpResponseRedirect, HttpRequest, HttpResponseForbidden, HttpResponseBadRequest
from django.views.decorators.csrf import csrf_exempt
from django.views.generic         import FormView, TemplateView
from django                       import forms
from django.shortcuts             import redirect, resolve_url, render
from django.contrib.auth          import authenticate, login
from collections                  import namedtuple

from filestorage.cryptoweb        import CryptoWeb

def int_to_bytes(x: int) -> bytes:
    return x.to_bytes((x.bit_length() + 7) // 8, 'big')

def int_from_bytes(xbytes: bytes) -> int:
    return int.from_bytes(xbytes, 'big')

def int_to_hashstr(x : int) -> str:
    return sha256(int_to_bytes(x)).hexdigest()

AUTH_REQUEST_URL    = '/auth_req'
REG_REQUEST_URL     = '/reg'
GENCODE_REQUEST_URL = '/code_gen'

def clear_session_item(request, name : str):
    try:
        del request.session[name]
    except KeyError:
        pass

def HttpResponseForbiddenTg():
    return HttpResponseForbidden("You are not authorized on the server. \
                Firstly use your credentials to sign-in on the home page and then you \
                    will automatically forward to Telegram Authentication")
    

def verify_scode(username, scode : int) -> bool:
    response = httpx.get(settings.TELEGRAM_BOT_URL + REG_REQUEST_URL, 
                  params={  'login'     : str(username), 
                          'secret_code' : int_to_hashstr(scode) })

    return json.loads(response.content.decode()).get('status')

def try_to_get_current_user(request):
    user_cookie = request.session.get('user') 
    if user_cookie is not None:
        user_dict  = json.loads(user_cookie)
        UserCookie = namedtuple('UserCookie', 'username password')
        return UserCookie(user_dict['username'], user_dict['password'])
    else:
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

                # Create user object
                new_user = UserProfile.objects.create( username=user.username, password=user.password, is_superuser=False )

                # Create root directory
                CryptoWeb.mkroot(new_user.id)

                # Delete session item and redirect to login page
                http_response = redirect('login')

                clear_session_item(request, 'user')
                return http_response
            else:
                return render(request, TgRegistrationView.template_name, { 'messages'  : ['Telegram Registration Failure. Invalid secret code.'] } )
            
    elif request.method == "POST":
        pass

    return HttpResponseBadRequest()
        
# Create your views here.
class TgRegistrationView(TemplateView):
    template_name = 'telegram/tg_reg.html'

    def get(self, request, *args, **kwargs):
        if request.session.get('user') is not None:
            context = self.get_context_data(**kwargs)
            response = self.render_to_response(context)
        else:
            response = HttpResponseForbidden()
        return response
        
    def get_context_data(self, **kwargs):
        context = super().get_context_data(**kwargs)
        scode = random.getrandbits(16)
        context['scode']      = scode
        context['verify_url'] = f"status/?value=verify&scode={scode}"
        return context

class TgAuthForm(forms.Form): 
    secret_code = forms.CharField(max_length = 200) 
    def save(self):
        try:
            return int(self.cleaned_data['secret_code'])
        except ValueError:
            return None

class TgAuthView(FormView):
    template_name = 'telegram/tg_auth.html'
    form_class    = TgAuthForm

    def get(self, request, *args, **kwargs):
        if request.session.get('signup-login') is not None:
            context = self.get_context_data(**kwargs)
            response = self.render_to_response(context)
        else:
            response = HttpResponseForbidden()
        return response

    def form_valid(self, form):
        user       = UserProfile.objects.get(username=self.request.session.get('signup-login'))
        form_scode = form.save()

        if user is None:
            return HttpResponseForbiddenTg()

        if form_scode is None:
            return render(self.request, TgAuthView.template_name, { 
                'form' : TgAuthView.form_class, 
                'messages'  : ['Invalid secret code'] } )

        # Request to Telegram Bot
        try:
            auth_response = httpx.get(settings.TELEGRAM_BOT_URL + f"/auth_req?login={user.username}")
        except Exception:
            return render(self.request, TgAuthView.template_name, { 
                'form' : TgAuthView.form_class, 
                'messages'  : ['Telegram Bot request timeout'] } )

        if auth_response.status_code == httpx.codes.OK and \
            int_to_hashstr(form_scode) == json.loads(auth_response.content.decode()).get('secret_code'):

            # Sign in 
            login(self.request, user)

            # Redirect to homepage
            http_response = redirect('home')
      
            clear_session_item(self.request, 'signup-login')

        else:
            http_response = render(self.request, TgAuthView.template_name, { 
                'form' : TgAuthView.form_class, 
                'messages'  : ['Telegram Authorization Failure. Invalid secret code.'] } )
        
        return http_response

def TgSecretCodeRequest(request):    
    username = request.session.get('signup-login')

    if username is None:
        return HttpResponseForbiddenTg()

    try:
        response = httpx.get(settings.TELEGRAM_BOT_URL + GENCODE_REQUEST_URL, params={ 'login' : str(username) })
    except Exception:
        return render(request, TgAuthView.template_name, { 'form' : TgAuthView.form_class, 'messages'  : ['Telegram Bot request timeout'] } )

    return \
        HttpResponseRedirect(request.META.get('HTTP_REFERER')) \
            if response.status_code == httpx.codes.OK \
                else HttpResponseBadRequest()
