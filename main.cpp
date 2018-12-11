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

char user_name[100];
char passwd[100];

char mailer[100];
char mail_list[100] = ".\\Data\\";
char receiver[30][50];
int rcv_num;

//邮件地址正则匹配格式
std::regex mail_format("<([0-9A-Za-z\\-_\\.]+)@([0-9a-z]+\\.[a-z]{2,3}(\\.[a-z]{2})?)>\r\n");

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

	int len = 0;

	//开始处理客户端命令
	for (svr >> svr.buffer_,rcv_num=0; svr.state_ != -2;)
	{

		switch (svr.state_)
		{
		case 0://EHLO------250 extention surported
			if (CheckCmd(svr, EHLO_C, EHLO_L) == 0)
			{
				svr << RB250_EXT;
				svr.state_ = 1;
				break;
			}

		case 1://Auth login ---- 334 recive Username and passwd	---- 235 Authentication successful
			if (CheckCmd(svr, AL, AL_L) == 0)
			{
				svr << RB334_USER;
				//保存用户名
				len = svr >> svr.buffer_;
				memcpy_s(user_name, 100, svr.buffer_, len + 1);

				svr << RB334_PASS;
				//保存密码
				len = svr >> svr.buffer_;
				memcpy_s(passwd, 100, svr.buffer_, len + 1);
				svr << RB235;

				svr.state_ = 2;
				break;
			}

		case 2://Mail From ------ 250 OK
			if (CheckCmd(svr, MF_C, MF_L) == 0)
			{
				//检查邮件地址格式
				if (!std::regex_match(GET_PARA(svr.buffer_, MF_C), mail_format))
				{
					svr << RB550;
					break;
				}

				//保存Mail From 构造用户对应的邮件列表名称
				memcpy_s(mailer, 100, svr.buffer_, len + 1);
				memcpy_s(mail_list+7, 100, GET_PARA(svr.buffer_, MF_C) + 1, len - strlen(MF_C) -4 );
				(mail_list+7)[len - strlen(MF_C) - 4] = '\0';
				strcat_s(mail_list, ".txt");

				svr << RB250;
				svr.state_ = 3;

				break;
			}

		case 3://RCPT TO ------- 250 OK
			if (CheckCmd(svr, RT_C, RT_L) == 0)
			{
				//检查邮件地址格式
				if (!std::regex_match(GET_PARA(svr.buffer_, RT_C), mail_format))
				{
					svr << RB550;
					break;
				}

				//保存RCPT TO
				memcpy_s(receiver[rcv_num], 100, svr.buffer_, len + 1);
				rcv_num++;

				svr << RB250;
				svr.state_ = 4;

				break;
			}

		case 4:// DATA --------- 354 Ready
			//判断是否继续是RCPT 如果是 则继续回到case 3
			if (CheckCmd(svr, RT_C, RT_L) == 0)
			{
				svr.state_ = 3;
				continue;
			}
			if (CheckCmd(svr, DATA, DATA_L) == 0)
			{
				svr << RB354;
				if (svr.SaveMailData(mail_list) == 0)
				{
					svr << RB250;
					svr.exstate_ = 4;
					svr.state_ = 5;
					break;
				}
			}

		case 5://QUIT ---------- 221 Bye
			if (CheckCmd(svr, QT, QT_L) != 0)
			{
				break;
			}
			svr << RB221;
			if (svr.exstate_ == 4)
			{
				return 0;
			}
			else
			{
				return 1;
			}

		case -1://Invalid Command ---- 500 
			svr << RB500;
			svr.state_ = svr.exstate_;
			break;
		}

		len = svr >> svr.buffer_;
	}
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
			svr.state_ = 5;
			return 1;
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

	svr << AL;
	svr >> svr.buffer_;

	svr << user_name;
	svr >> svr.buffer_;

	svr << passwd;
	svr >> svr.buffer_;

	svr << mailer;
	svr >> svr.buffer_;

	for (int i = 0; i < rcv_num; i++)
	{
		svr << receiver[i];
		svr >> svr.buffer_;
	}

	svr << DATA;
	svr >> svr.buffer_;

	svr.ReadMailData(mail_list);
	svr << svr.buffer_;
	svr >> svr.buffer_;

	svr << QT;
	svr >> svr.buffer_;


	return 0;
}