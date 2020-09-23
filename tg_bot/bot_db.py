import psycopg2
from hashlib import sha256

def int_to_bytes(x: int) -> bytes:
    return x.to_bytes((x.bit_length() + 7) // 8, 'big')

def int_to_hashstr(x : int) -> str:
    return sha256(int_to_bytes(x)).hexdigest()

class BotDatabase:

    def __init__(self):
        self.connection = None
        self.cursor = None

        self.connection = psycopg2.connect(
            dbname = "Telegram2",
            user = "postgres",
            password = "newPassword",
            host = "127.0.0.1",
            port = "5432")

        self.cursor = self.connection.cursor()
    
    def if_registered(self, chat_id):
        self.cursor.execute("SELECT ChatId FROM User_Table WHERE login IS NOT NULL AND ChatId = %d;" % chat_id)
        result = self.cursor.rowcount
        self.connection.commit()
        return result

    def if_taken(self, login):
        self.cursor.execute("SELECT ChatId FROM User_Table WHERE login = '%s' AND ChatId IS NOT NULL;" % login)
        result = self.cursor.rowcount
        self.connection.commit()
        if result:
            return 0

        self.cursor.execute("SELECT Login From User_Table WHERE login = '%s';" % login)
        result = self.cursor.rowcount
        self.connection.commit()
        return result

    def check_reg(self, login, hash):
        self.cursor.execute("SELECT ChatId FROM Chat_Table Where login_to_check = '%s' AND hash_to_check = '%s';" % (login, hash))
        row_count = self.cursor.rowcount
        result = self.cursor.fetchall()
        self.connection.commit()
        if row_count != 1:
            return 1

        for row in result:
            chat_id = int(row[0])
            break
        self.cursor.execute("UPDATE User_Table SET ChatId = %d WHERE Login = '%s';" % (chat_id, login))
        self.connection.commit()
        return 0


    def add_login_to_check(self, chat_id, login_to_check):
        self.cursor.execute("UPDATE Chat_Table SET login_to_check = '%s' Where ChatId = %d" % (login_to_check, chat_id))
        self.connection.commit()
        return

    def check_secretcode(self, chat_id, secretcode_check):
        self.cursor.execute("SELECT secretcode from User_table JOIN Chat_Table ON User_table.Login = Chat_Table.login_to_check AND Chat_table.ChatId = %d;" % chat_id)
        row_count = self.cursor.rowcount
        result = self.cursor.fetchall()
        self.connection.commit()
        if row_count == 0:
            return row_count

        for row in result:
            secretcode = row[0]
            break

        if str(secretcode) == str(secretcode_check):
            return 1
        return 0

        
    def check_login(self, login):
        self.cursor.execute("SELECT ChatId FROM User_Table WHERE login = '%s';" % login)
        result = self.cursor.rowcount
        self.connection.commit()
        return result

    def check_chat_id(self, chat_id):
        self.cursor.execute("SELECT ChatId FROM Chat_Table WHERE ChatId = %d;" % chat_id)
        result = self.cursor.rowcount
        self.connection.commit()
        return result

    def add_chat_id(self, chat_id):
        if self.check_chat_id(chat_id) == 0:
            self.cursor.execute("INSERT INTO Chat_Table VALUES(%d, NULL, NULL, NULL);" % chat_id)
            self.connection.commit()
            return 0
        return 1



    def add_login(self, login):
        self.cursor.execute("SELECT Login FROM User_Table WHERE Login = '%s';" % login)
        result = self.cursor.rowcount
        self.connection.commit()
        if result:
            return
        self.cursor.execute("INSERT INTO User_Table VALUES ('%s',NULL, NULL);" % login)
        self.connection.commit()


    def check_int(self, secret_code):
        try: 
            if int(secret_code).bit_length() <= 16:
                return True
            else:
                return False
        except ValueError:
            return False
           
    def add_secret_code(self, chat_id, secret_code):
        if self.check_int(secret_code):
            hash_code = int_to_hashstr(int(secret_code))
            self.cursor.execute("UPDATE Chat_Table SET hash_to_check = '%s' WHERE ChatId = %d;" % (hash_code, chat_id))
            return True
        return False
        
    def add_hash_code(self, login, hash_code):
        self.cursor.execute("UPDATE User_Table SET hashcode = '%s' WHERE Login = '%s';" % (hash_code, login))
        self.connection.commit()

    def get_hash_code(self, login):
        self.cursor.execute("SELECT Hashcode FROM User_Table WHERE Login = '%s';" % (login))
        result = self.cursor.fetchall()
        for row in result:
            hash_code = row[0]
        return hash_code

    def get_chat_id(self, login):
        self.cursor.execute("SELECT ChatId FROM User_Table WHERE Login = '%s';" % (login))
        result = self.cursor.fetchall()
        for row in result:
            chat_id = row[0]
        return chat_id

    def link_login(self, chat_id):
        self.cursor.execute("UPDATE Chat_Table SET login = login_to_check WHERE ChatId = %d;" % chat_id)
        self.connection.commit()
        return 

    def update_stage(self, chat_id, stage):
        if self.check_chat_id(chat_id):
            self.cursor.execute("UPDATE Chat_Table SET Stage = %d WHERE ChatId = %d;" % (stage, chat_id))
            self.connection.commit()
            return 0
        return 1        

    def check_stage(self, chat_id):
        if self.check_chat_id(chat_id) == 1:
            self.cursor.execute("SELECT Stage FROM Chat_Table WHERE ChatId = %d;" % chat_id)
            result = self.cursor.fetchall()
            for row in result:
                stage = row[0]
            return stage
        else:
            return -1 
