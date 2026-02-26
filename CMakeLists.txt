# CMake 3.28.0 est la version minimale requise pour ce projet
cmake_minimum_required(VERSION 3.28.0)

# ============================================================
# Project Configuration
# ============================================================
# Nom du projet : rcnet
#
# Langages :
#   - CXX   : cœur du moteur de serveur de jeu et des exemples
# ============================================================
project(rcnet
  LANGUAGES C CXX
  VERSION 1.0.0
)

# ============================================================
# Language Standards
# ============================================================
# CXX : C++17 pour les fonctionnalités modernes et 
#       la compatibilité avec les bibliothèques utilisées
## ============================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Option pour construire l'exécutable d'exemple
option(RCNET_BUILD_EXAMPLE "Build the example executable" ON)

# ============================================================
# Output Directories
# ============================================================
# Gestion du dossier de sortie selon le générateur CMake :
# - Visual Studio / Xcode : multi-config (Debug/Release/...)
# - Ninja / Makefiles     : single-config
# ============================================================
if(CMAKE_GENERATOR MATCHES "Visual Studio|Xcode")
  # Multi-config generators
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIGURATION>")
else()
  # Single-config generators (Makefiles, Ninja, etc.)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
endif()

# ============================================================
# Platform Minimum Versions / Architectures
# ============================================================
# Définit les versions minimales supportées (Windows / Apple)
# et les architectures cibles (ex: arm64 sur Apple).
# ============================================================
if(WIN32)
  set(CMAKE_SYSTEM_VERSION 10.0.10240 CACHE STRING "Windows 10 Minimum Version" FORCE)
elseif(APPLE)
  if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "macOS Architectures" FORCE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0" CACHE STRING "macOS Deployment Target" FORCE)
  endif()
endif()

# ============================================================
# Compiler Warnings & Strictness
# ============================================================
# Flags d'avertissements et de conformité :
# - MSVC : /W4, /permissive- (C++ strict), désactivations ciblées
# - GCC/Clang : -Wall -Wextra -pedantic + exceptions ciblées
# ============================================================
if(MSVC)
  # /W4 : Niveau d'avertissement 4 (equivalent a -Wextra)
  # /permissive- : Spécifie un mode de compilation strict pour suivre les standards C++ 
  # /wd4100 : Désactive l'avertissement pour les paramètres de fonction non utilisés (C4100)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /permissive- /wd4100")
else()
  # -Wall : Active tous les avertissements de base
  # -Wextra : Active des avertissements supplémentaires
  # -pedantic : Enforce strictement les standards C++
  # -Wno-unused-parameter : Désactive les avertissements pour les paramètres de fonction non utilisés
  # -Wno-error=strict-prototypes : Désactive les erreurs pour les prototypes de fonction stricts en C.
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Wno-unused-parameter")

  # Clang a un warning spécifique pour les fichiers qui ne se terminent pas par une nouvelle ligne (newline at end of file).
  # -Wno-newline-eof : Désactive les avertissements pour les nouvelles lignes à la fin du fichier
  if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-newline-eof")
  endif()
endif()

# ============================================================
# Hiredis — Configuration build
# ============================================================
if(UNIX AND NOT APPLE AND NOT ANDROID) # Linux, Raspberry PI 5 etc.
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)       # Pour forcer en static
  set(ENABLE_SSL ON CACHE BOOL "" FORCE)               # Pour activer hiredis_ssl
  set(DISABLE_TESTS ON CACHE BOOL "" FORCE)            # Pas besoin des tests
  set(ENABLE_EXAMPLES OFF CACHE BOOL "" FORCE)         # Pas besoin des exemples
  set(ENABLE_NUGET OFF CACHE BOOL "" FORCE)            # Pas besoin de nuget
endif()

# ============================================================
# NATS Client C — Configuration build
# ============================================================
if (UNIX AND NOT APPLE AND NOT ANDROID) # Linux, Raspberry PI 5 etc.
  set(NATS_BUILD_LIB_STATIC ON CACHE BOOL "" FORCE)  # Pour forcer en static
  set(NATS_BUILD_LIB_SHARED OFF CACHE BOOL "" FORCE) # Pas besoin de la lib dynamique
  set(NATS_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)   # Pas besoin des exemples
  set(NATS_BUILD_STREAMING OFF CACHE BOOL "" FORCE)  # Déprécié, maintenant on utilise JetStreaming
endif()

