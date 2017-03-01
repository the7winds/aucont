#!/usr/bin/python3

import os

def main(): 
	containers = map(lambda d: d.name, filter(lambda d: d.is_dir(), os.scandir('.')))
	processes = map(lambda d: d.name, filter(lambda d: d.is_dir(), os.scandir('/proc')))
	alive = set(containers) & set(processes)

	for pid in alive:
		print(pid)


if __name__ == '__main__':
	main()