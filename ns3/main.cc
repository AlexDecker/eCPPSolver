#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("simulacaoSBRC");

typedef struct{
	//para a contagem da energia gasta
	double joulesPerBit;
	double propagationTime;
	//nó cuja interface de rede (deste link) é identificado pelo final 1
	Ptr<Node> node1;
	//nó cuja interface de rede (deste link) é identificado pelo final 2
	Ptr<Node> node2;
	Ptr<Socket> socket;
}ControlLink;

class Controller{
	public:
		//vetor de [degree] posições
		ControlLink* controlPlaneLinks;
		//vetor de [degree+1] posições (o último liga o controlador e o
		//router da mesma localidade)
		ControlLink* southboundLinks;
		
		Controller(double _capacity, double _responseProbability,
			double _processingEnergy, double _energyPrice, uint32_t _responseSize,
			int degree);
		~Controller();
		//insere tanto o southbound link quanto o controlPlaneLink
		//correspondente à aresta real
		void addEdge();
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
		ControlLink* southboundLinks;
		
		Router(double _traffic, double _requestProbabilty, double _energyPrice,
			uint32_t _requestSize, int _degree);
		~Router();
		//insere um southbound link
		void addEdge();
		//contabiliza a energia gasta com o recebimento, processamento e eventual
		//transmissão de uma mensagem, além da transmissão de uma eventual solicitação
		//ao controlador
		void messageHandler();
		//contabiliza a energia gasta pelo recebimento e processamento da resposta e a
		//transmissão da mensagem
		void responseHandler();
	private:
		double traffic;//mensagens/s
		double requestProbabilty;//probabilidade de uma mensagem gerar um request
		double energyPrice;//dolares/joule
		uint32_t requestSize;//tamanho do request em bytes
};


class Wan{
	ControlLink* controlPlaneLinks;
	ControlLink* southBoundLinks;
	public:
		//vetores de [nLocations] posições
		Controller* controllers;
		Router* routers;
		
		Wan(int nControlPlaneLinks, int nSouthBoundLinks, int nLocations);
		int  addController();
		void addLink(double joulesPerBit, double propagationTime, int node1, int node2);
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
	Wan wan = Wan(0,0,10);
	wan.addController();
	Controller ctrl = Controller(0,0,0,0,0,3);
	ctrl.addEdge();
	Router rtr = Router(0, 0, 0, 0, 3);
	rtr.addEdge();
	
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

Controller::Controller(double _capacity, double _responseProbability, double _processingEnergy,
	double _energyPrice, uint32_t _responseSize, int degree){
	capacity = _capacity;//requests por segundo
	responseProbability = _responseProbability;//probabilidade de um request gerar um respose
	processingEnergy = _processingEnergy;
	energyPrice = _energyPrice;//dolares/joule
	responseSize = _responseSize;//tamanho da resposta em bytes
	
	controlPlaneLinks = (ControlLink*) malloc(sizeof(ControlLink)*degree);
	southboundLinks = (ControlLink*) malloc(sizeof(ControlLink)*(degree+1));
}

Controller::~Controller(){
	free(controlPlaneLinks);
	free(southboundLinks);
}

//insere tanto o southbound link quanto o controlPlaneLink
//correspondente à aresta real
void Controller::addEdge(){
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
	requestProbabilty = _requestProbabilty;//probabilidade de uma mensagem gerar um request
	energyPrice = _energyPrice;//dolares/joule
	requestSize = _requestSize;//tamanho do request em bytes
	
	southboundLinks = (ControlLink*) malloc(sizeof(ControlLink)*(degree+1));
}

Router::~Router(){
	free(southboundLinks);
}

//insere um southbound link
void Router::addEdge(){
}

//contabiliza a energia gasta com o recebimento, processamento e eventual
//transmissão de uma mensagem, além da transmissão de uma eventual solicitação ao controlador
void Router::messageHandler(){
}

//contabiliza a energia gasta pelo recebimento e processamento da resposta e a transmissão da mensagem
void Router::responseHandler(){
}

Wan::Wan(int nControlPlaneLinks, int nSouthBoundLinks, int nLocations){}

int  Wan::addController(){
	return 0;
}

void Wan::addLink(double joulesPerBit, double propagationTime, int node1,
	int node2){
}

Ptr<Socket> Wan::getSocket(InetSocketAddress iaddr){
	return NULL;
}

Ptr<Node> Wan::getNode(InetSocketAddress iaddr){
	return NULL;
}
