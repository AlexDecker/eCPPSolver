#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"

#include <stdio.h>
#include <time.h>
#include <vector>
#include <algorithm>    // std::sort

/*
 * Endereçamento: 10.X.Y.Z
 * X: 0 (link de plano de controle) ou 1 (link da interface southbound)
 * Y: index do link no vetor global
 * Z: 1 ou 2 
*/

using namespace ns3;
using std::vector;

NS_LOG_COMPONENT_DEFINE ("simulacaoSBRC");

//payload da mensagem da primeira parte do algoritmo distribuído
typedef struct{
	int fromId;
	int ownerId;
	double demand;
}PayLoad;

//estruturas para a segunda parte do algoritmo de controle de topologia
typedef struct{
	double demand;//demanda do nó a que este link se direciona
	double propagationTime;
	int to;
}TCLink;

typedef struct{
	double residualCapacity;
	double weight;
	int parent;
	bool hasController;
	vector<TCLink> neighborhood;
}TCNode;

//Estrutura para a simulação dos enlaces físicos da wan

typedef struct{
	//para a contagem da energia gasta
	double joulesPerBit;
	double propagationTime;
	//nós cuja interface de rede (deste link) é identificada pelo final index+1
	Ptr<Node> nodes[2];
	//sockets correspondentes aos nós supracitados (neste link)
	Ptr<Socket> sockets[2];
	//endereços de ip correspondentes aos nós supracitados (neste link)
	Ipv4Address ipv4Addr[2];
	//index dos nós nas listas correspondentes
	int id[2];
}ControlLink;

//Funções para a comparação em sorts
bool compPropagation(ControlLink i, ControlLink j){
	return(i.propagationTime>j.propagationTime);
}
bool compDemand(TCLink i, TCLink j){
	return(i.demand>j.demand);
}

class Controller{
	public:
		vector<ControlLink> controlPlaneLinks;
		vector<ControlLink> southBoundLinks;
		
		Ptr<Node> node;
		int ID;
		double capacity;
		double responseProbability;//probabilidade de um request gerar um respose
		double processingEnergy;//joules/request+response
		double fixedEnergy;//joules/s
		double energyPrice;//dolares/joule
		uint32_t responseSize;//tamanho da resposta em bytes
		
		//Utilizado em estatísticas
		unsigned long int numberOfResponses;
		double transmittingConsumption;//em $
		double receivedLastRequest;//momento em que recebeu a última mensagem em s
		unsigned long int nCPMsgs;//número de mensagens do algoritmo de controle de topologia
		////
		//Utilizado no algoritmo de determinação de topologia
		int nLocations;
		double* demands;//Dados que o algoritmo distribuido coletará
		int nMessages;//quantidade de mensagens distintas recebidas
		//////
		
		Controller(double _capacity, double _responseProbability,
			double _processingEnergy, double fixedEnergy, double _energyPrice,
			uint32_t _responseSize, int _ID, int _nLocations);
		~Controller();
		
		void addLink(ControlLink link);
		//prepara e envia a resposta e calcula a energia gasta
		void requestHandler(Address from, Ptr<Packet> packet);
		
		//Gerenciamento de topologia:
		void initializeTopologyAlgorithm();//começa a divulgar os parâmetros
		void cpMessageHandler(Address from, Ptr<Packet> packet);//disseminação de parâmetros
		void recognizeAsChild(int routerId);//envia uma notificação de paternidade
		void posicionadorRelaxado(TCNode* tcNodes);//heurística que não considera a restrição de latência
		void heuristica1();//realiza de fato a otimização
		////
	private:
		void sendBroadcast(int index, int exceptBy);
};

class Router{
	public:
		int parent;
		vector<ControlLink> southBoundLinks;
		
		Ptr<Node> node;
		int ID;
		
		unsigned long int numberOfMessages;
		unsigned long int numberOfRequests;
		double transmittingConsumption;//em $
		
		double traffic;//mensagens/ns
		double requestProbability;//probabilidade de uma mensagem gerar um request
		
		Router(double _traffic, double _requestProbability, double _energyPrice,
			uint32_t _requestSize, int _ID);
		
		//insere um southbound link
		void addLink(ControlLink link);
		
		//interval é o intervalo dentro do qual essa função é chamada
		void sendRequest(Time interval);
		
		//contabiliza a energia gasta pelo recebimento e processamento da resposta
		//e a transmissão da mensagem
		void responseHandler(Address from, Ptr<Packet> packet);
	private:
		double energyPrice;//dolares/joule
		uint32_t requestSize;//tamanho do request em bytes
};


class Wan{
	private:
		vector<ControlLink> controlPlaneLinks;
		vector<ControlLink> southBoundLinks;
		
