#include "services/http/client.h"

#include "core/config/server.h"

#include <RCNET/RCNET.h>

std::unique_ptr<httplib::Client> g_httpClient = nullptr;

bool ServerHttp_InitializeClient(void)
{
    g_httpClient = std::make_unique<httplib::Client>(ServerConfig::baseUrlApi.data());

    g_httpClient->set_connection_timeout(0, 300000);
    g_httpClient->set_read_timeout(5, 0);
    g_httpClient->set_write_timeout(5, 0);
    g_httpClient->enable_server_certificate_verification(false);
    return true;
}

void ServerHttp_ShutdownClient(void)
{
    g_httpClient.reset();
}
