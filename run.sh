palm-launch -c com.yourdomain.premplayer
palm-package .
palm-install com.yourdomain.premplayer_1.0.0_all.ipk
novaterm <<EOF
chmod +x /media/cryptofs/apps/usr/palm/applications/com.yourdomain.premplayer/premplayer
rm /tmp/premplayer.txt
exit
EOF
stty sane
palm-launch -i com.yourdomain.premplayer
palm-inspector &
palm-log --system-log-level info
palm-log -f com.yourdomain.premplayer
killall palm-inspector

