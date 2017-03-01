#!/usr/bin/python3

import os

BASE = os.environ['AUCONT']

def main(): 
	containers = filter(os.path.isdir, os.listdir(BASE))
	processes = filter(os.path.isdir, os.listdir('/proc'))
	alive = set(containers) & set(processes)

	for pid in alive:
		print(pid)


if __name__ == '__main__':
	main()