		NodeContainer nodes;
		void addSouthBoundLink(double joulesPerBit, double propagationTime,
			int controller, int router);
		void addControlPlaneLink(double joulesPerBit, double propagationTime,
			int controler1, int controler2);
		Controller* getControllerFromRouterIP(Address addr);
		Router* getRouterFromControllerIP(Address addr);
		Controller* getControllerFromControllerIP(Address addr);
	public:
		vector<Controller*> controllers;
		vector<Router*> routers;
		
		double maxControlLatency;
		
		void addLocation(Controller* ctrl, Router* rtr);
		void installIpv4();
		void addLink(double joulesPerBit, double propagationTime, int node1,
			int node2);
		//interval é o intervalo dentro do qual essa função é chamada
		void generateTraffic(Time interval);
		void handleRequest(Address from, Ptr<Packet> packet);
		void handleResponse(Address from, Ptr<Packet> packet);
		void handleCPMessage(Address from, Ptr<Packet> packet);
		void defineParents();//inicia o algoritmo distribuído nos controladores
		void printStatistics();
		double limitToCut(int nEdgesToCut);//[nEdgesToCut]º maior tempo de propagação
		double totalControlLatency(TCNode* tcNodes);//latência total dada uma configuração
		//topológica
};

Wan wan;//gloabal para facilitar os handlers

//função de callback para tratar respostas recebidas por determinado comutador
void ReceiveRequest(Ptr<Socket> socket);

//função de callback para tratar respostas recebidas por determinado comutador
void ReceiveResponse(Ptr<Socket> socket);

//função de callback para o algoritmo distribuido que roda no plano de controle
void ReceiveCPMessage(Ptr<Socket> socket);

//envia uma mensagem e ativa o temporizador para enviar mais, até terminbar pktCount
static void GenerateTraffic(uint32_t pktCount, Time pktInterval);

static void TopologyManager();

int main(int argc, char** argv){
	CommandLine cmd;
	cmd.Parse (argc, argv);
	
	double finishTime = 10.0;	//(s)
	
	//resolução de tempo em nanossegundos
	Time::SetResolution (Time::NS);
	srand((unsigned)time(NULL));
	
	wan.maxControlLatency = 0.5;//latência de todos os links deve somar 1s no máximo
	
	int nLocations = 2;
	uint32_t msgSize = 1024;//1kb<-Importante, esse valor não pode ser 1. Veja responseHandler
	//para melhores explicações
	
	//Alguns parâmetros de simulação
	uint32_t numPackets = 100000000;	//número máximo de pacotes a enviar
	//intervalo de envio dentro de um mesmo dispositivo (s)
	double interval = 0.0000001*msgSize*8;//Vazões de no máximo 10Gbps
	
	double capacity = 2000000/(msgSize*8);//2Gbps
	double responseProbability = 0.5;//50% de chance
	double processingEnergy = 0.00033;//J/mensagem
	double fixedEnergy = 600;//600W
	double energyPrice = 0.000000033;//$/J -> 0.12/kWh
	double requestProbability = 0.5;//50% de chance
	double traffic = 1000000/(msgSize*8);//1Gbps
	
	Controller ctrl1 = Controller(capacity, responseProbability, processingEnergy,
		fixedEnergy, energyPrice, msgSize,0,nLocations);
	Router rtr1 = Router(traffic, requestProbability, energyPrice, msgSize, 0);
	
	Controller ctrl2 = Controller(capacity, responseProbability, processingEnergy,
		fixedEnergy, energyPrice, msgSize,1,nLocations);
	Router rtr2 = Router(traffic/2, requestProbability, energyPrice, msgSize, 1);
	
	wan.addLocation(&ctrl1, &rtr1);
	wan.addLocation(&ctrl2, &rtr2);
	
	wan.installIpv4();
	
	double joulesPerBit = 0.000000033;
	double propagationTime = 0.5;
	int node1 = 0;
	int node2 = 1;
	wan.addLink(joulesPerBit, propagationTime, node1, node2);
	
	NS_LOG_UNCOND("INICIANDO SIMULAÇÃO");
		
	//chamando pela primeira vez o gerador de tráfego
	Simulator::Schedule(Seconds(0.0), &GenerateTraffic, numPackets,
		Seconds(interval));
	//chamando o gerador de topologia
	Simulator::Schedule(Seconds(0.0), &TopologyManager);

	Simulator::Stop(Seconds(finishTime));
	
	Simulator::Run();
	Simulator::Destroy();
	NS_LOG_UNCOND("EM "<<finishTime<<" SEGUNDOS DE SIMULAÇÃO: ");
	wan.printStatistics();
	
	return 0;
}

