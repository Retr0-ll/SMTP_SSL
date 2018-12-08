#ifndef _SMTP_
#include "smtp.h"
#endif

#ifndef _IOSTREAM_
#include <iostream>
#endif

#ifndef _CTIME_
#include <ctime>
#endif

#ifndef _FSTREAM_
#include <fstream>
#endif

#pragma comment (lib,"Ws2_32.lib")


void GetTimeStamp(char *output, const char * format)
{
	time_t now_time;
	struct tm info;

	//获取当前时间戳 换算为当地地址 转化成tm 结构
	time(&now_time);
	localtime_s(&info, &now_time);

	//按照指定格式输出到 传入缓冲中
	strftime(output, LOG_T_MAXLEN - 1, format, &info);
}


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


SmtpServer& operator<<(SmtpServer& server, const char *data_send)
{
	//发送数据
	send(server.session_socket_, data_send, strlen(data_send), 0);

	//记录日志，输出到标准输出
	GetTimeStamp(server.log_time_buffer_, LOG_T_F);
	server.log_file_ << server.log_time_buffer_ << "reply:  " << data_send;
	std::cout << "reply:  " << data_send;

	return server;
}


int operator>>(SmtpServer& server, char *data_receive)
{
	//接收数据，如果没有数据则阻塞挂起
	int data_len = 0;
	data_len = recv(server.session_socket_, data_receive, server.buffer_size_, NULL);
	GetTimeStamp(server.log_time_buffer_, LOG_T_F);

	//记录日志，输出到标准输出
	data_receive[data_len] = '\0';
	server.log_file_ << server.log_time_buffer_ << "receive:  " << data_receive;
	std::cout << "receive:  " << data_receive;
	
	return data_len;
}


SmtpServer::SmtpServer(int buffer_size) :listen_socket_(INVALID_SOCKET), buffer_size_(buffer_size), buffer_(NULL)
{
	//通过样式 LOG_FN_F 获取LOG文件名
	char log_fn[30];
	GetTimeStamp(log_fn, LOG_FN_F);

	//打开Log文件
	log_file_.open(log_fn);

	//获取socket 地址族ipv4 流式SOCKET 协议TCP
	listen_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listen_socket_ == INVALID_SOCKET)
	{
		GetTimeStamp(log_time_buffer_, LOG_T_F);
		log_file_ << log_time_buffer_ << "socket failed with error : "
			<< WSAGetLastError() << std::endl;

		WSACleanup();
		exit(2);
	}

	//申请缓冲内存
	buffer_ = new char[buffer_size_];
	if (buffer_ == NULL)
	{
		GetTimeStamp(log_time_buffer_, LOG_T_F);
		log_file_ << log_time_buffer_ << "failed to new a " << buffer_
			<< "bytes buffer" << std::endl;

		WSACleanup();
		exit(4);
	}

}


void SmtpServer::Listen(unsigned short listen_port)
{
	//设置地址和端口
	listen_addr_ = "127.0.0.1";
	listen_port_ = listen_port;

	//服务器端口地址初始化
	sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	
	//地址族 ipv4 
	sin.sin_family = AF_INET;
	sin.sin_port = htons(listen_port_);
	sin.sin_addr.S_un.S_addr = inet_addr(listen_addr_);


	//绑定端口和地址
	if (bind(listen_socket_, (LPSOCKADDR)&sin, sizeof(sin)) == SOCKET_ERROR)
	{
		GetTimeStamp(log_time_buffer_, LOG_T_F);
		log_file_ << log_time_buffer_ << "bind failed with error: "
			<< WSAGetLastError() << std::endl;

		closesocket(listen_socket_);
		WSACleanup();
		exit(3);
	}

	//开始监听端口
	if (listen(listen_socket_, SOMAXCONN) == SOCKET_ERROR)
	{
		GetTimeStamp(log_time_buffer_, LOG_T_F);
		log_file_ << log_time_buffer_ << "listen failed with error: "
			<< WSAGetLastError() << std::endl;

		closesocket(listen_socket_);
		WSACleanup();
		exit(3);
	}

	GetTimeStamp(log_time_buffer_, LOG_T_F);
	log_file_<< log_time_buffer_<< "Server listenning on " << inet_ntoa(sin.sin_addr)
		<< ":" << ntohs(sin.sin_port) << "......" << std::endl;

	std::cout << "Server listenning on " << inet_ntoa(sin.sin_addr)
		<< ":" << ntohs(sin.sin_port) << "......" << std::endl;

}


void SmtpServer::Start(CallBack callback, SmtpServer& svr)
{
	//客户端地址初始化
	sockaddr_in host_addr;
	int host_addr_len = sizeof(host_addr);
	memset(&host_addr, 0, host_addr_len);

	while (1)
	{
		session_socket_ = INVALID_SOCKET;

		//接收连接，如果没有连接则阻塞挂起
		session_socket_ = accept(listen_socket_, (SOCKADDR*)&host_addr, &host_addr_len);
		GetTimeStamp(log_time_buffer_, LOG_T_F);

		if (session_socket_ == INVALID_SOCKET)
		{
			log_file_ << GetTimeStamp << "accept failed with error: "
				<< WSAGetLastError() << std::endl;

			closesocket(listen_socket_);
			WSACleanup();
			exit(5);
		}

		log_file_ << log_time_buffer_ << "accepted a connection from " << inet_ntoa(host_addr.sin_addr)
			<< ":" << ntohs(host_addr.sin_port) << std::endl;
		std::cout << "accepted a connection from " << inet_ntoa(host_addr.sin_addr)
			<< ":" << ntohs(host_addr.sin_port) << std::endl;


		//然后调用回调函数开始 SMTP逻辑
		callback(svr);
	}
}


void SmtpServer::SaveMailData()
{
	int data_len = 0;
	int data_count = 0;
	data_file_.open(".\\Data\\mail_data.txt", std::ios::ate);

	while (true)
	{
		data_len = recv(session_socket_, buffer_, buffer_size_, 0);
		data_count += data_len;
		buffer_[data_len] = '\0';

		GetTimeStamp(log_time_buffer_, LOG_T_F);
		log_file_ << log_time_buffer_ << "receiving data........." << data_len <<std::endl;
		std::cout << "receiving data........." << data_len << std::endl;


		//检查数据结束标志
		if (strcmp(CHECK_DATA_END(buffer_, data_len), END_OF_DATA) == 0)
		{
			data_file_ << buffer_;
			std::cout << "receive:  " << buffer_;

			GetTimeStamp(log_time_buffer_, LOG_T_F);
			log_file_ << log_time_buffer_ << "finished  ....." << data_count << "bytes" << std::endl;

			std::cout << "finished" << std::endl;
			break;
		}
		data_file_ << buffer_;
		std::cout << "receive:  " << buffer_;
	}

	return;
}


SmtpServer::~SmtpServer()
{
	delete[]buffer_;
	log_file_.close();
	data_file_.close();
	closesocket(listen_socket_);
	WSACleanup();
}