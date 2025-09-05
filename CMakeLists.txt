# Version minimale de CMake requise
cmake_minimum_required(VERSION 3.25.0)

project(rcnet_static LANGUAGES C CXX)

# Spécifier la version C++ du projet
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (MSVC)
  # /W3 : Niveau d'avertissement 3 (equivalent a -Wall)
  # /W4 : Niveau d'avertissement 4 (equivalent a -Wextra)
  # /permissive- : Spécifie un mode de compilation strict pour suivre les standards C++ 
  # /wd4100 : Désactive l'avertissement pour les paramètres de fonction non utilisés (C4100)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W3 /W4 /permissive- /wd4100")
else()
  # Configuration des drapeaux du compilateur
  # -Wall : Active tous les avertissements de base
  # -Wextra : Active des avertissements supplémentaires
  # -pedantic : Enforce strictement les standards C
  # -Wno-unused-parameter : Désactive les avertissements pour les paramètres de fonction non utilisés
  # -Wno-error=strict-prototypes : Désactive les erreurs pour les prototypes de fonction stricts
  # -Wno-newline-eof : Désactive les avertissements pour les nouvelles lignes à la fin du fichier
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Wno-unused-parameter -Wno-error=strict-prototypes -Wno-newline-eof")
endif()

# Option pour construire l'exécutable d'exemple
option(RCNET_BUILD_EXAMPLE "Build the example executable" ON)

# Chemin vers les dépendances
set(DEPENDENCIES_PATH "${PROJECT_SOURCE_DIR}/dependencies")
set(CRZGAMES_LIBRARIES_PATH "${DEPENDENCIES_PATH}/Crzgames_Libraries")

# --- Libraries précompilé : trouve les libs selon l'OS
if (WIN32)
  # OpenSSL
  find_library(OPENSSL_CRYPTO_LIBRARY NAMES crypto REQUIRED PATHS "${CRZGAMES_LIBRARIES_PATH}/windows/lib/OpenSSL/x64" NO_DEFAULT_PATH)
  find_library(OPENSSL_LIBRARY NAMES ssl REQUIRED PATHS "${CRZGAMES_LIBRARIES_PATH}/windows/lib/OpenSSL/x64" NO_DEFAULT_PATH)
  set(OPENSSL_INCLUDE_DIR "${CRZGAMES_LIBRARIES_PATH}/windows/include")

  # NATS Client C
  find_library(NATS_LIBRARY NAMES nats_static REQUIRED PATHS "${CRZGAMES_LIBRARIES_PATH}/windows/lib/nats/x64" NO_DEFAULT_PATH)

  # Hiredis
  find_library(HIREDIS_LIBRARY      NAMES hiredis      REQUIRED PATHS "${CRZGAMES_LIBRARIES_PATH}/windows/lib/hiredis/x64" NO_DEFAULT_PATH)
  find_library(HIREDIS_SSL_LIBRARY  NAMES hiredis_ssl  REQUIRED PATHS "${CRZGAMES_LIBRARIES_PATH}/windows/lib/hiredis/x64" NO_DEFAULT_PATH)
elseif(APPLE)
  # OpenSSL
  find_library(OPENSSL_CRYPTO_LIBRARY NAMES crypto REQUIRED PATHS "${CRZGAMES_LIBRARIES_PATH}/macos/lib/OpenSSL" NO_DEFAULT_PATH)
  find_library(OPENSSL_LIBRARY NAMES ssl REQUIRED PATHS "${CRZGAMES_LIBRARIES_PATH}/macos/lib/OpenSSL" NO_DEFAULT_PATH)
  set(OPENSSL_INCLUDE_DIR "${CRZGAMES_LIBRARIES_PATH}/macos/include")
else() # Linux
  # OpenSSL
  find_library(OPENSSL_CRYPTO_LIBRARY NAMES crypto REQUIRED PATHS "${CRZGAMES_LIBRARIES_PATH}/linux/lib/OpenSSL/x64" NO_DEFAULT_PATH)
  find_library(OPENSSL_LIBRARY NAMES ssl REQUIRED PATHS "${CRZGAMES_LIBRARIES_PATH}/linux/lib/OpenSSL/x64" NO_DEFAULT_PATH)
  set(OPENSSL_INCLUDE_DIR "${CRZGAMES_LIBRARIES_PATH}/linux/include")
