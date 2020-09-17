from django.db import models
from django.contrib.auth.models import AbstractUser, User

class UserProfile(AbstractUser):
    is_telegram_auth = models.BooleanField(default=False)