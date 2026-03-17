#include "simulation/process/http/auth_signupresponse_message.h"

#include <RC2D/RC2D.h>

#include <mutex>

void ClientSimulation_ProcessHttpDispatcher_HandleAuthSignUpResponseMessage(
    NetworkState& networkState,
    SimulationToNetworkOUTQueue& simToNetQueue,
    const HttpToSimulationMessage& httpToSimMessage)
{
    bool secureSessionEstablished = false;
    bool authValidated = false;
    bool authTokenValidated = false;
    {
        std::lock_guard<std::mutex> lock(networkState.cryptoMutex);
        secureSessionEstablished = networkState.secureSessionEstablished;
        authValidated = networkState.authValidated;
        authTokenValidated = networkState.authTokenValidated;
    }

    size_t pendingOutMessages = 0;
    {
        std::lock_guard<std::mutex> lock(simToNetQueue.mtx);
        pendingOutMessages = simToNetQueue.q.size();
    }

    RC2D_log(
        RC2D_LOG_INFO,
        "[CLIENT] [SIMULATION] [HTTP] AUTH_SIGNUP_RESPONSE success=%u msg=%s (secure=%u authHandled=%u authTokenOk=%u pendingOut=%llu)",
        httpToSimMessage.authSignUpResponse.success ? 1u : 0u,
        httpToSimMessage.authSignUpResponse.message.c_str(),
        secureSessionEstablished ? 1u : 0u,
        authValidated ? 1u : 0u,
        authTokenValidated ? 1u : 0u,
        static_cast<unsigned long long>(pendingOutMessages));
}