# ============================================================
# cJSON — Configuration build
# ============================================================
# Objectif :
# - Build en statique uniquement
# - Pas de tests / utilitaires (inutile dans un framework)
# ============================================================
set(CMAKE_POLICY_VERSION_MINIMUM "3.5" CACHE STRING "Allow old subprojects" FORCE) # Autoriser les vieux subprojects de CJSON
set(CJSON_OVERRIDE_BUILD_SHARED_LIBS ON  CACHE BOOL "" FORCE) # ignore BUILD_SHARED_LIBS global
set(CJSON_BUILD_SHARED_LIBS      OFF CACHE BOOL "" FORCE) # force STATIC pour cJSON
set(ENABLE_CJSON_TEST   OFF CACHE BOOL "" FORCE) # Désactiver les tests
set(ENABLE_CJSON_UTILS  OFF CACHE BOOL "" FORCE) # Désactiver les outils

# ============================================================
# Dépendances vendored — add_subdirectory
# ============================================================
# On inclut SDL + modules, et cJSON depuis dependencies/.
# EXCLUDE_FROM_ALL : évite de builder ces targets si aucune target ne les référence.
# ============================================================
add_subdirectory(dependencies/SDL   EXCLUDE_FROM_ALL)
add_subdirectory(dependencies/cJSON EXCLUDE_FROM_ALL)
if(UNIX AND NOT APPLE AND NOT ANDROID) # Linux, Raspberry PI 5 etc.
  add_subdirectory(dependencies/hiredis EXCLUDE_FROM_ALL)
  add_subdirectory(dependencies/nats    EXCLUDE_FROM_ALL)
endif()

# ============================================================
# Paths — Dépendances précompilées (Crzgames_Libraries)
# ============================================================
# DEPENDENCIES_PATH : racine de ton dossier dependencies/
# CRZGAMES_LIBRARIES_PATH: dossier où on ranges les libs précompilées (openssl, rcenet etc.) pour chaque plateforme.
# ============================================================
set(DEPENDENCIES_PATH "${PROJECT_SOURCE_DIR}/dependencies")
set(CRZGAMES_LIBRARIES_PATH "${DEPENDENCIES_PATH}/Crzgames_Libraries")

# ============================================================
# rcnet_configure_cpphttplib
# ============================================================
function(rcnet_configure_cpphttplib target_name)
  target_include_directories(${target_name} PUBLIC
    "${DEPENDENCIES_PATH}/cpp-httplib"
  )
endfunction()

# ============================================================
# rcnet_configure_hiredis
#
# Configure hiredis + hiredis_ssl (prebuilt Crzgames_Libraries)
#
# Objectif :
# - Ajouter include + lib dirs selon la plateforme
# - Lier hiredis + hiredis_ssl
# - Lier les libs système nécessaires (Windows)
# ============================================================
function(rcnet_configure_hiredis target_name)
  if(WIN32) # Windows
    target_include_directories(${target_name} PUBLIC
      "${CRZGAMES_LIBRARIES_PATH}/windows/include/include_hiredis"
    )

    target_link_directories(${target_name} PUBLIC
      "${CRZGAMES_LIBRARIES_PATH}/windows/lib/hiredis/x64"
    )

    target_link_libraries(${target_name} PUBLIC
      hiredis
      hiredis_ssl
      ws2_32   # sockets
      Crypt32  # utilisé par OpenSSL / crypto API selon builds
    )

  elseif(APPLE AND CMAKE_SYSTEM_NAME STREQUAL "Darwin") # macOS
    target_include_directories(${target_name} PUBLIC
      "${CRZGAMES_LIBRARIES_PATH}/macos/include/include_hiredis"
    )

    target_link_directories(${target_name} PUBLIC
      "${CRZGAMES_LIBRARIES_PATH}/macos/lib/hiredis"
    )

    target_link_libraries(${target_name} PUBLIC
      hiredis
      hiredis_ssl
    )
  endif()
endfunction()

# ============================================================
# rcnet_configure_nats
#
# Configure NATS C client (prebuilt Crzgames_Libraries)
#
# Objectif :
# - Ajouter include + lib dirs selon la plateforme
# - Lier nats_static
# - Ajouter libs système nécessaires (Windows)
# ============================================================
function(rcnet_configure_nats target_name)
  if(WIN32) # Windows
    target_include_directories(${target_name} PUBLIC
      "${CRZGAMES_LIBRARIES_PATH}/windows/include/include_nats"
    )

    target_link_directories(${target_name} PUBLIC
      "${CRZGAMES_LIBRARIES_PATH}/windows/lib/nats/x64"
    )

    target_link_libraries(${target_name} PUBLIC
      nats_static
      ws2_32
      iphlpapi
      Crypt32
    )

  elseif(APPLE AND CMAKE_SYSTEM_NAME STREQUAL "Darwin") # macOS
    target_include_directories(${target_name} PUBLIC
      "${CRZGAMES_LIBRARIES_PATH}/macos/include/include_nats"
    )

    target_link_directories(${target_name} PUBLIC
      "${CRZGAMES_LIBRARIES_PATH}/macos/lib/nats"
    )

    target_link_libraries(${target_name} PUBLIC
      nats_static
    )
  endif()
