#include "net.hpp"

Slave::Slave(io_context &ioctx, const std::string& client_id, const std::string& host)
	: mqcl(mqtt::make_client(ioctx, host, "6000"))
{
	mqcl->set_client_id(client_id);
	mqcl->set_clean_session(true);
}

void Slave::connect()
{
	mqcl->connect();
}

