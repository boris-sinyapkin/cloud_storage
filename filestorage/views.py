from django.shortcuts       import render, redirect
from django.views.generic   import FormView, TemplateView
from django.conf            import settings
from django.http            import HttpResponse, HttpResponseForbidden, HttpResponseRedirect, StreamingHttpResponse
from web_project.models     import UserProfile, FileTable
from django.utils.text      import slugify
from itertools              import dropwhile
from io                     import BytesIO


import httpx
import json
import os

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
    path = path.replace('/filestorage', '')
    return '/' + '/'.join([ p for p in path.split('/') if p != '' ])

def ShowFilesHandler(request, path : str):
    user = get_user_object(request)
    if user is None:
        return HttpResponseForbidden()
    else:
        norm_path = normalize_path(path)
        if CryptoWeb.what_is_it(user.id, norm_path) == 'f':
            return FileHandler(request, 'download')
        else:
            return render(request, 'fs/home.html', { 'files' : CryptoWeb.ls(user.id, norm_path) })

def FileHandler(request, method):
    user = get_user_object(request)
    if user is None:
        return HttpResponseForbidden()
    else:
        if request.method == 'POST':
            if method == "upload" and request.FILES.get('myfile'):

                key       = b'\xb7\xf8\xce\x15\x49\x24\x2b\xa1\xba\x9b\xc8\x67\x15\xc5\x37\x98'
                myfile    = request.FILES['myfile']
                file_path = normalize_path( request.POST.get('cur_dir') + '/' + myfile.name )

                CryptoWeb.upload(user.id, key, file_path, myfile.read())
            
            elif method == "mkdir" and request.POST.get('new_dir_name'):

                new_dir = request.POST.get('cur_dir') + request.POST.get('new_dir_name')
                CryptoWeb.mkdir(user.id, normalize_path(new_dir))

            elif method == "remove":
                pass

        elif request.method == 'GET':
            if method == "download":
                key   = b'\xb7\xf8\xce\x15\x49\x24\x2b\xa1\xba\x9b\xc8\x67\x15\xc5\x37\x98'
                path  = normalize_path(request.path)
                bdata = CryptoWeb.download(user.id, key, path)

                if bdata is not None:
                    response = StreamingHttpResponse(BytesIO(bdata))
                    response['content_type'] = "application/octet-stream"
                    response['Content-Disposition'] = 'attachment; filename=' + os.path.basename(path) 
                    response['X-Sendfile'] = path
                    return response
                    
            elif method == "remove":
                pass 


    return HttpResponseRedirect(request.META.get('HTTP_REFERER'))