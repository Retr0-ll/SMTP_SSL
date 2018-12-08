#ifndef _SMTP_
#include"smtp.h"
#endif

#ifndef _IOSTREAM_
#include <iostream>
#endif

#define BUFFER_SIZE 1024*10 //×Ö½Ú

void ServerLogic(SmtpServer &svr);


int main()
{
	LoadSocket(2, 2);

	SmtpServer svr(BUFFER_SIZE);
	svr.Listen(25);
	svr.Start(&ServerLogic, svr);
	return 0;
}

void ServerLogic(SmtpServer &svr)
{
	//connection established ------- 220
	svr << RB220;

	//EHLO  --------- 250
	svr >> svr.buffer_;
	svr << RB250;

	//MAIL FROM ------- 250
	svr >> svr.buffer_;
	svr << RB250;

	//RCPT TO ------ 250
	svr >> svr.buffer_;
	svr << RB250;

	//DATA -------- 354
	svr >> svr.buffer_;
	svr << RB354;

	//SAVE MAIL DATA
	svr.SaveMailData();
	svr << RB250;

	//QUIT --Bye
	svr >> svr.buffer_;
	svr << RB221;

}