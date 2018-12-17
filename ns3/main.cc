#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"

#include <stdio.h>
/*
 * Endereçamento: 10.X.Y.Z
 * X: 0 (link de plano de controle) ou 1 (link da interface southbound)
 * Y: index do link no vetor global
 * Z: 1 ou 2 
*/

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("simulacaoSBRC");

typedef struct{
	//para a contagem da energia gasta
	double joulesPerBit;
	double propagationTime;
	//nós cuja interface de rede (deste link) é identificada pelo final index+1
	Ptr<Node> nodes[2];
	//sockets correspondentes aos nós supracitados (neste link)
	Ptr<Socket> sockets[2];
}ControlLink;

class Controller{
	public:
		//vetor de [degree] posições
		ControlLink* controlPlaneLinks;
		//vetor de [degree+1] posições (o último liga o controlador e o
		//router da mesma localidade)
		ControlLink* southBoundLinks;
		
		Ptr<Node> node;
		
		Controller(double _capacity, double _responseProbability,
			double _processingEnergy, double _energyPrice, uint32_t _responseSize,
			int degree);
		~Controller();
		
		void addLink(ControlLink link);
		//prepara e envia a resposta e calcula a energia gasta
		void requestHandler();
	private:
		double capacity;//requests por segundo
		double responseProbability;//probabilidade de um request gerar um respose
		double processingEnergy;//joules/request+response
		double energyPrice;//dolares/joule
		uint32_t responseSize;//tamanho da resposta em bytes
		//envia a resposta e calcula a energia gasta
		double sendResponse();
};

class Router{
	public:
		ControlLink parent;
		//vetor de [degree+1] posições (o último liga o controlador e o
		//router da mesma localidade)
		ControlLink* southBoundLinks;
		
		Ptr<Node> node;
		
		Router(double _traffic, double _requestProbabilty, double _energyPrice,
			uint32_t _requestSize, int _degree);
		~Router();
		//insere um southbound link
		void addLink(ControlLink link);
		//contabiliza a energia gasta com o recebimento, processamento e eventual
		//transmissão de uma mensagem, além da transmissão de uma eventual
		//solicitaçãoao controlador
		void messageHandler();
		//contabiliza a energia gasta pelo recebimento e processamento da resposta
		//e a transmissão da mensagem
		void responseHandler();
	private:
		double traffic;//mensagens/s
		double requestProbabilty;//probabilidade de uma mensagem gerar um request
		double energyPrice;//dolares/joule
		uint32_t requestSize;//tamanho do request em bytes
};


class Wan{
	int nCPLinks;
	ControlLink* controlPlaneLinks;
	int nSBLinks;
	ControlLink* southBoundLinks;
	NodeContainer nodes;
	public:
		//vetores de [nLocations] posições
		int nLoc;
		Controller* controllers;
		Router* routers;
		
		Wan(int nCPLinks, int nLocations);
		int addLocation(Controller ctrl, Router rtr);
		int addRouter();
		void installIpv4();
		void addLink(double joulesPerBit, double propagationTime, int node1,
			int node2);
		int addSouthBoundLink(double joulesPerBit, double propagationTime,
			int controller, int router);
		int addControlPlaneLink(double joulesPerBit, double propagationTime,
			int controler1, int controler2);
		Ptr<Socket> getSocket(InetSocketAddress iaddr);
		Ptr<Node> getNode(InetSocketAddress iaddr);
};

Ptr<Socket> s_controlador1;//gambs momentânea. Usar uma estrutura com todos os sockets
//e selecionar pelo segundo campo do ip. Para isso, usar Serialize() e pegar buff[2]

