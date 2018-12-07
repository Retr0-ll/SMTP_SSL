#ifndef _SMTP_
#include "smtp.h"
#endif

#ifndef _IOSTREAM_
#include <iostream>
#endif

#ifndef _CTIME_
#include <ctime>
#endif


#pragma comment (lib,"Ws2_32.lib")

void LoadSocket(int major_version, int minor_version)
{
	WSADATA wsadata;
	WORD socket_version = MAKEWORD(major_version, minor_version);
	int error;

	error = WSAStartup(socket_version, &wsadata);
	if (error)
	{
		std::cout << "WSAStartup failed with error: " << error << std::endl;
		exit(1);
	}
}


SmtpServer::SmtpServer(int buffer_size) :listen_socket_(INVALID_SOCKET), buffer_size_(buffer_size), buffer_(NULL)
{
	//Get log file's name
	time_t now_time;
	struct tm info;
	char log_fn[30];

	time(&now_time);
	localtime_s(&info, &now_time);
	
	strftime(log_fn, 30, "Log-%Y%m%d%H%M%S.txt", &info);

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
	sockaddr_in host_addr;
	int host_addr_len = sizeof(host_addr);
	memset(&host_addr, 0, host_addr_len);

	while (1)
	{
		session_socket = accept(listen_socket_, (SOCKADDR*)&host_addr, &host_addr_len);
		if (session_socket == INVALID_SOCKET)
		{
			std::cout << "accept failed with error: " << WSAGetLastError() << std::endl;
			closesocket(listen_socket_);
			WSACleanup();
			exit(5);
		}
		std::cout << "accepted a connection from " << inet_ntoa(host_addr.sin_addr)
			<< ":" << ntohs(host_addr.sin_port) << std::endl;

		std::cout << "reply:  " << RB220 << std::endl;
		send(session_socket, RB220, strlen(RB220), 0);

		HandleCmd(session_socket);
	}
}

int SmtpServer::HandleCmd(SOCKET session_socket)
{
	int data_len;
	const char *pr = NULL;

	//EHLO ----  250 OK
	data_len = recv(session_socket, buffer_, buffer_size_, 0);
	buffer_[data_len] = '\0';
	std::cout << "receive:  " << buffer_;

	pr = RB250;
	std::cout << "reply:  " << pr;
	send(session_socket, pr, strlen(pr), 0);

	//MAIL FROM ------250 OK
	data_len = recv(session_socket, buffer_, buffer_size_, 0);
	buffer_[data_len] = '\0';
	std::cout << "receive:  " << buffer_;

	pr = RB250;
	std::cout << "reply:  " << pr;
	send(session_socket, pr, strlen(pr), 0);

	//RCPT TO -----250 OK
	data_len = recv(session_socket, buffer_, buffer_size_, 0);
	buffer_[data_len] = '\0';
	std::cout << "receive:  " << buffer_;

	pr = RB250;
	std::cout << "reply:  " << pr;
	send(session_socket, pr, strlen(pr), 0);

	//DATA ----- 354 End data with <CR><LF>.<CR><LF>
	data_len = recv(session_socket, buffer_, buffer_size_, 0);
	buffer_[data_len] = '\0';
	std::cout << "receive:  " << buffer_;

	pr = RB354;
	std::cout << "reply:  " << pr;
	send(session_socket, pr, strlen(pr), 0);

	//Receive data and Wati for <CR><LF>.<CR><LF> ---------- 250 OK
	while (true)
	{
		std::cout << "receiving data........." << std::endl;
		data_len = recv(session_socket, buffer_, buffer_size_, 0);
		buffer_[data_len] = '\0';

		//Check End Point And Break;
		if (strcmp(CHECK_END_POINT(buffer_, data_len), END_OF_DATA) == 0)
		{
			std::cout << "receive:  " << buffer_;
			std::cout << "finished" << std::endl;
			break;
		}

		std::cout << "receive:  " << buffer_;
	}

	pr = RB250;
	std::cout << "reply:  " << pr;
	send(session_socket, pr, strlen(pr), 0);

	//QUIT ----- 221 Bye
	data_len = recv(session_socket, buffer_, buffer_size_, 0);
	buffer_[data_len] = '\0';
	std::cout << "receive:  " << buffer_;

	pr = RB221;
	std::cout << "reply:  " << pr;
	send(session_socket, pr, strlen(pr), 0);
	return 0;
}

SmtpServer::~SmtpServer()
{
	delete[]buffer_;
	closesocket(listen_socket_);
	WSACleanup();
}

/*
SmtpClient::SmtpClient(int buffer_size) :remote_socket_(INVALID_SOCKET), buffer_size_(buffer_size), buffer_(NULL)
{
	//Initialize buffer
	buffer_ = new char[buffer_size_];
	if (buffer_ == NULL)
	{
		std::cout << "failed to new" << std::endl;
		WSACleanup();
		exit(4);
	}

	remote_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (remote_socket_ == INVALID_SOCKET)
	{
		std::cout << "socket failed with error: " << WSAGetLastError() << std::endl;
		WSACleanup();
		exit(2);
	}
}*/