endif()

# Ajoute nlohmann_json en tant que sous-répertoire
add_subdirectory(dependencies/nlohmann_json)

if(APPLE OR UNIX)
  # NATS options
  set(NATS_BUILD_LIB_STATIC ON CACHE BOOL "" FORCE)  # Pour forcer en static
  set(NATS_BUILD_LIB_SHARED OFF CACHE BOOL "" FORCE) # Pas besoin de la lib dynamique
  set(NATS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)  # Pas besoin des exemples
  set(NATS_BUILD_STREAMING OFF CACHE BOOL "" FORCE)  # Déprécié, maintenant on utilise JetStreaming
  # Ajouter NATS en tant que sous-répertoire
  add_subdirectory(dependencies/Nats EXCLUDE_FROM_ALL)

  # Hiredis options
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)       # Pour forcer en static
  set(ENABLE_SSL ON CACHE BOOL "" FORCE)               # Pour activer hiredis_ssl
  set(DISABLE_TESTS ON CACHE BOOL "" FORCE)            # Pas besoin des tests
  set(ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)         # Pas besoin des exemples
  set(ENABLE_NUGET OFF CACHE BOOL "" FORCE)            # Pas besoin de nuget
  # Ajouter hiredis en tant que sous-répertoire
  add_subdirectory(dependencies/hiredis EXCLUDE_FROM_ALL)
endif()

# Ne pas construire les modules vidéo, audio, etc.
set(SDL_VIDEO OFF CACHE BOOL "" FORCE)
set(SDL_AUDIO OFF CACHE BOOL "" FORCE)
set(SDL_RENDER OFF CACHE BOOL "" FORCE)
set(SDL_JOYSTICK OFF CACHE BOOL "" FORCE)
set(SDL_HAPTIC OFF CACHE BOOL "" FORCE)
set(SDL_SENSOR OFF CACHE BOOL "" FORCE)
set(SDL_POWER OFF CACHE BOOL "" FORCE)
set(SDL_EVENTS OFF CACHE BOOL "" FORCE)
set(SDL_TIMERS ON CACHE BOOL "" FORCE)      # Garde les timers utiles pour un serveur
set(SDL_THREADS ON CACHE BOOL "" FORCE)     # Très utile sur un serveur
# Désactiver les sous-backends inutiles
set(SDL_WAYLAND OFF CACHE BOOL "" FORCE)
set(SDL_X11 OFF CACHE BOOL "" FORCE)
set(SDL_PIPEWIRE OFF CACHE BOOL "" FORCE)
set(SDL_PULSEAUDIO OFF CACHE BOOL "" FORCE)
set(SDL_ALSA OFF CACHE BOOL "" FORCE)
set(SDL_OPENGL OFF CACHE BOOL "" FORCE)
set(SDL_OPENGLES OFF CACHE BOOL "" FORCE)
set(SDL_VULKAN OFF CACHE BOOL "" FORCE)
# Désactiver les tests et les exemples
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
# Ne pas faire d’erreur si aucun backend GUI n’est présent
set(SDL_UNIX_CONSOLE_BUILD ON CACHE BOOL "" FORCE)
# Ajouter SDL3 en tant que sous-répertoire
add_subdirectory(dependencies/SDL EXCLUDE_FROM_ALL)

# Ajouter les fichiers sources du server de jeu ainsi que les librairies aes, lz4 et monocypher.
file(GLOB_RECURSE SOURCES 
  "${PROJECT_SOURCE_DIR}/src/RCNET/*.cpp"
  "${PROJECT_SOURCE_DIR}/src/external/monocypher/*.c"
  "${PROJECT_SOURCE_DIR}/src/external/aes/*.c"
  "${PROJECT_SOURCE_DIR}/src/external/lz4/*.c"
)

