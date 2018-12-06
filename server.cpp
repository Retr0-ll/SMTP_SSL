#include "smtp.h"
#include <iostream>

#pragma comment (lib,"Ws2_32.lib")


SmtpServer::SmtpServer(int buffer_size) :listen_socket_(INVALID_SOCKET), buffer_size_(buffer_size), buffer_(NULL)
{
	//Initialize buffer
	buffer_ = new char[buffer_size_];
	if (buffer_ == NULL)
	{
		std::cout << "failed to new" << std::endl;
		WSACleanup();
		exit(4);
	}

	//Create SOCKET
	listen_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listen_socket_ == INVALID_SOCKET)
	{
		std::cout << "socket failed with error: " << WSAGetLastError() << std::endl;
		WSACleanup();
		exit(2);
	}

}

void SmtpServer::Listen(unsigned short listen_port)
{
	listen_addr_ = "127.0.0.1";
	listen_port_ = listen_port;

	//Initialize address and port
	sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));

	sin.sin_family = AF_INET;
	sin.sin_port = htons(listen_port_);
	sin.sin_addr.S_un.S_addr = inet_addr(listen_addr_);


	//Bind address and port
	if (bind(listen_socket_, (LPSOCKADDR)&sin, sizeof(sin)) == SOCKET_ERROR)
	{
		std::cout << "bind failed with error: " << WSAGetLastError() << std::endl;
		closesocket(listen_socket_);
		WSACleanup();
		exit(3);
	}

	//start to listen
	if (listen(listen_socket_, SOMAXCONN) == SOCKET_ERROR)
	{
		std::cout << "listen failed with error: " << WSAGetLastError() << std::endl;
		closesocket(listen_socket_);
		WSACleanup();
		exit(3);
	}
	std::cout << "Server listenning on " << inet_ntoa(sin.sin_addr)
		<< ":" << ntohs(sin.sin_port) << "......" << std::endl;

}

void SmtpServer::Start()
{

	SOCKET session_socket = INVALID_SOCKET;
	sockaddr_in remote_addr;
	int remote_addr_len = sizeof(remote_addr);
	memset(&remote_addr, 0, remote_addr_len);

	while (1)
	{
		session_socket = accept(listen_socket_, (SOCKADDR*)&remote_addr, &remote_addr_len);
		if (session_socket == INVALID_SOCKET)
		{
			std::cout << "accept failed with error: " << WSAGetLastError() << std::endl;
			closesocket(listen_socket_);
			WSACleanup();
			exit(5);
		}
		std::cout << "accepted a connection from " << inet_ntoa(remote_addr.sin_addr)
			<< ":" << ntohs(remote_addr.sin_port) << std::endl;
	}
}

SmtpServer::~SmtpServer()
{
	delete[]buffer_;
	closesocket(listen_socket_);
	WSACleanup();
}