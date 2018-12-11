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
		std::cout << "ERROR wsastartup failed with error: " << error << std::endl;
		exit(1);
	}
}


SmtpServer& operator<<(SmtpServer& server, const char *data_send)
{
	//发送数据
	send(server.session_socket_, data_send, strlen(data_send), 0);

	//记录日志，输出到标准输出
	GetTimeStamp(server.log_time_buffer_, LOG_T_F);
	server.log_file_ << server.log_time_buffer_ << "INFO send:  " << data_send;
	std::cout << "INFO send:  " << data_send;

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
	server.log_file_ << server.log_time_buffer_ << "INFO receive:  " << data_receive;
	std::cout << "INFO receive:  " << data_receive;
	
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
		log_file_ << log_time_buffer_ << "ERROR socket failed with error : "
			<< WSAGetLastError() << std::endl;

		WSACleanup();
		exit(2);
	}

	//申请缓冲内存
	buffer_ = new char[buffer_size_];
	if (buffer_ == NULL)
	{
		GetTimeStamp(log_time_buffer_, LOG_T_F);
		log_file_ << log_time_buffer_ << "ERROR failed to new a " << buffer_
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
	sockaddr_in svr_adr;
	memset(&svr_adr, 0, sizeof(svr_adr));
	
	//地址族 ipv4 
	svr_adr.sin_family = AF_INET;
	svr_adr.sin_port = htons(listen_port_);
	svr_adr.sin_addr.S_un.S_addr = inet_addr(listen_addr_);


	//绑定端口和地址
	if (bind(listen_socket_, (LPSOCKADDR)&svr_adr, sizeof(svr_adr)) == SOCKET_ERROR)
	{
		GetTimeStamp(log_time_buffer_, LOG_T_F);
		log_file_ << log_time_buffer_ << "ERROR bind failed with error: "
			<< WSAGetLastError() << std::endl;

		closesocket(listen_socket_);
		WSACleanup();
		exit(3);
	}

	//开始监听端口
	if (listen(listen_socket_, SOMAXCONN) == SOCKET_ERROR)
	{
		GetTimeStamp(log_time_buffer_, LOG_T_F);
		log_file_ << log_time_buffer_ << "ERROR listen failed with error: "
			<< WSAGetLastError() << std::endl;

		closesocket(listen_socket_);
		WSACleanup();
		exit(3);
	}

	GetTimeStamp(log_time_buffer_, LOG_T_F);
	log_file_<< log_time_buffer_<< "INFO server listenning on " << listen_addr_
		<< ":" << listen_port_ << "......" << std::endl;

	std::cout << "INFO server listenning on " << listen_addr_
		<< ":" << listen_port_ << "......" << std::endl;

}


void SmtpServer::Start(CallBack server_logic, CallBack client_logic, SmtpServer& svr)
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
			log_file_ << GetTimeStamp << "WARNING accept failed with error: "
				<< WSAGetLastError() << std::endl;

			continue;
		}

		log_file_ << log_time_buffer_ << "INFO accepted a connection from " << inet_ntoa(host_addr.sin_addr)
			<< ":" << ntohs(host_addr.sin_port) << std::endl;
		std::cout << "INFO accepted a connection from " << inet_ntoa(host_addr.sin_addr)
			<< ":" << ntohs(host_addr.sin_port) << std::endl;


		//然后调用回调函数开始SMTP SERVER逻辑
		if (server_logic(svr) == 0)
		{
			closesocket(session_socket_);
			log_file_ << log_time_buffer_ << "INFO mail receive succeed" << std::endl;
			std::cout << "INFO mail receive success" << std::endl;
			//连接远程服务器  调用回调函数 开始SMTP Client逻辑
			if (ConnectRemote() == 0)
			{
				if (client_logic(svr) == 0)
				{
					closesocket(session_socket_);
					log_file_ << log_time_buffer_ << "INFO mail send succeed" << std::endl;
					std::cout << "INFO mail send succeed" << std::endl;
				}
				else
				{
					closesocket(session_socket_);
					log_file_ << log_time_buffer_ << "WARNING mail send succeed" << std::endl;
					std::cout << "INFO mail send succeed" << std::endl;
				}
			}
			closesocket(session_socket_);
		}
		else
		{
			closesocket(session_socket_);

			GetTimeStamp(log_time_buffer_, LOG_T_F);
			log_file_ << log_time_buffer_ << "WARNING mail receive failed" << std::endl;
			std::cout << "WARNING mail receive failed" << std::endl;
		}
	}
}


