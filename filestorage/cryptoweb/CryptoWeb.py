from Cryptodome.Random      import get_random_bytes
from Cryptodome.Cipher      import AES
from Cryptodome.Hash        import SHA256

from django.conf            import settings
from web_project.models     import FileTable, UserProfile
from pathlib                import Path
from django.db              import connection

import psycopg2
import zipfile
import os.path
import os
import shutil

cursor = None
conn = None

#Path to main storage dir
STORAGE_PATH = settings.FILE_STORAGE_PATH

def tmpprint(*args):
    return
tmpprint = print

def tree(uid):
    try:
        cursor = connection.cursor()

        cursor.execute( "SELECT _name, _type, _path FROM web_project_filetable " \
                        "WHERE web_project_filetable._uid_id = %s AND web_project_filetable._path like '/%%' " \
                        "ORDER BY _path",  tuple([uid]))
    except:
        return None

    result_tree = { '/': {} }

    for (fname, ftype, fpath) in cursor:
        fpath, tmp_dir = os.path.split(fpath)
        path_list = []
        while tmp_dir:
            path_list = [tmp_dir] + path_list
            fpath, tmp_dir = os.path.split(fpath)
        path_list = [fpath] + path_list

        result_sub_tree = result_tree
        for tmp_dir in path_list:
            result_sub_tree = result_sub_tree[tmp_dir]
        
        if ftype == 'd':
            result_sub_tree[fname] = {}
        else:
            result_sub_tree[fname] = 'file'
    return result_tree

def what_is_it(uid, path):
    fpath, fname = os.path.split(path)
    try:
        file_obj = FileTable.objects.get( 
            _uid=UserProfile.objects.get(id=uid), 
            _name=fname, 
            _path=fpath)
    except:
        return None

    return file_obj._type if file_obj is not None else None

def not_exists(uid, filename, ftype = None):
    fpath, fname = os.path.split(filename)
    postgres_select_query = "SELECT _uid_id FROM web_project_filetable "\
                            "WHERE web_project_filetable._uid_id = %s AND web_project_filetable._path = %s AND web_project_filetable._name = %s "
    if ftype:
        postgres_select_query += "AND web_project_filetable._type = %s"
        record_to_select = (uid, fpath, fname, ftype)
    else:
        record_to_select = (uid, fpath, fname)

    try:
        cursor = connection.cursor()
        cursor.execute(postgres_select_query, record_to_select)
    except:
        return None

    tmpprint('not_exists:', uid, fpath, fname, ':', ftype)
    return not cursor.fetchone()

def uploaded(uid, filename): #(uid, fpath, fname)
    #filename = fpath + fname
    part_name = str(SHA256.new(bytes(str(uid) + filename, 'utf-8')).hexdigest())
    serv_path = STORAGE_PATH / Path(str(uid)) / Path(part_name)
    tmpprint('check uploaded:', uid, filename, ':')
    tmpprint('  [vv]', os.path.exists(serv_path))
    return serv_path.exists()

def mkroot(uid):

    os.makedirs(os.path.join(STORAGE_PATH, str(uid)), exist_ok=True)

    FileTable.objects.create( _uid=UserProfile.objects.get(id=uid), \
                              _name='/', \
                              _type='d', \
                              _path='')

    return True

def mkdir(uid, dirpath):
    dpath, dname = os.path.split(dirpath)
    is_new = not_exists(uid, dirpath)
    if (is_new is True):
        FileTable.objects.create( _uid=UserProfile.objects.get(id=uid), \
                                  _name=dname, \
                                  _type='d', \
                                  _path=dpath)
        return True
    return False

