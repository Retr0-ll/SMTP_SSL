#ifndef _SMTP_
#include"smtp.h"
#endif

#ifndef _IOSTREAM_
#include <iostream>
#endif

#ifndef _REGEX_
#include<regex>
#endif

#define BUFFER_SIZE 1024*10 //字节

int ServerLogic(SmtpServer &svr);

int ClientLogic(SmtpServer &svr);

char user_name[50];
char passwd[50];
char mailer[50];
char receiver[30][50];

int CheckCmd(SmtpServer&svr, const char* cmd,int cmd_len);

int main()
{
	LoadSocket(2, 2);

	SmtpServer svr(BUFFER_SIZE);
	svr.Listen(25);
	svr.Start(&ServerLogic, &ClientLogic, svr);
	return 0;
}

int ServerLogic(SmtpServer &svr)
{
	//建立连接服务器回复220,服务器状态初始化
	svr << RB220;
	svr.state_ = 0;
	svr.exstate_ = 0;

	//开始处理客户端命令
	for (svr >> svr.buffer_; svr.state_ != 11; svr >> svr.buffer_)
	{
		switch (svr.state_)
		{
		case-1://Invalid Command ---- 500 
			svr << RB500;
			svr.state_ = svr.exstate_;
			break;

		case 0://EHLO------250 extention surported
			if (CheckCmd(svr, EHLO, EHLO_L) == 0)
			{
				svr << RB250_EXT;
				svr.state_ = 1;
			}
			break;
		case 1://Auth login ---- 334 recive Username and passwd
			   //           ---- 235 Authentication successful
			if (CheckCmd(svr, AL, AL_L) == 0)
			{
				svr << RB334_USER;
				svr >> svr.buffer_;
				svr << RB334_PASS;
				svr >> svr.buffer_;
				svr << RB235;

				svr.state_ = 2;
			}
			break;
		case 2://Mail From ------ 250 OK
			if (CheckCmd(svr, MF, MF_L) == 0)
			{
				svr << RB250;
				svr.state_ = 3;
			}
			break;
		case 3://RCPT TO ------- 250 OK
			if (CheckCmd(svr, RT, RT_L) == 0)
			{
				svr << RB250;
				svr.state_ = 4;
			}
			break;
		case 4:// DATA --------- 354 Ready
			if (CheckCmd(svr, DATA, DATA_L) == 0)
			{
				svr << RB354;
				svr.SaveMailData();
				svr.state_ = 5;
			}
			break;
		case 5://QUIT ---------- 221 Bye
			if (CheckCmd(svr, QT, QT_L) == 0)
			{
				svr << RB221;
				return 0;
			}
		}
	}
		
	return 0;
}

int CheckCmd(SmtpServer&svr, const char* state_cmd, int cmd_len)
{
	char cmd[25];
	
	/*提取命令*/
	memcpy_s(cmd, 20, svr.buffer_, cmd_len);
	cmd[cmd_len] = '\0';

	/*判断合法性*/
	if (strcmp(state_cmd, cmd) != 0)
	{
		/*判断是否为QUIT*/
		if (strcmp(svr.buffer_, QT) == 0)
		{
			svr.state_ = 11;
		}
		svr.exstate_ = svr.state_;
		svr.state_ = -1;
		return 1;
	}

	/*命令合法*/
	return 0;
}

int ClientLogic(SmtpServer &svr)
{
	svr >> svr.buffer_;
	svr << EHLO;
	svr >> svr.buffer_;

	return 0;
}

