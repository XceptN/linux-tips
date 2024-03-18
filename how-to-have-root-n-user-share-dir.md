## A way to provide root and regular user to share a full r/w directory without using symbolic links 

### At home directory
```
$ pwd
/home/oyuksel
```

### Create a file to be used as shared disk (e.g. 200 MB)
```
$ dd if=/dev/zero of=dualmount.img bs=200M count=1
```
### Create ext4 fs on it
```
mkfs.ext4 dualmount.img
```
### Create dir to "mount to" for regular user
```
$ mkdir /home/oyuksel/dualmount
```
### Create dir to "mount to" for root
```
# mkdir /root/dualmount
```
### Update /etc/fstab for mounts with allows. Add lines:
```
/home/oyuksel/dualmount.img     /home/oyuksel/dualmount ext4    loop,user,noauto       0 0
/home/oyuksel/dualmount.img     /root/dualmount ext4    loop,user,noauto       0 0
```
### Mount as root
```
# mount /root/dualmount
```
### Create a shared dir and change owner
```
# mkdir /root/dualmount/shared_dir
# chown oyuksel. /root/dualmount/shared_dir
```
### Mount as regular user
```
$ mount /home/oyuksel/dualmount
```
### Test access as regular user
```
$ touch /home/oyuksel/dualmount/shared_dir/newfile01
```
### Check from root side
```
# ls -l /root/dualmount/shared_dir
total 0
-rw-rw-r-- 1 oyuksel oyuksel 0 Jan 18 17:37 newfile01
# 
```