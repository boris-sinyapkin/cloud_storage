from django.conf                  import settings
from django.http                  import HttpResponse, HttpResponseRedirect
from django.views.decorators.csrf import csrf_exempt
from django.views.generic         import FormView, TemplateView
from django.contrib.auth.forms    import UserCreationForm
from django.contrib.auth          import logout
from django.shortcuts             import redirect, resolve_url
from tg.models                    import UserProfile

class RegistrationView(FormView):
    template_name = 'registration.html'
    form_class    = UserCreationForm

    def form_valid(self, form):
        form.save()
        return redirect('registration_complete')

class RegistrationCompleteView(TemplateView):
    template_name = 'registration_complete.html'

    def get_context_data(self, **kwargs):
        context = super().get_context_data(**kwargs)
        context['tg_reg'] = resolve_url('/telegram/reg/')
        return context

class HomePageView(TemplateView):
    template_name = 'home.html'

def LogoutHandler(request):
    if request.user.is_authenticated:
        user = UserProfile.objects.get(username=request.user.username)
        user.is_telegram_auth = False
        user.save()
        logout(request)

    return HttpResponseRedirect(resolve_url('home'))