#pragma once

#include "services/http/httplib_wrapper.h"

#include <memory> // std::unique_ptr

extern std::unique_ptr<httplib::Client> g_httpClient;

bool ServerHttp_InitializeClient(void);
void ServerHttp_ShutdownClient(void);