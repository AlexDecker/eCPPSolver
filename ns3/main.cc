#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"

#include <stdio.h>
#include <time.h>
#include <vector>

/*
 * Endereçamento: 10.X.Y.Z
 * X: 0 (link de plano de controle) ou 1 (link da interface southbound)
 * Y: index do link no vetor global
 * Z: 1 ou 2 
*/

using namespace ns3;
using std::vector;

NS_LOG_COMPONENT_DEFINE ("simulacaoSBRC");

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

class Controller{
	public:
		//vetor de [degree] posições
		vector<ControlLink> controlPlaneLinks;
		//vetor de [degree+1] posições (o último liga o controlador e o
		//router da mesma localidade)
		vector<ControlLink> southBoundLinks;
		
		Ptr<Node> node;
		int ID;
		
		Controller(double _capacity, double _responseProbability,
			double _processingEnergy, double _energyPrice, uint32_t _responseSize,
			int degree, int _ID);
		
		void addLink(ControlLink link);
		//prepara e envia a resposta e calcula a energia gasta
		void requestHandler(Address from, Ptr<Packet> packet);
	private:
		double capacity;//requests por segundo
		double responseProbability;//probabilidade de um request gerar um respose
		double processingEnergy;//joules/request+response
		double energyPrice;//dolares/joule
		uint32_t responseSize;//tamanho da resposta em bytes
};

class Router{
	public:
		int parent;
		//vetor de [degree+1] posições (o último liga o controlador e o
		//router da mesma localidade)
		vector<ControlLink> southBoundLinks;
		
		Ptr<Node> node;
		int ID;
		
		Router(double _traffic, double _requestProbabilty, double _energyPrice,
			uint32_t _requestSize, int _degree, int _ID);
		
		//insere um southbound link
		void addLink(ControlLink link);
		
		//interval é o intervalo dentro do qual essa função é chamada
		void sendRequest(Time interval);
		
		//contabiliza a energia gasta com o recebimento, processamento e eventual
		//transmissão de uma mensagem, além da transmissão de uma eventual
		//solicitaçãoao controlador
		void messageHandler(Address from, Ptr<Packet> packet);
		//contabiliza a energia gasta pelo recebimento e processamento da resposta
		//e a transmissão da mensagem
		void responseHandler(Address from, Ptr<Packet> packet);
	private:
		double traffic;//mensagens/ns
		double requestProbabilty;//probabilidade de uma mensagem gerar um request
		double energyPrice;//dolares/joule
		uint32_t requestSize;//tamanho do request em bytes
};


class Wan{
	private:
		vector<ControlLink> controlPlaneLinks;
		vector<ControlLink> southBoundLinks;
		vector<Controller*> controllers;
		vector<Router*> routers;
		NodeContainer nodes;
		void addSouthBoundLink(double joulesPerBit, double propagationTime,
			int controller, int router);
		void addControlPlaneLink(double joulesPerBit, double propagationTime,
			int controler1, int controler2);
	public:
		void addLocation(Controller* ctrl, Router* rtr);
		void installIpv4();
		void addLink(double joulesPerBit, double propagationTime, int node1,
			int node2);
		//interval é o intervalo dentro do qual essa função é chamada
		void generateTraffic(Time interval);
		void handleRequest(Address from, Ptr<Packet> packet);
		Controller* getControllerFromRouterIP(Address addr);
};

Wan wan;//gloabal para facilitar os handlers

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
	while ((packet = socket->RecvFrom(from))){
		//se válido
		if (packet->GetSize() > 0){
			NS_LOG_UNCOND("Nova resposta");
		}
	}
}

