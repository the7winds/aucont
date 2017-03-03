#!/usr/bin/python3

import sys
import os
import ipaddress as ip
import tarfile as tar

OLD = 'old'
BASE = os.path.dirname(os.path.realpath(__file__))

def main():
	pid = sys.argv[1]
	image = sys.argv[2]

	pid_path = os.path.join(BASE, pid)
	os.mkdir(pid_path)
	os.mkdir(os.path.join(pid_path, OLD));

	os.system("tar cf ./{}.tar -C {} .".format(pid, image))
	os.system("tar xf {}.tar -C {}".format(pid, pid_path))
	os.system("rm {}.tar".format(pid))
	
	if (sys.argv[3] != 'null'):
		container_ip = ip.ip_address(sys.argv[3]);
		host_ip = container_ip + 1;
		with open(os.path.join(pid_path, 'net_config'), 'w') as net_config:
			net_config.write("{}/24 {}/24".format(host_ip, container_ip))



if __name__ == '__main__':
	main()
