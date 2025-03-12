# enable ip forwarding (temporally)
sysctl -w net.ipv4.ip_forward=1
# enable nat (temporally)
iptables -t nat -A POSTROUTING -o wlo1 -j MASQUERADE
iptables -A FORWARD -i wlo1 -o zeth -m state --state RELATED,ESTABLISHED -j ACCEPT
iptables -A FORWARD -i zeth -o wlo1 -j ACCEPT
