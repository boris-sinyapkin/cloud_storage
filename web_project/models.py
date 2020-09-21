from django.db                  import models
from django.contrib.auth.models import AbstractUser

class UserProfile(AbstractUser):
    pass

class FileTable(models.Model):
    _uid        = models.ForeignKey(UserProfile, on_delete=models.CASCADE, to_field='id')
    _name       = models.CharField(max_length=260)
    _type       = models.CharField(max_length=260)
    _path       = models.CharField(max_length=260)
    upd_time    = models.DateTimeField(auto_now_add=True)

    class Meta:
        unique_together = ( '_uid', 'id' )