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
	int len;
	//connection established ------- 220
	svr << RB220;

	//EHLO  --------- 250_EXT
	svr >> svr.buffer_;
	svr << RB250_EXT;

	//Auth login ------- 334_User
	svr >> svr.buffer_;
	svr << RB334_USER;

	//user ***** -------- 334_pass
	svr >> svr.buffer_;
	svr << RB334_PASS;

	// pass ***** --------- 235
	svr >> svr.buffer_;
	svr << RB235;

	//MAIL FROM ------- 250
	len = svr >> svr.buffer_;
	svr << RB250;

	char mail_list[50] = ".\\Data\\";
	char mailer[50];
	svr.buffer_[len - 3] = '\0';
	strcpy_s(mailer, GET_PARA(svr.buffer_, MF) + 1);

	strcat_s(mail_list, GET_PARA(svr.buffer_,MF)+1);
	strcat_s(mail_list, ".txt");

	//RCPT TO ------ 250
	len = svr >> svr.buffer_;
	svr << RB250;

	char receiver[50];
	svr.buffer_[len - 3] = '\0';
	strcpy_s(receiver, GET_PARA(svr.buffer_, RT));

	//DATA -------- 354
	svr >> svr.buffer_;
	svr << RB354;


	//SAVE MAIL DATA
	svr.SaveMailData(mail_list);
	svr << RB250;


	//QUIT --Bye
	svr >> svr.buffer_;
	svr << RB221;

}