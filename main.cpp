#include"smtp.h"
#include<iostream>

int main()
{
	WSADATA wsadata;
	WORD socket_version = MAKEWORD(2, 2);
	int error;

	error = WSAStartup(socket_version, &wsadata);
	if (error)
	{
		std::cout << "WSAStartup failed with error: " << error << std::endl;
		exit(1);
	}

	SmtpServer svr(5);
	svr.Listen(1089);
	svr.Start();
}