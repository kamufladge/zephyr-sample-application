
# kamufladge sample application

For an overview of a good sample visit [zephyr-application-sample](https://github.com/zephyrproject-rtos/example-application)

## build for native_sim with overlay offloaded

- ``-DEXTRA_CONF_FILE=overlay_offloaded.conf``

## build for virtual ethernet

- ``-DEXTRA_CONF_FILE=overlay_veth.conf``

### setup virtual networking

```bash
# create virtual ethernet zeth
./net-setup.sh up
# enable ip forwarding (temporally)
sudo sysctl -w net.ipv4.ip_forward=1
# enable nat (temporally)
sudo iptables -t nat -A POSTROUTING -o wlo1 -j MASQUERADE
sudo iptables -A FORWARD -i wlo1 -o zeth -m state --state RELATED,ESTABLISHED -j ACCEPT
sudo iptables -A FORWARD -i zeth -o wlo1 -j ACCEPT
```

## build for nucleo_board

- ``-DEXTRA_CONF_FILE=overlay_veth.conf``
