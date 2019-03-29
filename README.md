# eCPP - energy-cost Controller Placement Problem solver
Controller Placement Problem heuristics and branch n' bound algorithm for optimizing energy financial cost with latency constraint and demand+capacity support.

Requeriments (with the versions I used):
* Python 2.7.12
* GLPK 4.35
* ns3 3.26 and related stuff (waf for example)

Instructions:
* A quick test for the python-based stuff:
	* cd testScripts/tests/
	* ./test1.py
	* (this script generates several random networks using fixed parammeters such as simulation area and number of edges)
* Using the python code:
	* Create a .xml file with the desired parammeters or use generateXMLInput.gen for creating online input files
	* Create a graph with eCPPGraph.graph using the generatedInput.xml or your file
	* Call the function run of the chosen algorithm and inform the graph object as parammeter
* Simulating with ns-3:
	* set the desired parammeters in ns3/main.cc
	* copy and paste main.cc into scratch/ folder
	* go to the waf folder
	* ./waf
