from django.conf                  import settings
from django.http                  import HttpResponse, HttpResponseRedirect, HttpResponseForbidden, HttpResponseBadRequest
from django.views.decorators.csrf import csrf_exempt
from django.views.generic         import FormView, TemplateView
from django.contrib.auth.forms    import UserCreationForm, AuthenticationForm
from django.contrib.auth          import logout, get_user_model, login
from django.shortcuts             import redirect, resolve_url, render
from .models                      import UserProfile

import httpx
import json

from tg.views import try_to_get_current_user

class RegistrationForm(UserCreationForm):
    class Meta:
        model = get_user_model()
        fields = ( 'username', )

class RegistrationView(FormView):
    template_name = 'registration.html'
    form_class    = RegistrationForm

    def form_valid(self, form):
        user    = form.save(commit=False)
        try:
            reponse = httpx.get(settings.TELEGRAM_BOT_URL + f"/new_login?login={user.username}")
        except Exception as e:
            return render(self.request, RegistrationView.template_name, { 'form' : RegistrationView.form_class, 'messages'  : ['Telegram Bot request timeout'] } )

        if reponse.status_code == httpx.codes.OK:
            response_redirect = HttpResponseRedirect('done/')
            response_redirect.set_cookie('user', json.dumps({ 'username' : user.username, 'password' : user.password }), max_age = 10 * 60)
            return response_redirect
        else:
            return HttpResponseBadRequest("Bad new login request to Telegram Bot")

class RegistrationCompleteView(TemplateView):
    template_name = 'registration_complete.html'

    def get(self, request, *args, **kwargs):
        if request.COOKIES.get('user') is not None:
            context  = self.get_context_data(**kwargs)
            response = self.render_to_response(context)
        else:
            response = HttpResponseForbidden()
        return response
        
    def get_context_data(self, **kwargs):
        context = super().get_context_data(**kwargs)
        context['tg_reg'] = resolve_url('/telegram/reg/')
        return context

class HomePageView(TemplateView):
    template_name = 'home.html'


class LoginForm(AuthenticationForm):
    pass

class LoginView(FormView):
    template_name = 'registration/login.html'
    form_class = LoginForm

    def get(self, request, *args, **kwargs):
        if not request.user.is_authenticated:
            context  = self.get_context_data(**kwargs)
            response = self.render_to_response(context)
        else:
            response = render(request, HomePageView.template_name, { 
                'messages' : [ f"You are already authorized as {request.user.username}"]
            })
        return response

    def form_valid(self, form):
        user = form.get_user()
        if user is None:
            return HttpResponseBadRequest()
        else:
            response = redirect('/telegram/auth')
            response.set_cookie('signup-login', user.username, max_age = 5 * 60)
            return response

def LogoutHandler(request):
    logout(request)
    return HttpResponseRedirect(resolve_url('home'))