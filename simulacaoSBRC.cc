#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("simulacaoSBRC");

//função de callback para tratar pacotes recebidos em determinado socket
void ReceivePacket (Ptr<Socket> socket){
	Ptr<Packet> packet;
	Address from;
	//para cada pacote
	while ((packet = socket->RecvFrom(from))){
		//se válido
		if (packet->GetSize () > 0){
			NS_LOG_UNCOND ("Novo pacote recebido");
		}
	}
}


//envia uma mensagem e ativa o temporizador para enviar mais, até terminbar pktCount
static void GenerateTraffic (Ptr<Socket>* socket, uint32_t pktSize,
	uint32_t pktCount, Time pktInterval){
	if (pktCount > 0){
		//envia PktSize bytes
		socket[0]->Send (Create<Packet> (pktSize));
		//ativa o temporizador para enviar mais
		Simulator::Schedule (pktInterval, &GenerateTraffic, socket, pktSize,
			pktCount - 1, pktInterval);
	}else{
		//se já tiver enviado tudo, termine
		socket[0]->Close ();
	}
}

int main (){
	
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
	Ptr<Node> node1 = CreateObject<Node>();
	Ptr<Node> node2 = CreateObject<Node>();
	
	//agrupando os nós
	NodeContainer nodesLink1_2 = NodeContainer(node1,node2);
	
	//configurando uma conexão ponto a ponto
	PointToPointHelper pointToPoint;
	pointToPoint.SetDeviceAttribute ("DataRate", StringValue ("5Mbps"));
	pointToPoint.SetChannelAttribute ("Delay", StringValue ("2ms"));
	
	//criando um link ponto a ponto entre os nós 1 e 2
	NetDeviceContainer link1_2 = pointToPoint.Install(nodesLink1_2);
	
	//Instalando a pilha ip nos nós 1 e 2
	InternetStackHelper internet;
	internet.Install(nodesLink1_2);

	Ipv4AddressHelper ipv4;
	ipv4.SetBase("10.1.1.0", "255.255.255.0");
	Ipv4InterfaceContainer i = ipv4.Assign(link1_2);

	TypeId tid = TypeId::LookupByName ("ns3::UdpSocketFactory");
	
	Ptr<Socket> recvSink = Socket::CreateSocket(node2, tid);
	InetSocketAddress local = InetSocketAddress(
		Ipv4Address::GetAny(),
		80);
	recvSink->Bind (local);
	recvSink->SetRecvCallback(MakeCallback(&ReceivePacket));

	Ptr<Socket> source = Socket::CreateSocket(node1, tid);
	InetSocketAddress remote = InetSocketAddress(
		Ipv4Address::GetBroadcast(),
		80);
	source->SetAllowBroadcast(true);
	source->Connect(remote);


	Simulator::Schedule(Seconds(startTime), &GenerateTraffic, &source,
		PpacketSize,numPackets, Seconds(interval));

	Simulator::Stop (Seconds(finishTime));
	
	Simulator::Run();
	Simulator::Destroy ();
	return 0;
}