# Créer une bibliothèque static
add_library(${PROJECT_NAME}
  STATIC 
  ${SOURCES}
)

# Définir des flags de compilation pour le mode Release (NDEBUG = No Debug, donc Release)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")

# Ajouter les fichiers include du serveur de jeu
target_include_directories(${PROJECT_NAME} PUBLIC
  "${PROJECT_SOURCE_DIR}/include"
  "${PROJECT_SOURCE_DIR}/include/external"
  "${DEPENDENCIES_PATH}/Nats/src/include"
  "${DEPENDENCIES_PATH}/Nats/src"
  "${DEPENDENCIES_PATH}/hiredis/"
  "${DEPENDENCIES_PATH}/nlohmann_json/include"
  "${DEPENDENCIES_PATH}/cpp-httplib"
  "${DEPENDENCIES_PATH}/jwt-cpp/include"
  "${DEPENDENCIES_PATH}/SDL/include"
  "${OPENSSL_INCLUDE_DIR}"
)

# Link les librairies
if (WIN32)
  target_link_libraries(${PROJECT_NAME} PUBLIC
    SDL3::SDL3
    ws2_32 # Pour OpenSSL
    Crypt32 # Pour OpenSSL
    ${OPENSSL_LIBRARY}
    ${OPENSSL_CRYPTO_LIBRARY}
    ${HIREDIS_LIBRARY}
    ${HIREDIS_SSL_LIBRARY}
    ${NATS_LIBRARY}
    nlohmann_json::nlohmann_json
  )
else()
  target_link_libraries(${PROJECT_NAME} PUBLIC
    SDL3::SDL3
    ${OPENSSL_LIBRARY}
    ${OPENSSL_CRYPTO_LIBRARY}
    hiredis::hiredis
    hiredis::hiredis_ssl
    nats_static
    nlohmann_json::nlohmann_json
  )
endif()

if(RCNET_BUILD_EXAMPLE)
  # Ajouter le fichier source pour l'exécutable de test
  file(GLOB_RECURSE EXAMPLE_SOURCES 
    "${PROJECT_SOURCE_DIR}/example/src/*.cpp"
  )

  # Créer l'exécutable de test
  add_executable(rcnet_example ${EXAMPLE_SOURCES})

  # Ajouter les fichiers include pour l'exécutable de test
  target_include_directories(rcnet_example PRIVATE
    "${PROJECT_SOURCE_DIR}/include"
    "${PROJECT_SOURCE_DIR}/include/external"
    "${DEPENDENCIES_PATH}/Nats/src/include"
    "${DEPENDENCIES_PATH}/Nats/src"
    "${DEPENDENCIES_PATH}/hiredis/"
    "${DEPENDENCIES_PATH}/nlohmann_json/include"
    "${DEPENDENCIES_PATH}/cpp-httplib"
    "${DEPENDENCIES_PATH}/jwt-cpp/include"
    "${DEPENDENCIES_PATH}/SDL/include"
    "${OPENSSL_INCLUDE_DIR}"
    "${PROJECT_SOURCE_DIR}/example/include"
  )

  # Link les librairies
  if (WIN32)
    target_link_libraries(rcnet_example
      SDL3::SDL3
      ws2_32 # Pour OpenSSL
      Crypt32 # Pour OpenSSL
      ${OPENSSL_LIBRARY}
      ${OPENSSL_CRYPTO_LIBRARY}
      ${HIREDIS_LIBRARY}
      ${HIREDIS_SSL_LIBRARY}
      ${NATS_LIBRARY}
      nlohmann_json::nlohmann_json
      ${PROJECT_NAME}
    )
  else()
    target_link_libraries(rcnet_example
      SDL3::SDL3
      ${OPENSSL_LIBRARY}
      ${OPENSSL_CRYPTO_LIBRARY}
      hiredis::hiredis
      hiredis::hiredis_ssl
      nats_static
      nlohmann_json::nlohmann_json
      ${PROJECT_NAME}
    )
  endif()
endif()