Controller::Controller(double _capacity, double _responseProbability,
	double _processingEnergy, double _fixedEnergy, double _energyPrice, 
	uint32_t _responseSize, int _ID, int _nLocations){
	capacity = _capacity;//requests por segundo
	//probabilidade de um request gerar um respose
	responseProbability = _responseProbability;
	processingEnergy = _processingEnergy;
	fixedEnergy = _fixedEnergy;
	energyPrice = _energyPrice;//dolares/joule
	responseSize = _responseSize;//tamanho da resposta em bytes
	
	node = CreateObject<Node>();
	ID = _ID;
	
	numberOfResponses = 0;
	receivedLastRequest = 0;
	transmittingConsumption = 0;//em $
	nCPMsgs = 0;
	
	//Para o algoritmo de controle de topologia
	nLocations = _nLocations;
	demands = (double*) malloc(sizeof(double)*_nLocations);
	nMessages = 0;
}

Controller::~Controller(){
	free(demands);
}

void Controller::addLink(ControlLink link){
	//separa os bytes do endereço de ipv4
	uint8_t buffer[4];
	//obtem um dos endereços associados
	Ipv4Address addr = link.ipv4Addr[0];
	//separa os bytes
	addr.Serialize(buffer);
	//verifica a natureza do link
	if(buffer[1]==1){
		//southbound
		southBoundLinks.push_back(link);
		NS_LOG_UNCOND("Inserido sbLink em controlador com endereços "<<link.ipv4Addr[0]<<" e "<<link.ipv4Addr[1]);
	}else{
		//control plane
		controlPlaneLinks.push_back(link);
		NS_LOG_UNCOND("Inserido cpLink em controlador com endereços "<<link.ipv4Addr[0]<<" e "<<link.ipv4Addr[1]);
	}
}

//prepara e envia a resposta e calcula a energia gasta
void Controller::requestHandler(Address from, Ptr<Packet> packet){
	int i;
	InetSocketAddress iAddr = InetSocketAddress::ConvertFrom(from);
	Ipv4Address refIpv4 = iAddr.GetIpv4();
	//resposta enviada com certa probabilidade
	if((float) rand()/RAND_MAX < responseProbability){
		//busca o destinatário correto
		for(i=0;i<(int)southBoundLinks.size();i++){
			//o index 0 em links southbound corresponde ao controlador
			if(southBoundLinks[i].ipv4Addr[1] == refIpv4){
				numberOfResponses++;//para estatísticas
				receivedLastRequest = Simulator::Now().GetSeconds();
				southBoundLinks[i].sockets[0]->Send(Create<Packet>(responseSize));
				//contabilizando energia gasta
				transmittingConsumption+=8*responseSize*southBoundLinks[i].joulesPerBit*energyPrice;
				break;
			}
		}
	}
}

void Controller::initializeTopologyAlgorithm(){
	int i;
	for(i=0;i<nLocations;i++){
		if(ID==i){
			demands[i] = wan.routers[ID]->traffic
				*wan.routers[ID]->requestProbability;
		}else{
			demands[i] = -1;
		}
	}
	nMessages = 1; //a própria demanda já foi obtida
	//envia a posição [ID] de demands para todos os vizinhos
	sendBroadcast(ID,-1);
}

//envia a demanda determinada pelo index a todos os vizinhos exceto
//o de ID [exceptBy]
void Controller::sendBroadcast(int index, int exceptBy){
	int i, j;
	//montando a mensagem: meuID, nó a que se refere o dado e o dado em si
	PayLoad payload;
	payload.fromId = ID;
	payload.ownerId = index;
	payload.demand = demands[index];
	//para cada vizinho
	for(i=0;i<(int)controlPlaneLinks.size();i++){
		//j se refere ao outro nó do link
		if(controlPlaneLinks[i].id[0] == ID) j = 1;
		else j = 0;
		//se o outro nó do link não for o proibido
		if(controlPlaneLinks[i].id[j] != exceptBy){
			controlPlaneLinks[i].sockets[1-j]->Send(
				Create<Packet>((uint8_t*)&payload,sizeof(payload)));
			//contabilizando energia gasta
			transmittingConsumption+=8*sizeof(payload)
				*controlPlaneLinks[i].joulesPerBit*energyPrice;
			//atualizando estatísticas
			nCPMsgs++;
		}
	}
}

void Controller::cpMessageHandler(Address from, Ptr<Packet> packet){
	//fetch da mensagem recebida
	PayLoad payload; packet->CopyData((uint8_t*)&payload, sizeof(PayLoad));
	//se o dado for novo
	if(demands[payload.ownerId]==-1){
		//insira no lugar
		demands[payload.ownerId] = payload.demand;
		//repasse via broadcast a todos menos o remetente
		sendBroadcast(payload.ownerId, payload.fromId);
		nMessages++;
		if(nMessages==nLocations){
			heuristica1();
		}
	}
}

