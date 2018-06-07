#include "greedy.h"

//fills C structure based on N data. Returns true only if a
//feasible solution was found
bool optimizeNetwork(ctrlConnections* C, net N){
	//starts with one controller for each location
	//removes one controller 
	C->bestTotalCost = -1;//starts invalid
	double totalCost;
	bool ctrlAdjMatrix[N.numLocations][N.numLocations];
	//index of the locations of the remaining controllers
	int controllerLocations[N.numLocations];
	int i, j, k;

	//starts as an identidy matrix
	for(i=0;i<N.numLocations;i++){
		for(j=0;j<N.numLocations;j++){
			C->bestCtrlAdjMatrix[i][j]=(i==j);
			ctrlAdjMatrix[i][j]=(i==j);
		}
	}
	
	//If even the identity matrix is feasible, there is no
	//feasible solution
	if(!isFeasible(ctrlAdjMatrix, N)) return false;
	
	//K = number of controllers
	for(k=N.numLocations-1;k>=1;k--){
	}
	return true;
}
