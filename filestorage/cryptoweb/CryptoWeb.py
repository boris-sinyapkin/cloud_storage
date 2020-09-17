from Cryptodome.Random      import get_random_bytes
from Cryptodome.Cipher      import AES
from Cryptodome.Hash        import SHA256
from django.conf            import settings
from web_project.models     import FileTable, UserProfile
from pathlib                import Path

import psycopg2
import zipfile
import os.path
import os

cursor = None
conn = None

#Path to main storage dir
STORAGE_PATH = settings.FILE_STORAGE_PATH

def connect(dbname, user, password, host):
    global cursor, conn
    try:
        conn = psycopg2.connect(dbname=dbname, user=user, 
                            password=password, host=host)
        cursor = conn.cursor()
    except psycopg2.OperationalError:
        print('no connection')
        return False
    return True

def tmpprint(*args):
    return
tmpprint = print

def tree(uid):
    if not cursor: return None
    postgres_select_query = "SELECT _name, _type, _path FROM file_db "\
                            "WHERE file_db._uid = %s AND file_db._path like '/%%' "\
                            "ORDER BY _path"
    record_to_select = tuple([uid])
    cursor.execute(postgres_select_query, record_to_select)

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

def not_exists(uid, filename, ftype = None):
    if not cursor: return None
    fpath, fname = os.path.split(filename)
    postgres_select_query = "SELECT _uid FROM file_db "\
                            "WHERE file_db._uid = %s AND file_db._path = %s AND file_db._name = %s "
    if ftype:
        postgres_select_query += "AND file_db._type = %s"
        record_to_select = (uid, fpath, fname, ftype)
    else:
        record_to_select = (uid, fpath, fname)
    cursor.execute(postgres_select_query, record_to_select)
    tmpprint('not_exists:', uid, fpath, fname, ':', ftype)
    return not cursor.fetchone()

def uploaded(uid, filename): #(uid, fpath, fname)
    #filename = fpath + fname
    part_name = str(SHA256.new(bytes(str(uid) + filename, 'utf-8')).hexdigest())
    tmpprint('check uploaded:', uid, filename, ':')
    tmpprint('  [vv]', os.path.exists(STORAGE_PATH + str(uid) + '/' + part_name))
    return os.path.exists(STORAGE_PATH + str(uid) + '/' + part_name)

def mkroot(uid):

    os.makedirs(os.path.join(STORAGE_PATH, str(uid)), exist_ok=True)

    FileTable.objects.create( _uid=UserProfile.objects.get(id=uid), \
                              _name='/', \
                              _type='d', \
                              _path='')

    return True

def mkdir(uid, dirpath):
    if not cursor: return None
    dpath, dname = os.path.split(dirpath)

    if dpath == '' or dname == '':
        return None
    if (dpath != '/'):
        parpath, parname = os.path.split(dpath)
        if not_exists(uid, dpath, 'd'):
            return False

    if (not_exists(uid, dirpath)):
        postgres_insert_query = "INSERT INTO file_db (_uid, _name, _type, _path) VALUES (%s,%s,%s,%s)"
        record_to_insert = (uid, dname, 'd', dpath)
        cursor.execute(postgres_insert_query, record_to_insert)
        conn.commit()
        return True
    return False
    pass

