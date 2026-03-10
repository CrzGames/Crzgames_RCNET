#pragma once

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>

#include <memory>


struct HttpClientState
{
    std::unique_ptr<httplib::Client> authApiClient;
};