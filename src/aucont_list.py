#!/usr/bin/python3

import os

BASE = os.environ['AUCONT']

def main(): 
	containers = filter(lambda d: os.path.isdir(os.path.join(BASE, d)), os.listdir(BASE))
	processes = filter(lambda d: os.path.isdir(os.path.join('/proc', d)), os.listdir('/proc'))
	alive = set(containers) & set(processes)

	for pid in alive:
		print(pid)


if __name__ == '__main__':
	main()