endfunction()

# ============================================================
# OpenSSL Configuration Function
# ============================================================
# Cette fonction configure les chemins d'inclusion et de liaison pour OpenSSL
# en fonction de la plateforme cible. Elle est appelée pour chaque target qui nécessite OpenSSL.
# ============================================================
function(rcnet_configure_openssl target_name)
  if(APPLE AND CMAKE_SYSTEM_NAME STREQUAL "Darwin") # macOS
    # Link le dossier include de OpenSSL à la target
    target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES_PATH}/macos/include/include_openssl")

    # Rajouter le répertoire de la lib OpenSSL (libssl_static et libcrypto_static), pour le linker ensuite
    target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES_PATH}/macos/lib/OpenSSL")

    # Link les librairies OpenSSL
    target_link_libraries(${target_name} PUBLIC
      ssl    # OpenSSL
      crypto # OpenSSL
    )
  elseif(WIN32) # Windows
    # Link le dossier include de OpenSSL à la target
    target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES_PATH}/windows/include/include_openssl")

    # Rajouter le répertoire de la lib OpenSSL (libssl_static et libcrypto_static), pour le linker ensuite
    target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES_PATH}/windows/lib/OpenSSL/x64")

    # Link les librairies OpenSSL
    target_link_libraries(${target_name} PUBLIC
      libssl    # OpenSSL
      libcrypto # OpenSSL
      Crypt32   # OpenSSL CryptoAPI (Librarie native de Windows, obligatoire)
      ws2_32    # OpenSSL Winsock2  (Librarie native de Windows, obligatoire)
    )
  elseif (UNIX AND NOT APPLE AND NOT ANDROID) # Linux, Raspberry PI 5 etc.
    # Link le dossier include de OpenSSL à la target
    target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES_PATH}/linux/include/include_openssl")

    # Rajouter le répertoire de la lib OpenSSL (libssl_static et libcrypto_static), pour le linker ensuite
    target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES_PATH}/linux/lib/OpenSSL/x64")

    # Link les librairies OpenSSL
    target_link_libraries(${target_name} PUBLIC
      ssl    # OpenSSL
      crypto # OpenSSL
    )
  endif()
endfunction()

# ============================================================
# rcnet_configure_rcenet
#
# Configure RCEnet pour une target donnée.
#
# Objectif :
# - Ajouter include + lib dirs selon la plateforme
# - Lier rcenet
# - Ajouter les libs système nécessaires (Windows : ws2_32, winmm)
# ============================================================
function(rcnet_configure_rcenet target_name)
  if (WIN32) # Windows
    # Link le dossier include de RCEnet à la target
    target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES_PATH}/windows/include/include_rcenet")

    # Rajouter le répertoire de la lib RCEnet, pour le linker ensuite
    target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES_PATH}/windows/lib/rcenet/x64")

    # Link les librairies RCEnet
    target_link_libraries(${target_name} PUBLIC
      rcenet
      ws2_32 # Bibliothèque native à Windows (pour la librairie Enet)
      winmm # Bibliothèque native à Windows (pour la librairie Enet)
    )

  elseif(APPLE AND CMAKE_SYSTEM_NAME STREQUAL "Darwin") # macOS
    # Link le dossier include de RCEnet à la target
    target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES_PATH}/macos/include/include_rcenet")

    # Ajouter explicitement la lib .dylib à linker
    target_link_directories(${target_name} PUBLIC
      "${CRZGAMES_LIBRARIES_PATH}/macos/lib/rcenet"
    )

    # Link la librairies RCEnet
    target_link_libraries(${target_name} PUBLIC
      rcenet
    )
  elseif (UNIX AND NOT APPLE AND NOT ANDROID) # Linux, Raspberry PI 5 etc.
    # Link le dossier include de RCEnet à la target
    target_include_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES_PATH}/linux/include/include_rcenet")

    # Rajouter le répertoire de la lib RCEnet, pour le linker ensuite
    target_link_directories(${target_name} PUBLIC "${CRZGAMES_LIBRARIES_PATH}/linux/lib/rcenet/x64")

    # Link les librairies RCEnet
    target_link_libraries(${target_name} PUBLIC
      rcenet
    )
  endif()
endfunction()