def upload(uid, key, filename, fdata : bytes): #(uid, key, fpath, fname)

    iv = b'\xb7\xf8\xce\x15\x49\x24\x2b\xa1\xba\x9b\xc8\x67\x15\xc5\x37\x98'
    aes = AES.new(key, AES.MODE_EAX, iv)
    
    first_part_index = SHA256.new(bytes(str(uid) + filename, 'utf-8')).digest()
    part_name = first_part_index.hex()

    USER_ROOT_PATH = STORAGE_PATH / Path(str(uid))

    if (USER_ROOT_PATH / Path(part_name)).exists():
        tmpprint(' ', filename, 'Already uploaded')
        return False

    size = len(fdata)
    part_count = int(pow(size//1024 + 1, 0.5))
    BYTES_IN_PART = size // part_count

    for i in range(0, part_count):
        tmpprint('  [->]:', part_name)

        data, fdata = fdata[0:BYTES_IN_PART], fdata[BYTES_IN_PART:]
        part_file = (USER_ROOT_PATH / Path(part_name)).open('wb')

        if (i < part_count - 1):
            part_index = get_random_bytes(32)
            part_name = part_index.hex()
            while (USER_ROOT_PATH / Path(part_name)).exists():
                part_index = get_random_bytes(32)
                part_name = part_index.hex()
        else:
            part_index = first_part_index

        data += part_index
        data = aes.encrypt(data)

        part_file.write(data)
        part_file.close()

    fpath, fname = os.path.split(filename) 

    FileTable.objects.create( _uid=UserProfile.objects.get(id=uid), \
                              _name=fname, \
                              _type='f', \
                              _path=fpath)

    return True

def ls(uid, dirpath):
    try:
        cursor = connection.cursor()
        cursor.execute( "SELECT _name, _type, upd_time FROM web_project_filetable " \
                        "WHERE web_project_filetable._uid_id = %s AND web_project_filetable._path = %s " \
                        "ORDER BY _name", (uid, dirpath))
    except:
        return None

    return { fname: {'type' : ftype, 'baseline' : upd_time } for fname, ftype, upd_time in cursor }

def object_info(uid, filename):

    fpath, fname = os.path.split(filename)
    try:
        cursor = connection.cursor()
        cursor.execute( "SELECT _uid_id, _path, _name, _type FROM web_project_filetable "\
                        "WHERE web_project_filetable._uid_id = %s AND web_project_filetable._path = %s AND web_project_filetable._name = %s ", (uid, fpath, fname))
        rec = cursor.fetchone()
    except:
        return None

    if not rec:
        return None

    return { 'uid': rec[0], 'path': rec[1], 'name': rec[2], 'type': rec[3] }


def download_file(uid, key, filename) -> bytes:
    tmpprint('downloading:', str(uid), filename, ':')

    iv = b'\xb7\xf8\xce\x15\x49\x24\x2b\xa1\xba\x9b\xc8\x67\x15\xc5\x37\x98'
    aes = AES.new(key, AES.MODE_EAX, iv)

    first_part_index = SHA256.new(bytes(str(uid) + filename, 'utf-8')).digest()
    part_name = first_part_index.hex()

    USER_ROOT_PATH = STORAGE_PATH / Path(str(uid))

    if not (USER_ROOT_PATH / Path(part_name)).exists():
        tmpprint(' ', str(uid) + filename, 'No such file')
        return None

    fdata = b''

    while(True):
        try:
            part_file = (USER_ROOT_PATH / Path(part_name)).open('rb')
        except:
            tmpprint('  Incorrect Key')
            return None

        data = part_file.read()
        part_file.close()
    
        data = aes.decrypt(data)
        data, part_index = data[:-32], data[-32:]

        fdata += data
        if (part_index == first_part_index):
            break

        part_name = part_index.hex()

    return fdata

def download_folder(uid, key, dirpath):
    
    try:
        cursor = connection.cursor()
        cursor.execute( "SELECT _path, _name, _type FROM web_project_filetable "\
                        "WHERE web_project_filetable._path like CONCAT(%s, \'%%\') AND web_project_filetable._uid_id = %s "\
                        "ORDER BY _path", (dirpath, uid))
        rec = cursor.fetchall()
    except:
        return None

    if not rec:
        return None
    
    temp_folder = os.path.join(STORAGE_PATH, '~' + str(uid))
    dirpath, dirname = os.path.split(dirpath)
    load_folder = os.path.join(temp_folder, dirname)
    tmpprint(load_folder)
    os.makedirs(load_folder, exist_ok=True)

    if dirpath == '/':
        dirpath = ''

    path_offset = len(dirpath) if dirpath != '/' else 0

    for fpath, fname, ftype in rec:
        if ftype == 'd':
            dr = os.path.join(fpath[path_offset:], fname)
            tmpprint('  foldering:', uid, dr)
            os.makedirs(temp_folder + dr, exist_ok=True)
        else:
            fdata = download(uid, key, fpath + '/' + fname)
            if fdata:
                dr = os.path.join(fpath[path_offset:], fname)
                tmpf = open(temp_folder + dr, 'wb')
                tmpf.write(fdata)
                tmpf.close()
    
    shutil.make_archive(load_folder, 'zip', temp_folder, dirname)
    zip_file = open(load_folder + '.zip', 'rb')
    zip_data = zip_file.read()
    zip_file.close()
    shutil.rmtree(temp_folder, True)

    return zip_data

def download(uid, key, filename) -> bytes:
    finfo = object_info(uid, filename)
    if not finfo: 
        return None

    if finfo['type'] == 'f':
        return download_file(uid, key, filename)
    else:
        return download_folder(uid, key, filename)

def remove_file(uid, key, filename):
    tmpprint('rmoving:', str(uid), filename, ':')

    iv = b'\xb7\xf8\xce\x15\x49\x24\x2b\xa1\xba\x9b\xc8\x67\x15\xc5\x37\x98'
    aes = AES.new(key, AES.MODE_EAX, iv)

    first_part_index = SHA256.new(bytes(str(uid) + filename, 'utf-8')).digest()
    part_name = first_part_index.hex()

    USER_ROOT_PATH = STORAGE_PATH / Path(str(uid))

    if not (USER_ROOT_PATH / Path(part_name)).exists():
        tmpprint(' ', str(uid) + filename, 'No such file')
        return False

    while True:
        #tmpprint('  [XX]:', part_name)
        part_file = (USER_ROOT_PATH / Path(part_name)).open('rb')
        data = part_file.read()
        part_file.close()
        os.remove(USER_ROOT_PATH / Path(part_name))

        part_index = aes.decrypt(data)[-32:]
        if (part_index == first_part_index):
            break
        part_name = part_index.hex()


    fpath, fname = os.path.split(filename) 

    try:
        cursor = connection.cursor()
        cursor.execute( "DELETE FROM web_project_filetable "\
                        "WHERE web_project_filetable._uid_id = %s "\
                        "AND web_project_filetable._name = %s "\
                        "AND web_project_filetable._path = %s", (uid, fname, fpath))
    except:
        return None


    return True

def remove_folder(uid, key, dirname):

    try:
        cursor = connection.cursor()
        cursor.execute( "SELECT _path, _name, _type FROM web_project_filetable "\
                        "WHERE web_project_filetable._uid_id = %s "\
                        "AND web_project_filetable._path like CONCAT(%s, \'%%\') "\
                        "ORDER BY _path", (uid, dirname))
    except:
        return None

    for fpath, fname, ftype in cursor.fetchall():
        if (ftype == 'f'):
            remove_file(uid, key, fpath + '/' + fname)
        else: #remove directory
            tmpprint('rmoving:', uid, fpath + '/' + fname, ':d')
            try:
                cursor.execute( "DELETE FROM web_project_filetable "\
                                "WHERE web_project_filetable._uid_id = %s "\
                                "AND web_project_filetable._name = %s "\
                                "AND web_project_filetable._path = %s ", (uid, fname, fpath))
            except:
                return None

    fpath, fname = os.path.split(dirname)

    try:
        cursor = connection.cursor()
        cursor.execute( "DELETE FROM web_project_filetable "\
                        "WHERE web_project_filetable._uid_id = %s "\
                        "AND web_project_filetable._name = %s "\
                        "AND web_project_filetable._path = %s ", (uid, fname, fpath))
    except:
        return None
    
    return True

def remove(uid, key, filename) -> bytes:
    finfo = object_info(uid, filename)
    if not finfo: 
        tmpprint('remove(', uid, filename, '): no such object')
        return None

    if finfo['type'] == 'f':
        return remove_file(uid, key, filename)
    else:
        return remove_folder(uid, key, filename)