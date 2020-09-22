import json
import httpx
import os
import filestorage.cryptoweb.CryptoWeb as CryptoWeb

from django.conf                    import settings
from django.contrib.auth.decorators import login_required
from django.contrib.auth.hashers    import make_password
from django.views.decorators.csrf   import csrf_exempt
from django.contrib                 import auth
from django.http                    import HttpResponse, HttpResponseBadRequest, HttpResponseForbidden, JsonResponse, StreamingHttpResponse
from io                             import BytesIO

from web_project.models             import UserProfile
from tg.views                       import REG_REQUEST_URL, GENCODE_REQUEST_URL, clear_session_item, int_to_hashstr
from filestorage.views              import normalize_path, get_user_key

def httpbody_as_json(request) -> dict:
    return json.loads(request.body.decode('utf-8')) if request.body else None

def find_user_in_database(login):
    try:
        return UserProfile.objects.get(username=login)
    except:
        return None

@csrf_exempt
def AndroidLogin(request):
    if request.method == 'POST':
        payload = httpbody_as_json(request)
        try:
            login, password = payload['login'], payload['pass']
        except:
            return HttpResponseBadRequest()
        
        user = find_user_in_database(login)
        if user and user.check_password(password) == True:
            request.session['android-sign-in'] = user.username
            return JsonResponse(status=200, data={ 'status' : 'ok'} )
        else:
            return HttpResponseForbidden(f"User {login} is not found or password is incorrect")

    return HttpResponseBadRequest()

@csrf_exempt
def AndroidVerify(request):
    if request.method == 'POST':
        payload = httpbody_as_json(request)
        try:
            login, secret_code  = payload['login'], payload['secret_code']
        except Exception as e:
            return HttpResponseBadRequest(e)

        user = find_user_in_database(login)
        if user and request.session.get('android-sign-in') == login:
            try:
                response = httpx.get(settings.TELEGRAM_BOT_URL + f"/auth_req?login={user.username}")
                scode    = json.loads(response.content.decode()).get('secret_code')

                if response.status_code == httpx.codes.OK and scode == int_to_hashstr(int(secret_code)):
                    auth.login(request, user)
                    clear_session_item(request, 'android-sign-in')
                    return JsonResponse(status=200, data={ 'status' : 'ok'} )
                else:
                    return HttpResponseForbidden("Telegram secret code verification failure")
            except Exception as e:
                return HttpResponseBadRequest(e)
    return HttpResponseBadRequest()

def AndroidSendCodeRequest(request):
    if request.method == 'GET' and request.session.get('android-sign-in') == request.GET.get('login'):
        try:
            httpx.get(settings.TELEGRAM_BOT_URL + GENCODE_REQUEST_URL, params={ 'login' : str(request.GET.get('login')) })
        except Exception:
            return HttpResponseBadRequest()
        else:
            return HttpResponse(status=200)

    return HttpResponseBadRequest()  

@login_required
@csrf_exempt
def AndroidFilelist(request):
    if request.method == 'POST':
        payload = httpbody_as_json(request)
        try:
            path  = normalize_path(payload['path'])
            login = request.user.username
        except Exception as e:
            return HttpResponseBadRequest(e)

        user = find_user_in_database(login)
        if user and user.is_authenticated:
            return JsonResponse(status=200, data=CryptoWeb.ls(user.id, path))
        else:
            return HttpResponseForbidden()
    return HttpResponseBadRequest()

@csrf_exempt
@login_required
def AndroidRemove(request):
    if request.method == 'POST':
        payload = httpbody_as_json(request)
        try:
            path  = normalize_path(payload['path'])
            login = request.user.username
        except Exception as e:
            return HttpResponseBadRequest(e)

        user = find_user_in_database(login)
        if user and user.is_authenticated:
            return AndroidFileHandler(user.id, path, "remove")
        else:
            return HttpResponseForbidden()
    return HttpResponseBadRequest()

@csrf_exempt
@login_required
def AndroidDownload(request):
    if request.method == 'POST':
        # payload = httpbody_as_json(request)
        try:
            # path  = normalize_path(payload['path'])
            login = request.user.username
        except Exception as e:
            return HttpResponseBadRequest(e)

        user = find_user_in_database(login)
        if user and user.is_authenticated:
            return AndroidFileHandler(user, normalize_path(request.GET['path']), "download")
        else:
            return HttpResponseForbidden()

    return HttpResponseBadRequest()

@csrf_exempt
@login_required
def AndroidUpload(request):
    if request.method == 'POST':  
        user = find_user_in_database(request.user.username)
        if user and request.FILES.get('uploaded_file') and request.GET.get('path'):
            upd_file  = request.FILES['uploaded_file']
            path      = request.GET.get('path')
            file_path = normalize_path( path + '/' + upd_file.name )
            CryptoWeb.upload(user.id, get_user_key(user), file_path, upd_file.read())
            return JsonResponse(status=200, data={ 'status' : 'ok'} )
        else:
            return HttpResponseForbidden()
    return HttpResponseBadRequest()

@login_required
def AndroidMkdir(request):
    return HttpResponseBadRequest()

@login_required
def AndroidLogout(request):
    auth.logout(request)
    return JsonResponse(status=200, data={ 'status' : 'ok'} )

def AndroidFileHandler(user : UserProfile, path, method):
    if method == "download":
        bdata    = CryptoWeb.download(user.id, get_user_key(user), path)
        basename = os.path.basename(path)
        basename = basename if CryptoWeb.what_is_it(user.id, path) == 'f' else f"{basename}.zip"
        if bdata is not None:
            response = StreamingHttpResponse(BytesIO(bdata))
            response['content_type'] = "application/octet-stream"
            response['Content-Disposition'] = 'attachment; filename=' + basename
            response['X-Sendfile'] = path
            return response

    elif method == "mkdir":
        pass

    elif method == "remove":
        CryptoWeb.remove(user.id, get_user_key(user), path)
        return JsonResponse(status=200, data={ 'status' : 'ok' } )

    return HttpResponseBadRequest()