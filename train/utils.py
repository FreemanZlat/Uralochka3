from os import listdir
from os.path import isfile, join
from random import shuffle
import requests
import time

# TELEGRAM_TOKEN = ""
# TELEGRAM_CHAT_ID = "154308653"


def telegram_bot_sendtext(bot_message):
    try:
        if TELEGRAM_TOKEN == "" or TELEGRAM_CHAT_ID == "":
            return

        send_text = 'https://api.telegram.org/bot' + TELEGRAM_TOKEN +\
                    '/sendMessage?chat_id=' + TELEGRAM_CHAT_ID +\
                    '&parse_mode=Markdown&text=' + bot_message
    except NameError:
        return

    try:
        requests.get(send_text)
    except:
        print("Can't send message to Telegram")


def current_time():
    return round(time.time() * 1000)


class Files:
    def __init__(self, dir, shuffle_first=True, shuffle_iter=True, files_count=0, keep_last=0):
        self.files = [join(dir, f) for f in listdir(dir) if isfile(join(dir, f))]
        self.files.sort()

        if keep_last != 0:
            self.files = self.files[-keep_last:]

        self.shuffle_iter = shuffle_iter
        self.idx = 0

        self.files_count = files_count
        if self.files_count == 0:
            self.files_count = len(self.files)

        if shuffle_first:
            shuffle(self.files)

    def get_file(self):
        file = self.files[self.idx]
        self.idx += 1
        if self.idx >= self.files_count:
            self.idx = 0
            if self.shuffle_iter:
                shuffle(self.files)
        return file

    def size(self):
        return self.files_count
