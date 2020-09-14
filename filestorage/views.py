from django.shortcuts       import render
from django.views.generic   import FormView, TemplateView

# Create your views here.
class FsHomeView(TemplateView):
    template_name = 'fs/home.html'

