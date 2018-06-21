import sys
sys.path.insert(0,'../utils')
import eCPPGraph
import evaluateSolution

import capacitedDominantingSet


g = eCPPGraph.graph('../testScripts/autoGen.xml')

g.makeSandBox()
g.printCurrentGraph()

s,a = capacitedDominantingSet.solve(g)
print s
print a
g.printCurrentGraph()
