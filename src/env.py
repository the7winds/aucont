#!/usr/bin/python3

import sys
import os
import ipaddress as ip
import tarfile as tar

ROOT = 'rootfs'
OLD = 'old'
CGROUP = 'cgrouph'

def main():
	cw = os.curdir
	pid = sys.argv[1]
	image = sys.argv[2]

	pid_path = os.path.join(cw, pid)
	os.mkdir(pid_path)

	root_path = os.path.join(pid_path, ROOT)
	os.mkdir(root_path)
	os.mkdir(os.path.join(root_path, OLD));

	os.system("tar cf ./{}.tar -C {} .".format(pid, image))
	os.system("tar xf {}.tar -C {}".format(pid, root_path))
	os.system("rm {}.tar".format(pid))

	os.mkdir(os.path.join(pid_path, CGROUP))	
	
	if (sys.argv[3] != 'null'):
		host_side_ip = ip.ip_address(sys.argv[3]);
		container_ip = host_side_ip + 1;
		with open(os.path.join(pid_path, 'net_config'), 'w') as net_config:
			net_config.write("{}/24 {}/24".format(host_side_ip, container_ip))



if __name__ == '__main__':
	main()
