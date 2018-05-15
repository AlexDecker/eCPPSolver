#!/usr/bin/env python

import sys
from xml.dom import minidom
import constraints
import objective


def main():
	if(len(sys.argv)!=2):
		print 'Usage: lsGenerator [configuration file].xml'
		exit()

	config_fn = sys.argv[1] #configuration file name
	config = minidom.parse(config_fn)

	edgelist = config.getElementsByTagName('edge')
	nodelist = config.getElementsByTagName('node')

	for e in edgelist:
		print(e.attributes['latency'].value)

main()