int SmtpServer::SaveMailData(char *mail_list)
{
	int data_len = 0;
	int data_count = 0;
	data_file_.open(mail_list,std::ios::app);
	data_file_ << END_OF_DATA;

	//标记邮件起点
	data_file_.width(8);
	data_file_ << 0 << std::endl;

	while (true)
	{
		data_len = recv(session_socket_, buffer_, buffer_size_, 0);

		//如果意外断开连接
		if (data_len == -1)
		{
			GetTimeStamp(log_time_buffer_, LOG_T_F);
			log_file_ << log_time_buffer_ << "WARRING disconnected from the client" << std::endl;
			std::cout<< "WARRING disconnected from the client" << std::endl;

			return 1;
		}
		data_count += data_len;
		buffer_[data_len] = '\0';

		GetTimeStamp(log_time_buffer_, LOG_T_F);
		log_file_ << log_time_buffer_ << "INFO receiving data......... " << data_len <<" bytes"<<std::endl;
		std::cout << "INFO receiving data......... " << data_len << " bytes" << std::endl;
		
		//写入文件
		data_file_ << buffer_;
		std::cout << "INFO receive:  " << buffer_;

		//检查数据结束标志
		if (strcmp(CHECK_DATA_END(buffer_, data_len), END_OF_DATA) == 0)
		{
			//标记邮件结束点
			data_file_.width(8);
			data_file_ << data_len << std::endl;
			data_file_.close();
			

			GetTimeStamp(log_time_buffer_, LOG_T_F);
			log_file_ << log_time_buffer_ << "INFO finished  ..... total: " << data_count << " bytes" << std::endl;

			std::cout << "INFO finished  ..... total: " << data_count << " bytes" << std::endl;
			break;
		}
	}

	return 0;
}


int SmtpServer::ReadMailData(char *mail_list)
{
	int mail_size;
	int offset;
	data_file_.open(mail_list, std::ios::in);

	//获取邮件大小
	data_file_.seekg(-10, std::ios::end);
	data_file_ >> mail_size;

	//定位到邮件起点
	char cmp[6];
	cmp[5] = '\0';
	for (offset = -mail_size;; offset--)
	{
		data_file_.seekg(offset, std::ios::end);
		data_file_.read(cmp, 5);
		if (strcmp(cmp, END_OF_DATA) == 0)
		{
			offset += 17;
			break;
		}
	}

	//读取邮件内容
	data_file_.seekg(offset, std::ios::end);
	data_file_.read(buffer_, mail_size);
	buffer_[mail_size] = '\0';

	data_file_.close();
	return 0;
}


int SmtpServer::ConnectRemote()
{
	session_socket_ = INVALID_SOCKET;
	remote_addr_ = "220.181.12.17";
	remote_port_ = 25;

	session_socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (session_socket_ == INVALID_SOCKET)
	{
		GetTimeStamp(log_time_buffer_, LOG_T_F);
		log_file_ << log_time_buffer_ << "ERROR session socket failed with error : "
			<< WSAGetLastError() << std::endl;

		WSACleanup();
		exit(2);
	}

	//初始化远程地址
	sockaddr_in remote_adr;
	memset(&remote_adr, 0, sizeof(remote_adr));

	remote_adr.sin_family = AF_INET;
	remote_adr.sin_port = htons(remote_port_);
	remote_adr.sin_addr.S_un.S_addr = inet_addr(remote_addr_);

	//连接远程SMTP服务器
	if (connect(session_socket_, (LPSOCKADDR)&remote_adr, sizeof(remote_adr)) == SOCKET_ERROR)
	{
		GetTimeStamp(log_time_buffer_, LOG_T_F);
		log_file_ << log_time_buffer_ << "ERROR connect failed with error: "
			<< WSAGetLastError() << std::endl;

		closesocket(listen_socket_);
		closesocket(session_socket_);
		WSACleanup();
		exit(3);
	}
	if (session_socket_ == INVALID_SOCKET)
	{
		GetTimeStamp(log_time_buffer_, LOG_T_F);
		log_file_ << log_time_buffer_ << "WARNING unable to connect to the remote: " << std::endl;

		return 1;
	}

	//连接成功
	GetTimeStamp(log_time_buffer_, LOG_T_F);
	log_file_ << log_time_buffer_ << "INFO server connected to " << remote_addr_
		<< ":" << remote_port_ << "......" << std::endl;

	std::cout << "INFO server connected to " << remote_addr_
		<< ":" << remote_port_ << "......" << std::endl;

	return 0;

}


SmtpServer::~SmtpServer()
{
	delete[]buffer_;
	log_file_.close();
	closesocket(listen_socket_);
	WSACleanup();
}