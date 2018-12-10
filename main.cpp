#ifndef _SMTP_
#include"smtp.h"
#endif

#ifndef _IOSTREAM_
#include <iostream>
#endif

#define BUFFER_SIZE 1024*10 //×Ö½Ú

int ServerLogic(SmtpServer &svr);

int ClientLogic(SmtpServer &svr);

int RsetQuit(SmtpServer &svr);

int state = 0;

char user_name[50];
char passwd[50];

char mailer[50];
char receiver[30][50];

int main()
{
	LoadSocket(2, 2);

	SmtpServer svr(BUFFER_SIZE);
	svr.Listen(25);
	svr.Start(&ServerLogic,&ClientLogic,svr);
	return 0;
}

int ServerLogic(SmtpServer &svr)
{
	int len;
	char mail_list[100] = ".\\Data\\";


	while (state != 11)
	{
		switch (state)
		{

		case 0:
			//connection established ------- 220
			svr << RB220;
			state = 1;
			break;
			
		case 1:
			//EHLO  --------- 250_EXT
			svr >> svr.buffer_;
			if (RsetQuit(svr) == 0) break;
			svr << RB250_EXT;
			state = 2;
			break;

		case 2:
			//Auth login ------- 334_User
			svr >> svr.buffer_;
			if (RsetQuit(svr) == 0) break;
			strcpy_s(user_name, GET_PARA(svr.buffer_, UN));

			svr << RB334_USER;
			state = 3;
			break;

		case 3:
			//user ***** -------- 334_pass
			svr >> svr.buffer_;
			if (RsetQuit(svr) == 0) break;
			strcpy_s(user_name, GET_PARA(svr.buffer_, UN));

			svr << RB334_PASS;
			state = 4;
			break;
		
		case 4:
			// pass ***** --------- 235
			svr >> svr.buffer_;
			if (RsetQuit(svr) == 0) break;
			svr << RB235;
			state = 5;
			break;

		case 5:
			//MAIL FROM ------- 250
			len = svr >> svr.buffer_;
			if (RsetQuit(svr) == 0) break;
			svr << RB250;

			svr.buffer_[len - 3] = '\0';

			strcpy_s(mailer, GET_PARA(svr.buffer_, MF) + 1);

			strcat_s(mail_list, GET_PARA(svr.buffer_, MF) + 1);

			strcat_s(mail_list, ".txt");

			state = 6;
			break;

		case 6:
			//RCPT TO ------ 250
			len = svr >> svr.buffer_;
			if (RsetQuit(svr) == 0 || RsetQuit(svr) == 1) break;
			svr << RB250;
			state = 7;
			break;

		case 7:
			//DATA -------- 354
			svr >> svr.buffer_;
			if (RsetQuit(svr) == 0 || RsetQuit(svr) == 1) break;
			svr << RB354;
			state = 8;
			break;
	
		case 8:
			//SAVE MAIL DATA
			svr.SaveMailData(mail_list);
			svr << RB250;
			state = 9;
			break;

		case 9:
			//QUIT --Bye
			svr >> svr.buffer_;
			if (RsetQuit(svr) == 1) break;
			svr << RB221;
			state = 11;
			break;

		case 10:
			svr << RB221;
			return 1;
		}
	}
		
	return 0;
}



int ClientLogic(SmtpServer &svr)
{
	svr >> svr.buffer_;
	svr << EHLO;
	svr >> svr.buffer_;

	return 0;
}

int RsetQuit(SmtpServer &svr)
{

	if (strcmp(svr.buffer_, RT)==0) 
	{
		state = 5;
		return 1;
	}

	if (strcmp(svr.buffer_, QT)==0)
	{
		state = 10;
		return 0;
	}

	return 2;
}