void Controller::recognizeAsChild(int routerId){
	int i;
	//buscando o link para o filho desejado
	for(i=0;i<(int)southBoundLinks.size();i++){
		if(southBoundLinks[i].id[1]==routerId){
			//enviando notificação de paternidade
			southBoundLinks[i].sockets[0]->Send(Create<Packet>(1));
		}
	}
}

//heurística sem considerar a restrição de latência
void Controller::posicionadorRelaxado(TCNode* tcNodes){
	int i,j,k;
	int ndNodes = nLocations;//no início, nenhum nó está dominado
	while(ndNodes>0){
		//buscando o nó de maior eficiência, o index que a maximiza e a eficiência
		//em si
		int node, ind; double maxEff = 0;
		for(i=0;i<nLocations;i++){
			if(!tcNodes[i].hasController){//busque instalar um controlador se não houver um aqui
				double demandSum;
				if(tcNodes[i].parent==-1){
					demandSum = demands[i];//se não foi dominado ainda, deve atender a própria demanda
				}else{
					demandSum = 0;
				}
				//verificando quantos nós pode atender
				for(j=0;j<(int)tcNodes[i].neighborhood.size();j++){
					if(demandSum+tcNodes[i].neighborhood[j].demand<=tcNodes[i].residualCapacity){
						demandSum+=tcNodes[i].neighborhood[j].demand;
					}else{
						break;
					}
				}
				//verificando se a eficiência é maior do que a que já se tinha conhecimento
				if(maxEff<=j/tcNodes[i].weight){
					maxEff = j/tcNodes[i].weight;
					ind = j;//o penúltimo valor de j foi o último a respeitar a capacidade residual 
					node = i;
				}
			}
		}
		
		//O nó de maior eficiência passa a ter um controlador
		tcNodes[node].hasController = true;
		
		//os ind primeiros nós são dominados por node
		for(i=0;i<ind;i++){
			tcNodes[node].residualCapacity -= tcNodes[node].neighborhood[i].demand;
			tcNodes[tcNodes[node].neighborhood[i].to].parent = node;
			ndNodes--;
			//retirando esse nó das vizinhanças de outros nós
			for(j=0;j<nLocations;j++){
				if(j!=node){
					k=0;
					while(k<(int)tcNodes[j].neighborhood.size()){
						//verificando se é para um nó dominado
						if(tcNodes[j].neighborhood[k].to==tcNodes[node].neighborhood[i].to){
							//se sim, retire-o. Não avance o apontador, a remoção deste valor
							//trará o próximo
							tcNodes[j].neighborhood.erase(tcNodes[j].neighborhood.begin()+k);
						}else{
							//senão, avance o apontador
							k++;
						}
					}
				}
			}
		}
		
		//eliminando os nós dominados por node
		for(i=0;i<ind;i++){
			tcNodes[node].neighborhood.erase(tcNodes[node].neighborhood.begin());
		}
		
		//se node ainda não foi dominado
		if(tcNodes[node].parent==-1){
			tcNodes[node].residualCapacity -= demands[node];
			tcNodes[node].parent = node;
			ndNodes--;
			//retirando esse nó das vizinhanças de outros nós
			for(j=0;j<nLocations;j++){
				if(j!=node){
					k=0;
					while(k<(int)tcNodes[j].neighborhood.size()){
						//verificando se é para o nó em questão
						if(tcNodes[j].neighborhood[k].to==node){
							//se sim, retire-o. Não avance o apontador, a remoção deste valor
							//trará o próximo
							tcNodes[j].neighborhood.erase(tcNodes[j].neighborhood.begin()+k);
						}else{
							//senão, avance o apontador
							k++;
						}
					}
				}
			}
		}
	}
}