//função de callback para o algoritmo distribuido que roda no plano de controle
void ReceiveCPMessage(Ptr<Socket> socket){
	Ptr<Packet> packet;
	Address from;
	//para cada pacote
	while ((packet = socket->RecvFrom(from))){
		//se válido
		if (packet->GetSize() > 0){
			NS_LOG_UNCOND("Nova mensagem de plano de controle");
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

int main(int argc, char** argv){
	CommandLine cmd;
	cmd.Parse (argc, argv);
	
	//Alguns parâmetros de simulação
	uint32_t numPackets = 10000;	//número máximo de pacotes a enviar
	//intervalo de envio dentro de um mesmo dispositivo (s)
	double interval = 1;
	
	double startTime = 0.0;	//(s)
	double finishTime = 10.0;	//(s)
	
	//resolução de tempo em nanossegundos
	Time::SetResolution (Time::NS);
	srand((unsigned)time(NULL));
	
	double capacity = 2;//2 requests por ns
	double responseProbability = 0.5;//50% de chance
	double processingEnergy = 0;//J/mensagem
	double energyPrice = 0;//$/J
	uint32_t responseSize = 1024;//1kb
	uint32_t requestSize = 1024;//1kb
	double requestProbabilty = 0.5;//50% de chance
	int degree = 1;
	double traffic = 0.7;//0.7 mensagens por s
	
	Controller ctrl1 = Controller(capacity, responseProbability, processingEnergy,
		energyPrice, responseSize,degree,0);
	Router rtr1 = Router(traffic, requestProbabilty, energyPrice, requestSize,
		degree,0);
	
	Controller ctrl2 = Controller(capacity, responseProbability, processingEnergy,
		energyPrice, responseSize,degree,1);
	Router rtr2 = Router(traffic, requestProbabilty, energyPrice, requestSize,
		degree,1);
	
	wan.addLocation(&ctrl1, &rtr1);
	wan.addLocation(&ctrl2, &rtr2);
	
	wan.installIpv4();
	
	double joulesPerBit = 0;
	double propagationTime = 0;
	int node1 = 0;
	int node2 = 1;
	wan.addLink(joulesPerBit, propagationTime, node1, node2);
	
	NS_LOG_UNCOND("INICIANDO SIMULAÇÃO");
	
	rtr1.parent = 0;
	rtr2.parent = 1;
	
	//chamando pela primeira vez o gerador de tráfego
	Simulator::Schedule(Seconds(startTime), &GenerateTraffic, numPackets,
		Seconds(interval));

	Simulator::Stop(Seconds(finishTime));
	
	Simulator::Run();
	Simulator::Destroy ();
	return 0;
}

Controller::Controller(double _capacity, double _responseProbability,
	double _processingEnergy, double _energyPrice, uint32_t _responseSize,
	int degree, int _ID){
	capacity = _capacity;//requests por segundo
	//probabilidade de um request gerar um respose
	responseProbability = _responseProbability;
	processingEnergy = _processingEnergy;
	energyPrice = _energyPrice;//dolares/joule
	responseSize = _responseSize;//tamanho da resposta em bytes
	
	node = CreateObject<Node>();
	ID = _ID;
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
	for(i=0;i<(int)southBoundLinks.size();i++){
		//o index 0 em links southbound corresponde ao controlador
		if(southBoundLinks[i].ipv4Addr[1] == refIpv4){
			NS_LOG_UNCOND("Nova requisição recebida pelo controlador "<<ID<<", de "<<southBoundLinks[i].ipv4Addr[1]<<" para "<<southBoundLinks[i].ipv4Addr[0]);
			NS_LOG_UNCOND("Disparando resposta");
			southBoundLinks[i].sockets[0]->SendTo(
				Create<Packet>(responseSize),0,iAddr);
			break;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

Router::Router(double _traffic, double _requestProbabilty, double _energyPrice, 
	uint32_t _requestSize, int degree, int _ID){
	traffic = _traffic;//mensagens/s
	//probabilidade de uma mensagem gerar um request
	requestProbabilty = _requestProbabilty;
	energyPrice = _energyPrice;//dolares/joule
	requestSize = _requestSize;//tamanho do request em bytes
	
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
	
	//probabilidade de um request ser enviado é a probabilidade de uma mensagem ser processada
	//E gerar um request
	double probReq = probMsg * requestProbabilty;
	
	//gera um request com a probabilidade calculada
	if((float) rand()/RAND_MAX < probReq){
		NS_LOG_UNCOND("Disparando requisição");
		//no caso de um southBound link, o index 1 sempre corresponde ao comutador,
		//de endereço ipv4 10.1.Y.1
		southBoundLinks[parent].sockets[1]->Send(Create<Packet>(requestSize));
		printf(">>>>%d, %d, p=%d\n", southBoundLinks[parent].id[0], southBoundLinks[parent].id[1],parent);
	}
}
//contabiliza a energia gasta com o recebimento, processamento e eventual
//transmissão de uma mensagem, além da transmissão de uma eventual solicitação
//ao controlador
void Router::messageHandler(Address from, Ptr<Packet> packet){
}

//contabiliza a energia gasta pelo recebimento e processamento da resposta e a
//transmissão da mensagem
void Router::responseHandler(Address from, Ptr<Packet> packet){
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
	//controlador se torna servidor
	link.sockets[0]->Bind(InetSocketAddress(Ipv4Address::GetAny(), 80));
	//handler de requests dessa interface de rede do controlador
	link.sockets[0]->SetRecvCallback(MakeCallback(&ReceiveRequest));
	
	//COMUTADOR
	//comutador se torna cliente
	InetSocketAddress remote = InetSocketAddress(
		addr_controller,
		80);
	
	link.sockets[1]->Connect(remote);
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
	
	link.sockets[0]->Bind(InetSocketAddress(Ipv4Address::GetAny(), 80));
	link.sockets[1]->Bind(InetSocketAddress(Ipv4Address::GetAny(), 80));
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

Controller* Wan::getControllerFromRouterIP(Address addr){
	//separa os bytes do endereço de ipv4
	uint8_t buffer[4];
	InetSocketAddress isaddr = InetSocketAddress::ConvertFrom(addr);
	isaddr.GetIpv4().Serialize(buffer);
	//coleta o controller correspondente ao identificador Y do ip
	return controllers[southBoundLinks[(int)buffer[2]].id[0]];
}