def upload(uid, key, filename, fdata): #(uid, key, fpath, fname)
    if not cursor: return None
    
    #filename = fpath + fname

    tmpprint('storing:', uid, '/', filename, ':')

    iv = b'\xb7\xf8\xce\x15\x49\x24\x2b\xa1\xba\x9b\xc8\x67\x15\xc5\x37\x98'
    aes = AES.new(key, AES.MODE_EAX, iv)
    
    first_part_index = SHA256.new(bytes(str(uid) + filename, 'utf-8')).digest()
    part_name = first_part_index.hex()

    #already exists
    if (os.path.exists(STORAGE_PATH + str(uid) + '/' + part_name)):
        tmpprint(' ', filename, 'Already uploaded')
        return False

    size = len(fdata)
    part_count = int(pow(size//1024 + 1, 0.5))
    BYTES_IN_PART = size // part_count

    for i in range(0, part_count):
        tmpprint('  [->]:', part_name)

        data, fdata = fdata[0:BYTES_IN_PART], fdata[BYTES_IN_PART:]
        part_file = open(STORAGE_PATH + str(uid) + '/' + part_name, 'wb')

        if (i < part_count - 1):
            part_index = get_random_bytes(32)
            part_name = part_index.hex()
            while (os.path.exists(STORAGE_PATH + str(uid) + '/' + part_name)):
                part_index = get_random_bytes(32)
                part_name = part_index.hex()
        else:
            part_index = first_part_index

        data += part_index
        data = aes.encrypt(data)

        part_file.write(data)
        part_file.close()

    postgres_insert_query = "INSERT INTO file_db (_uid, _name, _type, _path) VALUES (%s,%s,%s,%s)"
    fpath, fname = os.path.split(filename) 
    record_to_insert = (uid, fname, 'f', fpath)
    cursor.execute(postgres_insert_query, record_to_insert)
    conn.commit()
    return True

def download(uid, key, filename) -> bytes:
    tmpprint('downloading:', str(uid), '/', filename, ':')

    iv = b'\xb7\xf8\xce\x15\x49\x24\x2b\xa1\xba\x9b\xc8\x67\x15\xc5\x37\x98'
    aes = AES.new(key, AES.MODE_EAX, iv)

    first_part_index = SHA256.new(bytes(str(uid) + filename, 'utf-8')).digest()
    part_name = first_part_index.hex()

    if (not os.path.exists(STORAGE_PATH + str(uid) + '/' + part_name)):
        tmpprint(' ', str(uid) + filename, 'No such file')
        return None

    fdata = b''

    while(True):
        tmpprint('  [<-]:', part_name)

        try:
            part_file = open(STORAGE_PATH + str(uid) + '/' + part_name, 'rb')
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

def remove(uid, key, filename):
    if not cursor: return None
    tmpprint('rmoving:', str(uid), '/', filename, ':')

    iv = b'\xb7\xf8\xce\x15\x49\x24\x2b\xa1\xba\x9b\xc8\x67\x15\xc5\x37\x98'
    aes = AES.new(key, AES.MODE_EAX, iv)

    first_part_index = SHA256.new(bytes(str(uid) + filename, 'utf-8')).digest()
    part_name = first_part_index.hex()

    if (not os.path.exists(STORAGE_PATH + str(uid) + '/' + part_name)):
        tmpprint(' ', str(uid) + filename, 'No such file')
        return False
    BLOCK_SIZE = os.path.getsize(STORAGE_PATH + str(uid) + '/' + part_name)

    while True:
        tmpprint('  [XX]:', part_name)

        part_file = open(STORAGE_PATH + str(uid) + '/' + part_name, 'rb')
        data = part_file.read()
        part_file.close()
        os.remove(STORAGE_PATH + str(uid) + '/' + part_name)

        part_index = aes.decrypt(data)[-32:]
        if (part_index == first_part_index):
            break
        part_name = part_index.hex()

    postgres_insert_query = "DELETE FROM file_db "\
                            "WHERE file_db._uid = %s AND file_db._name = %s AND file_db._path = %s"
    fpath, fname = os.path.split(filename) 
    record_to_insert = (uid, fname, fpath)
    cursor.execute(postgres_insert_query, record_to_insert)
    conn.commit()
    return True

def download_folder(uid, key, path, name):
    if not cursor: return None
    postgres_select_query = "SELECT _path, _name, _type FROM file_db WHERE file_db._path like CONCAT(%s, \'%%\') AND file_db._uid = %s"
    record_to_select = (path + name + '/', uid)
    cursor.execute(postgres_select_query, record_to_select)
    tree = cursor.fetchall()
    print('download_folder', tree)
    
    main_folder = '---loaded---'
    for sub in tree:
        if not os.path.exists(main_folder + sub[0]):
            os.makedirs(main_folder + sub[0])
        if (sub[2] == 'f'):
            if not download(uid, key, sub[0], sub[1]):
                return False
        elif not os.path.exists(main_folder + sub[0] + sub[1]):
            os.mkdir(main_folder + sub[0] + sub[1])
    
    if os.path.exists(main_folder + '.zip'):
        os.remove(main_folder + '.zip')

    arch = zipfile.ZipFile(main_folder + '.zip', 'w', zipfile.ZIP_DEFLATED)
    tmpprint('download_folder:', str(uid), '/', path + name, ':')

    tmpprint('  [<-]:', main_folder + '.zip')
    for root, dirs, files in os.walk(main_folder):
        print(root, dirs, files)
        for tarfile in dirs + files:
            if tarfile != '':
                arch.write(root + '/' + tarfile)
    arch.close()
    return True


def remove_folder(uid, key, path, name):
    if not cursor: return None
    postgres_select_query = "SELECT _path, _name, _type FROM file_db WHERE file_db._path like CONCAT(%s, \'%%\') AND file_db._uid = %s"
    record_to_select = (path + name + '/', uid)
    cursor.execute(postgres_select_query, record_to_select)
    tree = cursor.fetchall()
    print('remove_folder', tree)

    for sub in tree:
        if (sub[2] == 'd'):
            postgres_insert_query = "DELETE FROM file_db WHERE file_db._name = %s AND file_db._path = %s AND file_db._uid = %s"
            record_to_insert = (sub[1], sub[0], uid)
            cursor.execute(postgres_insert_query, record_to_insert)
            conn.commit()
        else :
            remove(uid, key, sub[0], sub[1])
    
    postgres_insert_query = "DELETE FROM file_db WHERE file_db._name = %s AND file_db._path = %s AND file_db._uid = %s"
    record_to_insert = (name, path, uid)
    cursor.execute(postgres_insert_query, record_to_insert)
    conn.commit()
    return True