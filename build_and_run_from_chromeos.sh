
sudo chroot --groups=video,sudo,plugdev,input,audio,wayland,crouton --userspec=markham:markham /run/crouton/mnt/stateful_partition/crouton/chroots/precise /home/markham/drm/build.sh && \
sudo mv drm.out /usr/local/bin/ && \
drm.out
