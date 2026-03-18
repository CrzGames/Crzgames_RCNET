#pragma once

#include <cstddef> // size_t
#include <string>  // std::string

struct ClientBootstrapSignUpAccount
{
    std::string username;
    std::string email;
    std::string password;
};

struct ClientBootstrapSignInAccount
{
    std::string email;
    std::string password;
};

// Nombre total de comptes disponibles pour le bootstrap dev.
size_t ClientBootstrap_GetAuthAccountCount();

// Recuperation des comptes signup/signin par index.
const ClientBootstrapSignUpAccount& ClientBootstrap_GetSignUpAccount(size_t index);
const ClientBootstrapSignInAccount& ClientBootstrap_GetSignInAccount(size_t index);

// Parse la ligne de commande et extrait un index de compte si present.
// Formats supportes:
// - --account-index 12
// - --account-index=12
// - --account 12
bool ClientBootstrap_TryGetAccountIndexFromCmdline(size_t& outIndex);
