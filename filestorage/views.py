from django.shortcuts       import render
from django.views.generic   import FormView, TemplateView
from django.conf            import settings
from django.http            import HttpResponse

import httpx
import json

FLIST_URL = '/list'

def request_filelist(dir : str) -> list:
    response = httpx.get(settings.FILESERVER_URL, params={ 'list' : dir})
    
    if response.status_code == httpx.codes.OK:
        payload = json.loads(response.content.decode())['dir']
        print(payload)

    return None
    
def show_files(request, path : str):
    response = httpx.request('POST', settings.FILESERVER_URL + '/list', json={ 'path' : path })
    
    if response.status_code == httpx.codes.OK:
        payload = json.loads(response.content.decode())['content']

    return render(request, 'fs/home.html', { 'cur_dir' : path, 'files' : payload })