#include "core/bootstrap/auth_dev_accounts.h"

#include <RC2D/RC2D_cmdline.h>

#include <array>   // std::array
#include <cstdlib> // std::strtoull
#include <cstring> // std::strcmp, std::strncmp
#include <limits>  // std::numeric_limits
#include <string>  // std::string

namespace
{
static constexpr size_t kClientBootstrapAuthAccountCount = 4000;
static constexpr size_t kClientBootstrapAccountSuffixWidth = 4;
static constexpr char kClientBootstrapDomain[] = "@orangexxx.fr";
static constexpr char kClientBootstrapPassword[] = "toto35000!xx";

static std::string ClientBootstrap_BuildFixedWidthDecimal(size_t value, size_t width)
{
    std::string text = std::to_string(value);
    if (text.size() < width)
    {
        text.insert(text.begin(), width - text.size(), '0');
    }
    return text;
}

static std::array<ClientBootstrapSignUpAccount, kClientBootstrapAuthAccountCount> ClientBootstrap_BuildSignUpAccounts()
{
    std::array<ClientBootstrapSignUpAccount, kClientBootstrapAuthAccountCount> out{};

    for (size_t i = 0; i < out.size(); ++i)
    {
        const std::string suffix = ClientBootstrap_BuildFixedWidthDecimal(i, kClientBootstrapAccountSuffixWidth);
        out[i].username = "coco" + suffix;
        out[i].email = "coco" + suffix + kClientBootstrapDomain;
        out[i].password = kClientBootstrapPassword;
    }

    return out;
}

static std::array<ClientBootstrapSignInAccount, kClientBootstrapAuthAccountCount> ClientBootstrap_BuildSignInAccounts()
{
    std::array<ClientBootstrapSignInAccount, kClientBootstrapAuthAccountCount> out{};

    for (size_t i = 0; i < out.size(); ++i)
    {
        const std::string suffix = ClientBootstrap_BuildFixedWidthDecimal(i, kClientBootstrapAccountSuffixWidth);
        out[i].email = "coco" + suffix + kClientBootstrapDomain;
        out[i].password = kClientBootstrapPassword;
    }

    return out;
}

static const std::array<ClientBootstrapSignUpAccount, kClientBootstrapAuthAccountCount> gSignUpAccounts =
    ClientBootstrap_BuildSignUpAccounts();

static const std::array<ClientBootstrapSignInAccount, kClientBootstrapAuthAccountCount> gSignInAccounts =
    ClientBootstrap_BuildSignInAccounts();

static bool ClientBootstrap_TryParseUnsignedIndex(const char* text, size_t& outValue)
{
    if (text == nullptr || text[0] == '\0')
    {
        return false;
    }

    char* endPtr = nullptr;
    const unsigned long long parsed = std::strtoull(text, &endPtr, 10);
    if (endPtr == text || *endPtr != '\0')
    {
        return false;
    }

    if (parsed > static_cast<unsigned long long>((std::numeric_limits<size_t>::max)()))
    {
        return false;
    }

    outValue = static_cast<size_t>(parsed);
    return true;
}
} // namespace

size_t ClientBootstrap_GetAuthAccountCount()
{
    return gSignInAccounts.size();
}

const ClientBootstrapSignUpAccount& ClientBootstrap_GetSignUpAccount(size_t index)
{
    const size_t accountCount = ClientBootstrap_GetAuthAccountCount();
    return gSignUpAccounts[index % accountCount];
}

const ClientBootstrapSignInAccount& ClientBootstrap_GetSignInAccount(size_t index)
{
    const size_t accountCount = ClientBootstrap_GetAuthAccountCount();
    return gSignInAccounts[index % accountCount];
}

bool ClientBootstrap_TryGetAccountIndexFromCmdline(size_t& outIndex)
{
    const int argc = rc2d_cmdline_getArgc();
    if (argc <= 1)
    {
        return false;
    }

    for (int i = 1; i < argc; ++i)
    {
        const char* arg = rc2d_cmdline_getArgv(i);
        if (arg == nullptr)
        {
            continue;
        }

        if ((std::strcmp(arg, "--account-index") == 0 || std::strcmp(arg, "--account") == 0) && i + 1 < argc)
        {
            size_t parsed = 0;
            if (ClientBootstrap_TryParseUnsignedIndex(rc2d_cmdline_getArgv(i + 1), parsed))
            {
                outIndex = parsed;
                return true;
            }
        }

        static constexpr char kAccountIndexPrefix[] = "--account-index=";
        if (std::strncmp(arg, kAccountIndexPrefix, sizeof(kAccountIndexPrefix) - 1) == 0)
        {
            size_t parsed = 0;
            if (ClientBootstrap_TryParseUnsignedIndex(
                    arg + (sizeof(kAccountIndexPrefix) - 1),
                    parsed))
            {
                outIndex = parsed;
                return true;
            }
        }
    }

    return false;
}
