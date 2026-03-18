#include "simulation/process/http/auth_validatetokenresponse_message.h"

#include "auth/types.h"
#include "network/packets/server/reliable.h"
#include "network/serialization/serialize_packets_server.h"

#include <mutex>         // std::lock_guard
#include <unordered_map> // std::unordered_map

#include <RCNET/RCNET.h>

void ServerSimulation_ProcessHttpDispatcher_HandleAuthValidateTokenResponseMessage(
    NetworkState& networkState,
    SimulationToNetworkOUTQueue& simToNetQueue,
    const HttpToSimulationMessage& httpMessage)
{
    // Préparer le packet de réponse serveur -> client.
    ServerAuthResponsePacketReliable authResponsePacket{};

    // Renseigner le type réseau du packet.
    authResponsePacket.header.type = ServerReliablePacketType::SERVER_AUTH_RESPONSE_PACKET_RELIABLE;

    {
        std::lock_guard<std::mutex> lock(networkState.sessionsMutex);

        // Rechercher la session associée à cette réponse HTTP.
        std::unordered_map<uint32_t, ClientSession>::iterator sit =
            networkState.sessions.find(httpMessage.connectionId);

        // Si la session n'existe pas, on ne peut pas rattacher la réponse.
        if (sit == networkState.sessions.end())
        {
            // Log d'avertissement indiquant une réponse orpheline.
            RCNET_log(RCNET_LOG_WARN,
                      "[SERVER] [SIMULATION] [AUTH] - connectionId: %u - Auth validation response received for unknown session\n",
                      httpMessage.connectionId);

            // Abandon du traitement.
            return;
        }

        // Référence directe vers la session concernée.
        ClientSession& session = sit->second;

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
                      "[SERVER] [SIMULATION] [AUTH] - connectionId: %u - Token validated (accountId: %llu, username: %s)\n",
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

            // Eviter un log vide si aucune erreur n'a ete fournie.
            const char* authFailureReason =
                session.authErrorMessage.empty() ? "unknown reason" : session.authErrorMessage.c_str();

            // Log d'échec d'authentification.
            RCNET_log(RCNET_LOG_INFO,
                      "[SERVER] [SIMULATION] [AUTH] - connectionId: %u - Token validation failed (reason=%s)\n",
                      httpMessage.connectionId,
                      authFailureReason);
        }
    }

    // Construire le message simulation -> réseau.
    SimulationToNetworkOUTMessage outMsg{};

    // Renseigner le type logique de message sortant.
    outMsg.type = SimulationToNetworkOUTMessageType::SERVER_AUTH_RESPONSE_PACKET_RELIABLE;

    // Renseigner l'identifiant de connexion cible.
    outMsg.connectionId = httpMessage.connectionId;

    // Renseigner l'indicateur de déconnexion après accusé de réception du packet correspondant.
    // Si le token est invalide, on veut déconnecter le client après lui avoir envoyé la réponse d'échec d'authentification.
    outMsg.disconnectAfterAck = (httpMessage.authTokenVerificationResponse.isValid == false);

    // Sérialiser le packet de réponse.
    outMsg.serializedPacket = serializeServerAuthResponsePacketReliable(authResponsePacket);

    // Pousser le message dans la queue simulation -> réseau.
    simToNetQueue.push(outMsg);
}
