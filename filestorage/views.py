from django.shortcuts                  import render, redirect
from django.views.generic              import FormView, TemplateView
from django.conf                       import settings
from django.http                       import HttpResponse, HttpResponseForbidden, HttpResponseRedirect, StreamingHttpResponse, HttpResponseBadRequest
from django.contrib.auth.decorators    import login_required

from web_project.models     import UserProfile
from django.utils.text      import slugify
from itertools              import dropwhile
from io                     import BytesIO

import httpx
import json
import os

from filestorage.cryptoweb import CryptoWeb

def get_user_key(user : UserProfile) -> bytes:
    return str.encode(user.password)[-16:]

def get_user_object(request):
    try:
        if request.user:
            return UserProfile.objects.get(username=request.user.username)
    except:
        return None

def normalize_dirname(path):
    return '/' + slugify(path)

def normalize_path(path):
    path = path.replace('/filestorage', '')
    return '/' + '/'.join([ p for p in path.split('/') if p != '' ])

@login_required
def ShowFilesHandler(request, path : str):
    user = get_user_object(request)
    if user is None:
        return HttpResponseForbidden()
    else:
        if request.method == 'GET':
            norm_path = normalize_path(path)
            ftype     = CryptoWeb.what_is_it(user.id, norm_path)
            if ftype == 'f':
                return FileHandler(request, 'download')
            else:
                warning = None
                if norm_path != '/' and CryptoWeb.not_exists(user.id, norm_path, ftype='d'):
                    warning = [f"Directory {norm_path} does not exist"]

                return render(request, 'fs/home.html', { 
                    'files'     : CryptoWeb.ls(user.id, norm_path), 
                    'cur_dir'   : norm_path,
                    'messages'  : warning})
    
    return HttpResponseRedirect(request.META.get('HTTP_REFERER'))

@login_required
def FileHandler(request, method):
    user = get_user_object(request)
    if user is None:
        return HttpResponseForbidden()
    else:

        if request.method == 'POST':
            if method == "upload":
                if request.FILES.get('myfile') is not None:
                    myfile    = request.FILES['myfile']
                    file_path = normalize_path( request.POST.get('cur_dir') + '/' + myfile.name )
                    CryptoWeb.upload(user.id, get_user_key(user), file_path, myfile.read())
                else:
                    pass
            elif method == "mkdir" and request.POST.get('new_dir_name'):

                new_dir = request.POST.get('cur_dir') + request.POST.get('new_dir_name')
                CryptoWeb.mkdir(user.id, normalize_path(new_dir))

            elif method == "remove" and request.POST.get('filepath') :
                
                CryptoWeb.remove(user.id, 
                                get_user_key(user), 
                                normalize_path(request.POST.get('filepath')))
            elif not method == 'download':
                return HttpResponseBadRequest()

        if method == "download":  
            key = get_user_key(user)
            if request.method == 'POST' and request.POST.get('filepath'):
                path  = normalize_path(request.POST.get('filepath'))
            elif request.method == 'GET':
                path  = normalize_path(request.path)
            else:
                return HttpResponseBadRequest()

            bdata    = CryptoWeb.download(user.id, key, path)
            basename = os.path.basename(path)
            basename = basename if CryptoWeb.what_is_it(user.id, path) == 'f' else f"{basename}.zip"
            if bdata is not None:
                response = StreamingHttpResponse(BytesIO(bdata))
                response['content_type'] = "application/octet-stream"
                response['Content-Disposition'] = 'attachment; filename=' + basename
                response['X-Sendfile'] = path
                return response
                    
    return HttpResponseRedirect(request.META.get('HTTP_REFERER'))