//função de callback para tratar respostas recebidas por determinado comutador
void ReceiveRequest(Ptr<Socket> socket){
	Ptr<Packet> packet;
	Address from;
	//para cada pacote
	while ((packet = socket->RecvFrom(from))){
		//se válido
		if (packet->GetSize() > 0){
			//InetSocketAddress iaddr = InetSocketAddress::ConvertFrom(from);
			//uint8_t buf[4]; iaddr.GetIpv4().Serialize(buf);
			NS_LOG_UNCOND("Nova requisicao");
			//envie uma resposta
			s_controlador1->SendTo(Create<Packet>(
				packet->GetSize()),
				0,
				InetSocketAddress::ConvertFrom(from));
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
}

//envia uma mensagem e ativa o temporizador para enviar mais, até terminbar pktCount
static void GenerateTraffic (Ptr<Socket>* socket, uint32_t pktSize,
	uint32_t pktCount, Time pktInterval){
	if (pktCount > 0){
		//envia PktSize bytes
		socket[0]->Send(Create<Packet>(pktSize));
		//ativa o temporizador para enviar mais
		Simulator::Schedule (pktInterval, &GenerateTraffic, socket, pktSize,
			pktCount - 1, pktInterval);
	}else{
		//se já tiver enviado tudo, termine
		socket[0]->Close ();
	}
}

int main(int argc, char** argv){
	CommandLine cmd;
	cmd.Parse (argc, argv);
	
	//Alguns parâmetros de simulação
	uint32_t PpacketSize = 200;	//tamanho dos pacotes em bytes
	uint32_t numPackets = 10000;	//número máximo de pacotes a enviar
	//intervalo de envio dentro de um mesmo dispositivo (s)
	double interval = 1;
	
	double startTime = 0.0;	//(s)
	double finishTime = 10.0;	//(s)
	
	//resolução de tempo em nanossegundos
	Time::SetResolution (Time::NS);
	
	//criando nós da rede
	Ptr<Node> comutador = CreateObject<Node>();
	Ptr<Node> controlador1 = CreateObject<Node>();
	Ptr<Node> controlador2 = CreateObject<Node>();
	
	//agrupando os nós
	NodeContainer nodes = NodeContainer(comutador,controlador1,controlador2);//usar add
	NodeContainer nodesLink1 = NodeContainer(comutador,controlador1);
	NodeContainer nodesLink2 = NodeContainer(comutador,controlador2);
	
	//configurando uma conexão ponto a ponto
	PointToPointHelper pointToPoint;
	pointToPoint.SetDeviceAttribute ("DataRate", StringValue("5Mbps"));
	pointToPoint.SetChannelAttribute ("Delay", StringValue("2ms"));
	
	//criando um link ponto a ponto entre os nós 1 e 2
	NetDeviceContainer link1 = pointToPoint.Install(nodesLink1);
	NetDeviceContainer link2 = pointToPoint.Install(nodesLink2);
	
	//Instalando a pilha ip em todos os nós
	InternetStackHelper internet;
	internet.Install(nodes);
	
	//atribuindo endereços de ip (cada enlace é uma sub-rede)
	Ipv4AddressHelper ipv4;
	ipv4.SetBase("10.1.1.0", "255.255.255.0");
	ipv4.Assign(link1);
	ipv4.SetBase("10.1.2.0", "255.255.255.0");
	ipv4.Assign(link2);
	
	//usaremos UDP por conveniência
	TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
	
	//criando os sockets (a declaração de s_controlador1 está fora por gambs)
	Ptr<Socket> s_comutador1 = Socket::CreateSocket(comutador, tid);
	Ptr<Socket> s_comutador2 = Socket::CreateSocket(comutador, tid);
	s_controlador1 = Socket::CreateSocket(controlador1, tid);
	Ptr<Socket> s_controlador2 = Socket::CreateSocket(controlador2, tid);
	
	//configurando a parte servidora (controladores) e definindo handlers
	s_controlador1->Bind(InetSocketAddress(Ipv4Address::GetAny(), 80));
	s_controlador1->SetRecvCallback(MakeCallback(&ReceiveRequest));
	s_controlador2->Bind(InetSocketAddress(Ipv4Address::GetAny(), 80));
	s_controlador2->SetRecvCallback(MakeCallback(&ReceiveRequest));
	
	//configurando a parte cliente (o comutador) e definindo handlers
	//Há um socket para cada link
	Ptr<Ipv4> ipv4Controlador1 = controlador1->GetObject<Ipv4>();
	InetSocketAddress remote1 = InetSocketAddress(
		ipv4Controlador1->GetAddress(1,0).GetLocal(),
		80);
	s_comutador1->Connect(remote1);
	s_comutador1->SetRecvCallback(MakeCallback(&ReceiveResponse));
	
	Ptr<Ipv4> ipv4Controlador2 = controlador2->GetObject<Ipv4>();
	InetSocketAddress remote2 = InetSocketAddress(
		ipv4Controlador2->GetAddress(1,0).GetLocal(),
		80);
	s_comutador2->Connect(remote2);
	s_comutador2->SetRecvCallback(MakeCallback(&ReceiveResponse));
	
	//chamando pela primeira vez o gerador de tráfego
	Simulator::Schedule(Seconds(startTime), &GenerateTraffic, &s_comutador1,
		PpacketSize,numPackets, Seconds(interval));

	Simulator::Stop (Seconds(finishTime));
	
	Simulator::Run();
	Simulator::Destroy ();
	return 0;
}

Controller::Controller(double _capacity, double _responseProbability,
	double _processingEnergy, double _energyPrice, uint32_t _responseSize,
	int degree){
	capacity = _capacity;//requests por segundo
	//probabilidade de um request gerar um respose
	responseProbability = _responseProbability;
	processingEnergy = _processingEnergy;
	energyPrice = _energyPrice;//dolares/joule
	responseSize = _responseSize;//tamanho da resposta em bytes
	
	node = CreateObject<Node>();
	
	controlPlaneLinks = (ControlLink*) malloc(sizeof(ControlLink)*degree);
	southBoundLinks = (ControlLink*) malloc(sizeof(ControlLink)*(degree+1));
}

Controller::~Controller(){
	free(controlPlaneLinks);
	free(southBoundLinks);
}

void Controller::addLink(ControlLink link){
	//separa os bytes do endereço de ipv4
	uint8_t buffer[4];
	//obtem um dos sockets
	Ptr<Socket> socket = link.sockets[0];
	//obtem o endereço associado
	Ipv4Address addr = socket->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
	//separa os bytes
	addr.Serialize(buffer);
	//verifica a natureza do link
	if(buffer[1]==1){
		//southbound
		southBoundLinks[(int)buffer[2]] = link;
	}else{
		//control plane
		controlPlaneLinks[(int)buffer[2]] = link;
	}
}

//prepara e envia a resposta e calcula a energia gasta
void Controller::requestHandler(){
}

//envia a resposta e calcula a energia gasta
double Controller::sendResponse(){
	return 0.0;
}

Router::Router(double _traffic, double _requestProbabilty, double _energyPrice, 
	uint32_t _requestSize, int degree){
	traffic = _traffic;//mensagens/s
	//probabilidade de uma mensagem gerar um request
	requestProbabilty = _requestProbabilty;
	energyPrice = _energyPrice;//dolares/joule
	requestSize = _requestSize;//tamanho do request em bytes
	
	node = CreateObject<Node>();
	
	southBoundLinks = (ControlLink*) malloc(sizeof(ControlLink)*(degree+1));
}

Router::~Router(){
	free(southBoundLinks);
}

//insere um southbound link
void Router::addLink(ControlLink link){
	//separa os bytes do endereço de ipv4
	uint8_t buffer[4];
	//obtem um dos sockets
	Ptr<Socket> socket = link.sockets[0];
	//obtem o endereço associado
	Ipv4Address addr = socket->GetObject<Ipv4>()->GetAddress(1,0).GetLocal();
	//separa os bytes
	addr.Serialize(buffer);
	//verifica a natureza do link
	if(buffer[1]==1){
		//southbound
		southBoundLinks[(int)buffer[2]] = link;
	}else{
		NS_LOG_UNCOND("Comutadores não possuem links de plano de controle.");
	}
}

//contabiliza a energia gasta com o recebimento, processamento e eventual
//transmissão de uma mensagem, além da transmissão de uma eventual solicitação
//ao controlador
void Router::messageHandler(){
}

//contabiliza a energia gasta pelo recebimento e processamento da resposta e a
//transmissão da mensagem
void Router::responseHandler(){
}

Wan::Wan(int nCPLinks, int nLocations){
	int nSBLinks = nCPLinks + nLocations;
	controlPlaneLinks = (ControlLink*) malloc(nCPLinks*sizeof(ControlLink));
	southBoundLinks = (ControlLink*) malloc(nSBLinks*sizeof(ControlLink));
	nLoc = 0;//quantas locações já foram instaladas
	controllers = (Controller*) malloc(nLocations*sizeof(Controller));
	routers = (Router*) malloc(nLocations*sizeof(Router));
}

int  Wan::addLocation(Controller ctrl, Router rtr){
	//registrando os nós novos no container global da rede
	nodes.Add(ctrl.node);
	nodes.Add(rtr.node);
	//inserindo os objetos nos vetores correspondentes
	controllers[nLoc] = ctrl;
	routers[nLoc] = rtr;
	//adicionando o link da interface southbound interna à localização
	addSouthBoundLink(0, 0, nLoc, nLoc);
	return nLoc++;
}

void Wan::installIpv4(){
	//Instalando a pilha ip em todos os nós
	InternetStackHelper internet;
	internet.Install(nodes);
}

void Wan::addLink(double joulesPerBit, double propagationTime, int node1,
	int node2){
	addSouthBoundLink(joulesPerBit, propagationTime, node1, node2);
	addControlPlaneLink(joulesPerBit, propagationTime, node1, node2);
}

int Wan::addSouthBoundLink(double joulesPerBit, double propagationTime,
	int controller, int router){
	ControlLink link;
	link.joulesPerBit = joulesPerBit;
	link.propagationTime = propagationTime;
	link.nodes[0] = controllers[controller].node;
	link.nodes[1] = routers[router].node;
	//configurando conexão ponto a ponto
	char ptime[50];
	sprintf(ptime,"%fms",propagationTime);
	PointToPointHelper pointToPoint;
	pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
	pointToPoint.SetChannelAttribute("Delay", StringValue(ptime));
	//definindo endpoints
	NetDeviceContainer nDev = pointToPoint.Install(NodeContainer(
		controllers[controller].node,
		routers[router].node));
	
	//atribuindo endereços
	Ipv4AddressHelper ipv4;
	char addr[50];
	sprintf(addr,"10.1.%d.0", nSBLinks);
	ipv4.SetBase(addr, "255.255.255.0");
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
	Ptr<Ipv4> ipv4Ctrl = link.sockets[1]->GetObject<Ipv4>();
	InetSocketAddress remote = InetSocketAddress(
		ipv4Ctrl->GetAddress(1,0).GetLocal(),
		80);
	link.sockets[1]->Connect(remote);
	//handler de response dessa interface de rede do comutador
	link.sockets[1]->SetRecvCallback(MakeCallback(&ReceiveResponse));
	
	//Inserindo o link na lista de Wan
	southBoundLinks[nSBLinks] = link;
	//Inserindo nas listas dos nós
	controllers[controller].addLink(link);
	routers[router].addLink(link);
	
	return nSBLinks++;
}

int Wan::addControlPlaneLink(double joulesPerBit, double propagationTime,
	int controller1, int controller2){
	ControlLink link;
	link.joulesPerBit = joulesPerBit;
	link.propagationTime = propagationTime;
	link.nodes[0] = controllers[controller1].node;
	link.nodes[1] = controllers[controller2].node;
	//configurando conexão ponto a ponto
	char ptime[50];
    sprintf(ptime ,"%fms", propagationTime);
    PointToPointHelper pointToPoint;
    pointToPoint.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
	pointToPoint.SetChannelAttribute("Delay", StringValue(ptime));
	//definindo endpoints
	NetDeviceContainer nDev = pointToPoint.Install(NodeContainer(
		controllers[controller1].node,
		controllers[controller2].node));
	
	//atribuindo endereços
	Ipv4AddressHelper ipv4;
	char addr[50];
	sprintf(addr, "10.0.%d.0",nCPLinks);
	ipv4.SetBase(addr, "255.255.255.0");
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
	controlPlaneLinks[nCPLinks] = link;
	//Inserindo nas listas dos nós
	controllers[controller1].addLink(link);
	controllers[controller2].addLink(link);
	
	return nCPLinks++;
}

Ptr<Socket> Wan::getSocket(InetSocketAddress iaddr){
	//separa os bytes do endereço de ipv4
	uint8_t buffer[4];
	iaddr.GetIpv4().Serialize(buffer);
	//verifica a natureza do link
	if(buffer[1]==1){
		//controlPlane
		return controlPlaneLinks[(int)buffer[2]].sockets[(int)buffer[3]-1];
	}else{
		//southbound
		return southBoundLinks[(int)buffer[2]].sockets[(int)buffer[3]-1];
	}
}

Ptr<Node> Wan::getNode(InetSocketAddress iaddr){
	//separa os bytes do endereço de ipv4
	uint8_t buffer[4];
	iaddr.GetIpv4().Serialize(buffer);
	//verifica a natureza do link
	if(buffer[1]==1){
		//controlPlane
		return controlPlaneLinks[(int)buffer[2]].nodes[(int)buffer[3]-1];
	}else{
		//southbound
		return southBoundLinks[(int)buffer[2]].nodes[(int)buffer[3]-1];
	}
}