void Controller::heuristica1(){
	int i,j;
	int nEdgesToCut = 0;
	int x = 2;
	while(x--){
		//valor de tempo de propagação a partir do qual as arestas serão cortadas
		//(considero aqui, por motivo de simplificação, que os valores não se repetem.
		//caso se repitam, diferencie por algum valor infinitesimal)
		double lim = wan.limitToCut(nEdgesToCut);
		
		//preparando as estruturas para a próima iteração
		TCNode tcNodes[nLocations];
		for(i=0;i<nLocations;i++){
			//inicial
			tcNodes[i].parent = -1;
			tcNodes[i].hasController = false;
			tcNodes[i].residualCapacity = wan.controllers[i]->capacity;
			tcNodes[i].weight = wan.controllers[i]->energyPrice
				*(wan.controllers[i]->fixedEnergy
				+ demands[i] * wan.controllers[i]->processingEnergy);
			tcNodes[i].neighborhood.clear();//assegurando que está vazio
			for(j=0;j<(int)wan.controllers[i]->southBoundLinks.size();j++){
				//verificando se o link é para uma localidade externa ao controlador i
				//verificando também se há um limite de tempo de propagação a ser obedecido
				//(lim!=-1) e o enlace obedece esse limite
				if((wan.controllers[i]->southBoundLinks[j].id[1]!=wan.controllers[i]->ID)
					&&((wan.controllers[i]->southBoundLinks[j].propagationTime<lim)
					|| lim==-1)){
					
					TCLink link;
					
					link.to = wan.controllers[i]->southBoundLinks[j].id[1];//id do nó a que esse link leva
					link.demand = demands[link.to];
					link.propagationTime = wan.controllers[i]->southBoundLinks[j].propagationTime;
					tcNodes[i].neighborhood.push_back(link);
				}
			}
		}
		/*
		//Verificando dados
		for(i=0;i<nLocations;i++){
			//inicial
			NS_LOG_UNCOND("cap:"<<tcNodes[i].residualCapacity<<"; peso: "<<tcNodes[i].weight<<"; demand: "<<demands[i]);
			for(j=0;j<(int)tcNodes[i].neighborhood.size();j++){
				NS_LOG_UNCOND("  pt:"<<tcNodes[i].neighborhood[j].propagationTime<<"; demand:"<<tcNodes[i].neighborhood[j].demand);
			}
		}
		NS_LOG_UNCOND("\\");*/
		
		posicionadorRelaxado(tcNodes);
		
		if(wan.totalControlLatency(tcNodes)<=wan.maxControlLatency){
			for(i=0;i<nLocations;i++){
				if(tcNodes[i].parent==ID){
					recognizeAsChild(i);
				}
			}
			break;
		}else{
			nEdgesToCut++;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

Router::Router(double _traffic, double _requestProbability, double _energyPrice, 
	uint32_t _requestSize, int _ID){
	traffic = _traffic;//mensagens/s
	//probabilidade de uma mensagem gerar um request
	requestProbability = _requestProbability;
	energyPrice = _energyPrice;//dolares/joule
	requestSize = _requestSize;//tamanho do request em bytes
	
	numberOfMessages = 0;
	numberOfRequests = 0;
	transmittingConsumption = 0;//em $
	
	parent = 0;
	node = CreateObject<Node>();
	ID = _ID;
}

//insere um southbound link
void Router::addLink(ControlLink link){
	//separa os bytes do endereço de ipv4
	uint8_t buffer[4];
	//obtem um dos endereços associados
	Ipv4Address addr = link.ipv4Addr[0];
	//separa os bytes
	addr.Serialize(buffer);
	//verifica a natureza do link
	if(buffer[1]==1){
		//southbound
		southBoundLinks.push_back(link);
		NS_LOG_UNCOND("Inserido sbLink em comutador com endereços "<<link.ipv4Addr[0]<<" e "<<link.ipv4Addr[1]);
	}else{
		NS_LOG_UNCOND("Comutadores não possuem links de plano de controle.");
	}
}

void Router::sendRequest(Time interval){
	//probabilidade de uma mensagem ser processada agora
	//prob = período entre chamadas dessa função / período entre mensagens nesse comutador
	//prob = interval.GetSeconds()/(1/traffic);
	double probMsg = interval.GetSeconds()*traffic;
	probMsg = (probMsg>1)?1:probMsg;
	
	if((float) rand()/RAND_MAX < probMsg){
		numberOfMessages++;
		
		//gera um request com a probabilidade calculada
		if((float) rand()/RAND_MAX < requestProbability){
			//no caso de um southBound link, o index 1 sempre corresponde ao comutador,
			//de endereço ipv4 10.1.Y.1
			southBoundLinks[parent].sockets[1]->Send(Create<Packet>(requestSize));
			//contabilizando energia gasta
			transmittingConsumption+=8*requestSize*southBoundLinks[parent].joulesPerBit*energyPrice;
			//para estatísticas
			numberOfRequests++;
		}
	}
}

//contabiliza a energia gasta pelo recebimento e processamento da resposta e a
//transmissão da mensagem
void Router::responseHandler(Address from, Ptr<Packet> packet){
	int old;
	//a mensagem de determinação parental é identificada pelo tamanho (1) por
	//motivos de simplificação do ponto de vista da simulação
	if(packet->GetSize()==1){
		int i;
		InetSocketAddress iAddr = InetSocketAddress::ConvertFrom(from);
		Ipv4Address refIpv4 = iAddr.GetIpv4();
		//busca o remetente correto
		for(i=0;i<(int)southBoundLinks.size();i++){
			//o index 0 em links southbound corresponde ao controlador
			if(southBoundLinks[i].ipv4Addr[0] == refIpv4){
				old = parent;
				parent = i;
				NS_LOG_UNCOND("Alterando pai de "<<southBoundLinks[old].id[0]
					<<" para "<<southBoundLinks[parent].id[0]);
				break;
			}
		}
	}
}


//////////////////////////////////////////////////////////////////////////////

void Wan::addLocation(Controller* ctrl, Router* rtr){
	//registrando os nós novos no container global da rede
	nodes.Add(ctrl->node);
	nodes.Add(rtr->node);
	//inserindo os objetos nos vetores correspondentes
	controllers.push_back(ctrl);
	routers.push_back(rtr);
}

void Wan::installIpv4(){
	//Instalando a pilha ip em todos os nós
	InternetStackHelper internet;
	internet.Install(nodes);
	
	int i;
	for(i=0;i<(int)controllers.size();i++){
		//adicionando o link da interface southbound interna à localização i
		addSouthBoundLink(0, 0, i, i);
	}
}

void Wan::addLink(double joulesPerBit, double propagationTime, int node1,
	int node2){
	addSouthBoundLink(joulesPerBit, propagationTime, node1, node2);
	addSouthBoundLink(joulesPerBit, propagationTime, node2, node1);
	addControlPlaneLink(joulesPerBit, propagationTime, node1, node2);
}

void Wan::addSouthBoundLink(double joulesPerBit, double propagationTime,
	int controller, int router){
	ControlLink link;
	link.joulesPerBit = joulesPerBit;
	link.propagationTime = propagationTime;
	link.nodes[0] = controllers[controller]->node;
	link.nodes[1] = routers[router]->node;
	link.id[0] = controller;
	link.id[1] = router;
	//configurando conexão ponto a ponto
	char ptime[50];
	sprintf(ptime,"%fms",propagationTime);
	PointToPointHelper pointToPoint;
	pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
	pointToPoint.SetChannelAttribute("Delay", StringValue(ptime));
	//definindo endpoints
	NetDeviceContainer nDev = pointToPoint.Install(NodeContainer(
		controllers[controller]->node,
		routers[router]->node));
	
	//atribuindo endereços
	Ipv4AddressHelper ipv4;
	char addr_base[50];
	sprintf(addr_base,"10.1.%d.0", (int) southBoundLinks.size());
	char addr_controller[50];
	sprintf(addr_controller,"10.1.%d.1", (int) southBoundLinks.size());
	char addr_router[50];
	sprintf(addr_router,"10.1.%d.2", (int) southBoundLinks.size());
	
	link.ipv4Addr[0] = Ipv4Address(addr_controller);
	link.ipv4Addr[1] = Ipv4Address(addr_router);
	ipv4.SetBase(addr_base, "255.255.255.0");
	ipv4.Assign(nDev);
	
	//criando sockets
	TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
	link.sockets[0] = Socket::CreateSocket(link.nodes[0], tid);
	link.sockets[1] = Socket::CreateSocket(link.nodes[1], tid);
	
	//CONTROLADOR
	link.sockets[0]->Bind(InetSocketAddress(addr_controller, 80));
	link.sockets[0]->Connect(InetSocketAddress(addr_router, 80));
	//handler de requests dessa interface de rede do controlador
	link.sockets[0]->SetRecvCallback(MakeCallback(&ReceiveRequest));
	
	//COMUTADOR
	link.sockets[1]->Bind(InetSocketAddress(addr_router,80));
	link.sockets[1]->Connect(InetSocketAddress(addr_controller,80));
	//handler de response dessa interface de rede do comutador
	link.sockets[1]->SetRecvCallback(MakeCallback(&ReceiveResponse));
	//Inserindo o link na lista de Wan
	southBoundLinks.push_back(link);
	//Inserindo nas listas dos nós
	controllers[controller]->addLink(link);
	routers[router]->addLink(link);
}

void Wan::addControlPlaneLink(double joulesPerBit, double propagationTime,
	int controller1, int controller2){
	ControlLink link;
	link.joulesPerBit = joulesPerBit;
	link.propagationTime = propagationTime;
	link.nodes[0] = controllers[controller1]->node;
	link.nodes[1] = controllers[controller2]->node;
	link.id[0] = controller1;
	link.id[1] = controller2;
	//configurando conexão ponto a ponto
	char ptime[50];
    sprintf(ptime ,"%fms", propagationTime);
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
	pointToPoint.SetChannelAttribute("Delay", StringValue(ptime));
	//definindo endpoints
	NetDeviceContainer nDev = pointToPoint.Install(NodeContainer(
		controllers[controller1]->node,
		controllers[controller2]->node));
	
	//atribuindo endereços
	Ipv4AddressHelper ipv4;
	char addr_base[50];
	sprintf(addr_base,"10.0.%d.0", (int) controlPlaneLinks.size());
	char addr_controller1[50];
	sprintf(addr_controller1,"10.0.%d.1", (int) controlPlaneLinks.size());
	char addr_controller2[50];
	sprintf(addr_controller2,"10.0.%d.2", (int) controlPlaneLinks.size());
	
	link.ipv4Addr[0] = Ipv4Address(addr_controller1);
	link.ipv4Addr[1] = Ipv4Address(addr_controller2);
	ipv4.SetBase(addr_base, "255.255.255.0");
	ipv4.Assign(nDev);
	//criando sockets
	TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
	
	link.sockets[0] = Socket::CreateSocket(link.nodes[0], tid);
	link.sockets[1] = Socket::CreateSocket(link.nodes[1], tid);
	
	link.sockets[0]->Bind(InetSocketAddress(addr_controller1, 1023));
	link.sockets[1]->Bind(InetSocketAddress(addr_controller2, 1023));
	link.sockets[0]->Connect(InetSocketAddress(addr_controller2, 1023));
	link.sockets[1]->Connect(InetSocketAddress(addr_controller1, 1023));
	//handler de requests dessa interface de rede do controlador
	link.sockets[0]->SetRecvCallback(MakeCallback(&ReceiveCPMessage));
	link.sockets[1]->SetRecvCallback(MakeCallback(&ReceiveCPMessage));
	
	//Inserindo o link na lista de Wan
	controlPlaneLinks.push_back(link);
	//Inserindo nas listas dos nós
	controllers[controller1]->addLink(link);
	controllers[controller2]->addLink(link);
}

void Wan::generateTraffic(Time interval){
	int i;
	for(i=0;i<(int)routers.size();i++){
		routers[i]->sendRequest(interval);
	}
}

void Wan::handleRequest(Address from, Ptr<Packet> packet){
	Controller* ctrl = getControllerFromRouterIP(from);
	ctrl->requestHandler(from, packet);
}

void Wan::handleResponse(Address from, Ptr<Packet> packet){
	Router* rtr = getRouterFromControllerIP(from);
	rtr->responseHandler(from, packet);
}

void Wan::handleCPMessage(Address from, Ptr<Packet> packet){
	Controller* ctrl = getControllerFromControllerIP(from);
	ctrl->cpMessageHandler(from, packet);
}

Controller* Wan::getControllerFromRouterIP(Address addr){
	//separa os bytes do endereço de ipv4
	uint8_t buffer[4];
	InetSocketAddress isaddr = InetSocketAddress::ConvertFrom(addr);
	isaddr.GetIpv4().Serialize(buffer);
	//coleta o controller correspondente ao identificador Y do ip
	return controllers[southBoundLinks[(int)buffer[2]].id[0]];
}

Router* Wan::getRouterFromControllerIP(Address addr){
	//separa os bytes do endereço de ipv4
	uint8_t buffer[4];
	InetSocketAddress isaddr = InetSocketAddress::ConvertFrom(addr);
	isaddr.GetIpv4().Serialize(buffer);
	//coleta o router correspondente ao identificador Y do ip
	return routers[southBoundLinks[(int)buffer[2]].id[1]];
}

Controller* Wan::getControllerFromControllerIP(Address addr){
	//separa os bytes do endereço de ipv4
	uint8_t buffer[4];
	InetSocketAddress isaddr = InetSocketAddress::ConvertFrom(addr);
	isaddr.GetIpv4().Serialize(buffer);
	//coleta o controller correspondente ao identificador Y do ip
	return controllers[controlPlaneLinks[(int)buffer[2]].id[2-(int)buffer[3]]];
}

//inicia o algoritmo distribuído nos controladores
void Wan::defineParents(){
	int i=1;
	for(i=0; i<(int)controllers.size(); i++){
		controllers[i]->initializeTopologyAlgorithm();
	}
}

void Wan::printStatistics(){
	int i;
	double totalCtrlConsumption = 0;
	NS_LOG_UNCOND("Controladores:");
	for(i=0;i<(int)controllers.size();i++){
		double fixedConsumption = controllers[i]->receivedLastRequest
			*controllers[i]->fixedEnergy*controllers[i]->energyPrice;
		double processingConsumption = controllers[i]->numberOfResponses
			*controllers[i]->processingEnergy
			*controllers[i]->energyPrice;
		double transmittingConsumption = controllers[i]->transmittingConsumption;
		double controllerConsumption = fixedConsumption
			+processingConsumption+transmittingConsumption;
		NS_LOG_UNCOND("|"<<i<<"| Respostas: "<<controllers[i]->numberOfResponses
			<<"; Mensagens de plano de controle: "<<controllers[i]->nCPMsgs
			<<"; ConsumoFixo: "<<fixedConsumption
			<<"; Consumo por processamento: "<<processingConsumption
			<<"; Consumo por transmissão: "<<transmittingConsumption
			<<"; TOTAL: "<<controllerConsumption);
		totalCtrlConsumption+=controllerConsumption;
	}
	NS_LOG_UNCOND("TOTAL GASTO PELOS CONTROLADORES: $"<<totalCtrlConsumption);
	double totalRtrConsumption = 0;
	NS_LOG_UNCOND("Comutadores");
	for(i=0;i<(int)routers.size();i++){
		double transmittingConsumption = routers[i]->transmittingConsumption;
		NS_LOG_UNCOND("|"<<i<<"| Mensagens: "<<routers[i]->numberOfMessages
		<< "; Requisições: " << routers[i]->numberOfRequests
		<< "; Consumo por transmissão: "<<transmittingConsumption);
		totalRtrConsumption += transmittingConsumption;
	}
	NS_LOG_UNCOND("TOTAL GASTO PELOS COMUTADORES: $"<<totalRtrConsumption);
	NS_LOG_UNCOND("TOTAL GERAL: $"<<totalCtrlConsumption+totalRtrConsumption);
}

//////////////////////////////////////////////////////////////////////////

//função de callback para tratar respostas recebidas por determinado comutador
void ReceiveRequest(Ptr<Socket> socket){
	Ptr<Packet> packet;
	Address from;
	//para cada pacote
	while ((packet = socket->RecvFrom(from))){
		//se válido
		if (packet->GetSize() > 0){
			wan.handleRequest(from, packet);
		}
	}
}

//função de callback para tratar respostas recebidas por determinado comutador
void ReceiveResponse(Ptr<Socket> socket){
	Ptr<Packet> packet;
	Address from;
	//para cada pacote
	while((packet = socket->RecvFrom(from))){
		//se válido
		if(packet->GetSize()>0){
			wan.handleResponse(from, packet);
		}
	}
}

//função de callback para o algoritmo distribuido que roda no plano de controle
void ReceiveCPMessage(Ptr<Socket> socket){
	Ptr<Packet> packet;
	Address from;
	//para cada pacote
	while((packet = socket->RecvFrom(from))){
		//se válido
		if (packet->GetSize() > 0){
			wan.handleCPMessage(from, packet);
		}
	}
}

//envia uma mensagem e ativa o temporizador para enviar mais, até terminbar pktCount
static void GenerateTraffic(uint32_t pktCount, Time pktInterval){
	if (pktCount > 0){
		wan.generateTraffic(pktInterval);
		//ativa o temporizador para enviar mais
		Simulator::Schedule (pktInterval, &GenerateTraffic, pktCount-1, pktInterval);
	}
}

static void TopologyManager(){
	NS_LOG_UNCOND("INICIANDO GERENCIADOR DE TOPOLOGIA");
	wan.defineParents();
}

//[nEdgesToCut]º maior tempo de propagação
double Wan::limitToCut(int nEdgesToCut){
	int i;
	vector<ControlLink> links;
	if(nEdgesToCut==0) return -1;
	else{
		//copiando o vetor (para manter a ordem do original)
		for(i=0;i<(int)southBoundLinks.size();i++){
			links.push_back(southBoundLinks[i]);
		}
		//ordenando por tempo de propagação
		std::sort(links.begin(), links.end(), compPropagation);
		
		return(links[nEdgesToCut-1].propagationTime);
	}
}

double Wan::totalControlLatency(TCNode* tcNodes){
	int i,j;
	double lat = 0;
	//o tamanho de 'routers' é usado por ser exatamente o número de localidades
	for(i=0;i<(int)routers.size();i++){
		//links dentro de uma mesma localidade tem tempo de propagação 0
		if(tcNodes[i].parent != i){
			//encontrando o valor de tempo de propagação do link
			for(j=0;j<(int)southBoundLinks.size();j++){
				if((southBoundLinks[j].id[0]==tcNodes[i].parent)
					&&(southBoundLinks[j].id[1]==i)){
					lat += southBoundLinks[j].propagationTime;
				}
			}
		}
	}
	return lat;
}