#include "simulation/process/http/auth_validate_token_response_message.h"

#include "auth/types.h"
#include "network/packets/server/reliable.h"
#include "network/serialization/serialize_packets_server.h"

#include <unordered_map>

#include <RCNET/RCNET.h>

void ServerSimulationUpdate_ProcessHttpMessages_HandleAuthValidateTokenResponse(
    NetworkState& networkState,
    SimulationToNetworkOUTQueue& simToNetQueue,
    const HttpToSimulationMessage& httpMessage)
{
    // Rechercher la session associée à cette réponse HTTP.
    std::unordered_map<uint32_t, ClientSession>::iterator sit =
        networkState.sessions.find(httpMessage.connectionId);

    // Si la session n'existe pas, on ne peut pas rattacher la réponse.
    if (sit == networkState.sessions.end())
    {
        // Log d'avertissement indiquant une réponse orpheline.
        RCNET_log(RCNET_LOG_WARN,
                  "[SERVER] [SIMULATION] [AUTH] - Received auth validation response for unknown connectionId=%u\n",
                  httpMessage.connectionId);

        // Abandon du traitement.
        return;
    }

    // Référence directe vers la session concernée.
    ClientSession& session = sit->second;

    // Préparer le packet de réponse serveur -> client.
    ServerAuthResponsePacketReliable authResponsePacket{};

    // Renseigner le type réseau du packet.
    authResponsePacket.header.type =
        ServerReliablePacketType::SERVER_AUTH_RESPONSE_PACKET_RELIABLE;

    // Vérifier si le backend a validé le token.
    if (httpMessage.authTokenVerificationResponse.isValid)
    {
        // Marquer la session comme authentifiée.
        session.authStatus = AuthStatus::Valid;

        // Sauvegarder l'identifiant de compte de base de données.
        session.accountIdDatabase = httpMessage.authTokenVerificationResponse.accountIdDatabase;

        // Sauvegarder le username de base de données.
        session.accountUsernameDatabase = httpMessage.authTokenVerificationResponse.accountUsernameDatabase;

        // Nettoyer un éventuel message d'erreur précédent.
        session.authErrorMessage.clear();

        // Indiquer le succès dans le packet de réponse.
        authResponsePacket.status = ServerAuthResponseStatus::SUCCESS;

        // Log de succès d'authentification.
        RCNET_log(RCNET_LOG_INFO,
                  "[SERVER] [SIMULATION] [AUTH] - connectionId=%u authenticated successfully with accountId=%llu username=%s\n",
                  httpMessage.connectionId,
                  (unsigned long long)session.accountIdDatabase,
                  session.accountUsernameDatabase.c_str());
    }
    else
    {
        // Marquer la session comme authentification invalide.
        session.authStatus = AuthStatus::Invalid;

        // Sauvegarder le message d'erreur renvoyé par le backend.
        session.authErrorMessage = httpMessage.authTokenVerificationResponse.errorMessage;

        // Indiquer l'échec dans la réponse réseau.
        authResponsePacket.status = ServerAuthResponseStatus::INVALID_AUTH_TOKEN;

        // Log d'échec d'authentification.
        RCNET_log(RCNET_LOG_INFO,
                  "[SERVER] [SIMULATION] [AUTH] - connectionId=%u authentication failed: %s\n",
                  httpMessage.connectionId,
                  session.authErrorMessage.c_str());
    }

    // Construire le message simulation -> réseau.
    SimulationToNetworkOUTMessage outMsg{};

    // Renseigner le type logique de message sortant.
    outMsg.type = SimulationToNetworkOUTMessageType::SERVER_AUTH_RESPONSE_PACKET_RELIABLE;

    // Renseigner l'identifiant de connexion cible.
    outMsg.connectionId = httpMessage.connectionId;

    // Sérialiser le packet de réponse.
    outMsg.serializedPacket = serializeServerAuthResponsePacketReliable(authResponsePacket);

    // Pousser le message dans la queue simulation -> réseau.
    simToNetQueue.push(outMsg);
}