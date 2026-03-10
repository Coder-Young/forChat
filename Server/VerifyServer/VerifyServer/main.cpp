#include "VerifyServer.h"
#include "EmailService.h"

const std::string SMTP_SERVER = "smtp.163.com";
const int SMTP_PORT = 465;
const std::string SMTP_USER = "18362705353@163.com";
const std::string SMTP_AUTH_CODE = "RY3yWZGyY97dv7MC";
const std::string FROM_NAME = "forChat";

int main()
{
	EmailService::GetInstance()->init(SMTP_SERVER, SMTP_PORT, 
		SMTP_USER, SMTP_AUTH_CODE, FROM_NAME);
	VerifyServer s("tcp://127.0.0.1:8081");
	s.Run();
	return 0;
}