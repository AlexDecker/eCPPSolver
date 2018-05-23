#!/usr/bin/env python

import sys

def main():
	if(len(sys.argv)!=4):
		print 'Usage: logger [log file].log'
		exit()
	while(True):
		line = raw_input()
		if((line=='End of output')or(line=='')):
			print 'Error, no valid output'
			exit()
		line_l = line.split(' ')
		if(line_l[0]=='Objective:'):
			val = float(line_l[4])
			break
	time = float(sys.argv[3])-float(sys.argv[2])
	with open(sys.argv[1], "a") as myfile:
		myfile.write(str(time)+', '+str(val)+';\n')

main()