# ============================================================
# rcnet_configure_cjson
#
# Configure l’intégration de cJSON pour une target donnée.
#
# Objectif :
# - Exposer les headers cJSON via include path (PUBLIC)
# - Lier la lib statique cJSON à la target
# ============================================================
function(rcnet_configure_cjson target_name)
  # Le header cJSON.h est dans dependencies/cJSON/cJSON.h
  target_include_directories(${target_name} PUBLIC
    "${DEPENDENCIES_PATH}/cJSON"
  )

  # Link la librairie cJSON
  target_link_libraries(${target_name} PUBLIC cjson)
endfunction()

# ============================================================
# Sources & Headers (RCNET)
# ============================================================
# Récupère automatiquement tous les fichiers sources C++ du moteur,
# ainsi que les headers, pour les ajouter à la target.
# ============================================================

# Ajouter les fichiers sources du server de jeu et lz4.
file(GLOB_RECURSE SOURCES 
  "${PROJECT_SOURCE_DIR}/src/*.cpp"
  "${PROJECT_SOURCE_DIR}/src/*.c"
)

# Ajouter les fichiers d'en-tête du server de jeu, base64 et lz4.
file(GLOB_RECURSE HEADERS 
  "${PROJECT_SOURCE_DIR}/include/*.hpp"
  "${PROJECT_SOURCE_DIR}/include/*.h"
)

# ============================================================
# Library Target (static)
# ============================================================
# Crée la target principale du moteur RCNet, en utilisant les sources 
# et headers récupérés.
# ============================================================
add_library(${PROJECT_NAME}
  STATIC 
  ${SOURCES}
  ${HEADERS}
)

# ============================================================
# Include Directories (Public API)
# ============================================================
# Expose les headers publics (include/) et les éventuels headers externes
# (include/external) à tous les projets qui linkent RCNET.
# ============================================================
target_include_directories(${PROJECT_NAME} PUBLIC
  "${PROJECT_SOURCE_DIR}/include"
  "${PROJECT_SOURCE_DIR}/include/external"
)

# ============================================================
# Link Common Dependencies
# ===========================================================
target_link_libraries(${PROJECT_NAME} PUBLIC
  SDL3::SDL3
)

# OpenSSL
rcnet_configure_openssl(${PROJECT_NAME})

# RCEnet / ENet
rcnet_configure_rcenet(${PROJECT_NAME})

# cJSON (parsing JSON léger côté runtime)
rcnet_configure_cjson(${PROJECT_NAME})

# Hiredis (client Redis pour la gestion de cache, sessions, etc.)
rcnet_configure_hiredis(${PROJECT_NAME})

# NATS Client C (client NATS pour la communication inter-services)
rcnet_configure_nats(${PROJECT_NAME})

# cpp-httplib (client HTTP léger pour les requêtes REST, webhooks, etc.)
rcnet_configure_cpphttplib(${PROJECT_NAME})







# ============================================================
#
# Example Target (exécutable de démonstration)
#
# ============================================================
if(RCNET_BUILD_EXAMPLE)
  # ============================================================
  # Nom de la target de l’exécutable d’exemple
  # ============================================================
  set(RCNET_EXAMPLE_TARGET_NAME rcnet_example)

  # ============================================================
  # Fichiers source de l’exemple
  #
  # Récupère automatiquement tous les fichiers sources .cpp de manière récursive 
  # dans "example/src" pour les compiler dans la target de l’exemple.
  # ============================================================
  file(GLOB_RECURSE EXAMPLE_SOURCES 
    "${PROJECT_SOURCE_DIR}/example/src/*.cpp"
  )

  # ============================================================
  # Fichiers d’en-tête (headers) de l’exemple
  #
  # Récupère automatiquement tous les fichiers d’en-tête .h de manière récursive 
  # dans "example/include" pour les inclure dans la target de l’exemple.
  # ============================================================
  file(GLOB_RECURSE EXAMPLE_HEADERS 
    "${PROJECT_SOURCE_DIR}/example/include/*.h"
  )

  # ============================================================
  # Création de la target exécutable pour l’exemple
  # ============================================================
  add_executable(${RCNET_EXAMPLE_TARGET_NAME}
    ${EXAMPLE_SOURCES} 
    ${EXAMPLE_HEADERS}
  )

  # ============================================================
  # Include directories pour l’exemple
  # 
  # Ajoute le dossier "examples/include" aux include paths privés de la target de l’exemple.
  # ============================================================
  target_include_directories(${RCNET_EXAMPLE_TARGET_NAME} PRIVATE 
    "${PROJECT_SOURCE_DIR}/example/include"
  )

  # ============================================================
  # Linker l’exemple avec la bibliothèque RCNET
  #
  # Lie la target de l’exemple à la bibliothèque RCNET pour pouvoir utiliser le moteur dans l’exemple.
  # ============================================================
  target_link_libraries(${RCNET_EXAMPLE_TARGET_NAME} PRIVATE
    ${PROJECT_NAME} # RCNET
  )
endif()
