from django.db import models
from django.contrib.auth.models import AbstractUser

class UserProfile(AbstractUser):
    pass

class FileTable(models.Model):
    _uid  = models.OneToOneField(UserProfile, on_delete=models.CASCADE)
    _name = models.CharField(max_length=260)
    _type = models.CharField(max_length=260)
    _path = models.CharField(max_length=260)