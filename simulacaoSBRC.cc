#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include <iostream>

using namespace ns3;

NS_LOG_COMPONENT_DEFINE ("simulacaoSBRC");

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
