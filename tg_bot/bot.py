import telebot
from telebot import types
import bot_db
import random

Database = bot_db.BotDatabase()

bot = telebot.TeleBot('1233665226:AAEh9GBqvei3oja-HHNbCktiZqegt79fr8s')
keyboard1 = types.ReplyKeyboardMarkup()
keyboard1.row('/info', '/registry')

updates = bot.get_updates()
updates = bot.get_updates(1234,100,20)

@bot.message_handler(commands=['info'])
def start_message(message):
    bot.send_message(message.chat.id, 'Hello, My name is CloudStorageBot. I will help you protect your data :)', reply_markup=keyboard1)
    bot.send_message(message.chat.id, '/registry - Registry in system (Link your Web-Account with Bot)', reply_markup=keyboard1)


@bot.message_handler(commands=['registry'])
def start_registry(message):
    Database.add_chat_id(message.chat.id)
    if Database.if_registered(message.chat.id) == 0:
        Database.update_stage(message.chat.id, 1)
        bot.send_message(message.chat.id, 'Enter username')
    else:
        bot.send_message(message.chat.id, 'You are already registered!')


@bot.message_handler(content_types=['text'])
def send_text(message):
    current_stage = Database.check_stage(message.chat.id)
    print("Current_stage " + str(current_stage))
    if current_stage == 1:
        if Database.if_taken(message.text) == 1:
            Database.update_stage(message.chat.id, 2)
            Database.add_login_to_check(message.chat.id, message.text)
            bot.send_message(message.chat.id, "Enter secret code")
            
        else:
            bot.send_message(message.chat.id, "Wrong Login! Please use login from Web-site")
            Database.update_stage(message.chat.id, 0)
    elif current_stage == 2:
        if Database.add_secret_code(message.chat.id, message.text) == False:
            bot.send_message(message.chat.id, "Invalid Secret Code!")
        Database.update_stage(message.chat.id, 0)
        
    else:
        bot.send_message(message.chat.id, "Unknown Command! Use /info to show functionality", reply_markup=keyboard1)
        Database.update_stage(message.chat.id, 0)
    


bot.polling(none_stop=True, timeout=1000)

