### Create testing environment
```
# pvcreate /dev/disk/by-id/scsi-0DO_Volume_volume-ams3-01
# pvcreate /dev/disk/by-id/scsi-0DO_Volume_volume-ams3-02
# vgcreate vgtest /dev/disk/by-id/scsi-0DO_Volume_volume-ams3-01
# lvcreate -l 100%FREE -n lvtest vgtest
# mkfs -t ext4 /dev/vgtest/lvtest
# mount -t ext4 /dev/vgtest/lvtest /lvmtest
# dd if=/dev/zero of=/lvmtest/a4G bs=1M count=4096
# vgextend vgtest /dev/disk/by-id/scsi-0DO_Volume_volume-ams3-02
# lvextend -l +100%FREE /dev/vgtest/lvtest
# resize2fs /dev/vgtest/lvtest
# df -h
# umount /lvmtest
# e2fsck -f /dev/vgtest/lvtest
# resize2fs /dev/vgtest/lvtest
# mount -t ext4 /dev/vgtest/lvtest /lvmtest
# dd if=/dev/zero of=/lvmtest/b4G bs=1M count=4096
# sync
```
### Now try to shrink it back to ~5G
```
# rm /lvmtest/a4G
```
### We would need to empty the FS more (due to internal maintenance)
```
# rm /lvmtest/b4G
```
### Shrink the FS
```
# umount /lvmtest
# e2fsck -f /dev/vgtest/lvtest
# resize2fs -p /dev/vgtest/lvtest 5G
```
### Need to go below 5G due to internal allocations etc.
```
# resize2fs -p /dev/vgtest/lvtest 4000M
# lvreduce -L 4000M /dev/vgtest/lvtest
```
### We will remove the secondly added disk, so we need to empty it
```
# pvmove /dev/disk/by-id/scsi-0DO_Volume_volume-ams3-02
# vgreduce -a vgtest
# mount -t ext4 /dev/vgtest/lvtest /lvmtest
# ls -l /lvmtest/
```