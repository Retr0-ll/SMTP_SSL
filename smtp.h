#pragma once

#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

#include<WinSock2.h>

class SmtpServer
{
private:
	SOCKET listen_socket_;
	const char *listen_addr_;
	unsigned short listen_port_;

	int buffer_size_;
	char *buffer_;

public:
	void Listen(unsigned short listen_port);
	void Start();
private:
public:
	SmtpServer(int buffer_size);
	~SmtpServer();
};

class SmtpClient
{
};
