import sys
sys.path.insert(0,'../utils')
import eCPPGraph
import capacitedDominantingSet
import evaluateSolution
import numpy

g = eCPPGraph.graph('../testScripts/autoGen.xml')

g.makeSandBox()
g.printCurrentGraph()

s,a = capacitedDominantingSet.solve(g)
print s
print numpy.matrix(a)
v,f = evaluateSolution.eval(s,a,g)
print v
print f