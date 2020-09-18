from django.shortcuts       import render, redirect
from django.views.generic   import FormView, TemplateView
from django.conf            import settings
from django.http            import HttpResponse, HttpResponseForbidden, HttpResponseRedirect
from web_project.models     import UserProfile, FileTable
from django.utils.text      import slugify
from itertools              import dropwhile

import httpx
import json

from filestorage.cryptoweb import CryptoWeb

def get_user_object(request):
    try:
        if request.user:
            return UserProfile.objects.get(username='sinyap')
    except:
        return None


def normalize_dirname(path):
    return '/' + slugify(path)

def normalize_path(path):
    return '/' + '/'.join([ p for p in path.split('/') if p != '' ])

def ShowFilesHandler(request, path : str):
    user = get_user_object(request)
    if user is None:
        return HttpResponseForbidden()
    else:
        return render(request, 'fs/home.html', { 'files' : CryptoWeb.ls(user.id, normalize_path(path)) })

def FileHandler(request, method):
    user = get_user_object(request)
    if user is None:
        return HttpResponseForbidden()
    else:
        if request.method == 'POST':
            if method == "upload" and request.FILES.get('myfile'):
                key = b'\xb7\xf8\xce\x15\x49\x24\x2b\xa1\xba\x9b\xc8\x67\x15\xc5\x37\x98'
                myfile = request.FILES['myfile']
                CryptoWeb.upload(user.id, key, '/' + myfile.name, myfile.read())
            
            elif method == "mkdir" and request.POST.get('new_dir_name'):

                current_dir = request.POST.get('cur_dir').replace('/filestorage', '')
                new_dir     = normalize_dirname(request.POST.get('new_dir_name'))
                CryptoWeb.mkdir(user.id, normalize_path(current_dir + new_dir))

    return HttpResponseRedirect(request.META.get('HTTP_REFERER'))