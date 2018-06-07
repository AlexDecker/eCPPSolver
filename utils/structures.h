#include <stdlio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct{
	double requestFrequency;
	double energyPerRequest;//only spent with the processing (joules)
}switchDevice;

typedef struct{
	double maxRequestFreq;
	double staticPower;//W
	double energyPerResponse;//only spent with processing (joules)
}ctrlDevice;

typedef struct{
	switchDevice switchDev;
	bool hasController;
	double energyCost;//cost per joule
}location;

typedef struct{
	bool valid;
	//energy needed to send a message from switch in i to a controller
	//in j
	double** energy;//Joules
	//latency of the request/response from switch in i to a controller in
	//j (RTT)
	double** latency;//seconds
}link;

typedef struct{
	int numLocations;
	location* locations;
	link** links;
	double maxTotalLatency;//seconds
}net;

typedef struct{
	double bestTotalCost;
	bool** bestCtrlAdjMatrix;
}ctrlConnections;
