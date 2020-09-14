from django.db import models
from django.contrib.auth.models import AbstractUser

class UserProfile(AbstractUser):
    is_telegram_auth = models.BooleanField(default=False)