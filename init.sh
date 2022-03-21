#!/bin/bash
cd ./ovs

/sbin/modprobe openvswitch
/sbin/lsmod | grep openvswitch

rm -rf /usr/local/etc/openvswitch
rm -rf /usr/local/var/run/openvswitch

mkdir -p /usr/local/etc/openvswitch

sudo ovsdb-tool create /usr/local/etc/openvswitch/conf.db vswitchd/vswitch.ovsschema

mkdir -p /usr/local/var/run/openvswitch

ovsdb-server --remote=punix:/usr/local/var/run/openvswitch/db.sock \
--remote=db:Open_vSwitch,Open_vSwitch,manager_options \
--private-key=db:Open_vSwitch,SSL,private_key \
--certificate=db:Open_vSwitch,SSL,certificate \
--bootstrap-ca-cert=db:Open_vSwitch,SSL,ca_cert \
--pidfile --detach --log-file

ovs-vsctl --no-wait init

/usr/local/share/openvswitch/scripts/ovs-ctl --no-ovsdb-server start