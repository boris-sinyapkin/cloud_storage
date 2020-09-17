from django.shortcuts       import render
from django.views.generic   import FormView, TemplateView
from django.conf            import settings
from django.http            import HttpResponse

import httpx
import json

import filestorage.cryptoweb.CryptoWeb
    
def show_files(request, path : str):
    path = f"/{path.strip('/')}"
    print(path)
    return render(request, 'fs/home.html', { 'cur_dir' : 'path', 'files' : ['payload'] })