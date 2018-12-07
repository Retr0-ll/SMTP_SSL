#pragma once

#ifndef _SMTP_
#define SMTP
#endif

#ifndef _FSTREAM_
#include <fstream>
#endif

#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif


#define RB220 "220 localhost\r\n"
#define RB250 "250 OK\r\n"
#define RB354 "354 End data with <CR><LF>.<CR><LF>\r\n"
#define RB221 "221 Bye\r\n"

#define RB334_USER "334 dXNlcm5hbWU6"
#define RB334_PWD "334 UGFzc3dvcmQ6"

#define CHECK_END_POINT(buffer,data_len) (buffer+data_len-5)
#define END_OF_DATA "\r\n.\r\n"

#include<WinSock2.h>


void LoadSocket(int major_version, int minor_version);

class SmtpServer
{
private:
	SOCKET listen_socket_;
	const char *listen_addr_;
	unsigned short listen_port_;

	int buffer_size_;
	char* buffer_;

	std::fstream log_file_;

public:
	void Listen(unsigned short listen_port);
	void Start();
private:
	int HandleCmd(SOCKET session_socket);
	/*void HandleData(SOCKET session_socket)*/
public:
	SmtpServer(int buffer_size);
	~SmtpServer();
};

/*class SmtpClient
{
private:
	SOCKET remote_socket_;
	struct sockaddr_in remote_addr_;

	int buffer_size_;
	char* buffer_;

public:
	void Connect(struct sockaddr_in remote_addr_);
public:
	SmtpClient(int buffer_size);
	~SmtpClient();
};*/