get_filename_component(RCNET_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
set(VENDORED_DIR "${RCNET_ROOT}/dependencies")
set(DEPENDENCIES_FILE "${RCNET_ROOT}/dependencies.txt")

# Créer le répertoire s'il n'existe pas
file(MAKE_DIRECTORY ${VENDORED_DIR})

# Lire le contenu du fichier dependencies.txt
file(READ ${DEPENDENCIES_FILE} DEPENDENCIES_CONTENT)

# Normaliser les fins de lignes pour compatibilité multiplateforme
string(REPLACE "\r\n" "\n" DEPENDENCIES_CONTENT "${DEPENDENCIES_CONTENT}")
string(REPLACE "\r" "\n" DEPENDENCIES_CONTENT "${DEPENDENCIES_CONTENT}")
string(REPLACE "\n" ";" DEPENDENCIES_LINES "${DEPENDENCIES_CONTENT}")

# Parcourir chaque ligne de dépendance
foreach(LINE IN LISTS DEPENDENCIES_LINES)
  string(STRIP "${LINE}" LINE)

  # Ignorer les lignes vides ou commentaires
  if(LINE STREQUAL "" OR LINE MATCHES "^#.*")
    continue()
  endif()

  # Extraire le nom de la lib
  string(REGEX MATCH "^[^=]+" LIB_NAME "${LINE}")
  # Extraire le repository et le tag/commit
  string(REGEX REPLACE "^[^=]+=" "" VALUE_PART "${LINE}")
  string(REGEX MATCH "^(.*):([^:]*)$" _ "${VALUE_PART}")

  if(NOT _)
    message(WARNING "❌ Ligne de dépendance invalide: '${LINE}'")
    continue()
  endif()

  set(LIB_REPO "${CMAKE_MATCH_1}")
  set(LIB_REF "${CMAKE_MATCH_2}")
  set(CLONE_DIR "${VENDORED_DIR}/${LIB_NAME}")

  # Cloner le dépôt si non présent
  if(NOT EXISTS "${CLONE_DIR}/.git")
    message(STATUS "📥 Clonage ${LIB_NAME} depuis ${LIB_REPO} @ ${LIB_REF}")
    execute_process(COMMAND git clone ${LIB_REPO} ${CLONE_DIR})
  endif()

  # S'assurer d'avoir toutes les refs distantes
  execute_process(COMMAND git fetch origin
                  WORKING_DIRECTORY ${CLONE_DIR})
  execute_process(COMMAND git fetch --all
                  WORKING_DIRECTORY ${CLONE_DIR})

  # Revenir proprement à la version souhaitée (tag ou commit SHA)
  execute_process(COMMAND git reset --hard ${LIB_REF}
                  WORKING_DIRECTORY ${CLONE_DIR})

  # Mettre à jour les éventuels sous-modules
  execute_process(COMMAND git submodule update --init --recursive
                  WORKING_DIRECTORY ${CLONE_DIR})
endforeach()

# Vérifie s'il faut exécuter le setup_dependencies.cmake de Crzgames_RC2D
set(RC2D_DEPENDENCIES_SCRIPT "${VENDORED_DIR}/Crzgames_RC2D/cmake/setup_dependencies.cmake")
if(EXISTS "${RC2D_DEPENDENCIES_SCRIPT}")
  message(STATUS "➡️  Exécution de setup_dependencies.cmake dans Crzgames_RC2D")
  execute_process(
    COMMAND ${CMAKE_COMMAND} -P ${RC2D_DEPENDENCIES_SCRIPT}
    WORKING_DIRECTORY "${VENDORED_DIR}/Crzgames_RC2D"
  )
endif()

# Patch Agones CMakeLists pour supporter un usage via add_subdirectory
# sans dépendre exclusivement de find_package(gRPC/Protobuf), et sans
# exporter les règles d'installation quand Agones est un sous-projet.
set(AGONES_CMAKELISTS "${VENDORED_DIR}/agones/sdks/cpp/CMakeLists.txt")
if(EXISTS "${AGONES_CMAKELISTS}")
  file(READ "${AGONES_CMAKELISTS}" AGONES_CMAKELISTS_CONTENT_RAW)

  # Normaliser pour simplifier les remplacements.
  string(REPLACE "\r\n" "\n" AGONES_CMAKELISTS_CONTENT "${AGONES_CMAKELISTS_CONTENT_RAW}")
  string(REPLACE "\r" "\n" AGONES_CMAKELISTS_CONTENT "${AGONES_CMAKELISTS_CONTENT}")

  set(AGONES_CMAKELISTS_CHANGED FALSE)

  # Patch bloc gRPC (+ suppression du message dupliqué dans le fichier original)
  set(AGONES_GRPC_BLOCK_ORIGINAL [=[# gRPC
find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(Threads REQUIRED)

message(STATUS "gRPC version found: ${gRPC_VERSION}")]=])

  set(AGONES_GRPC_BLOCK_PATCHED [=[# gRPC
if (NOT TARGET protobuf::libprotobuf)
    find_package(Protobuf REQUIRED)
endif()
if (TARGET libprotobuf AND NOT TARGET protobuf::libprotobuf)
    add_library(protobuf::libprotobuf ALIAS libprotobuf)
endif()

if (NOT TARGET gRPC::grpc++_unsecure AND NOT TARGET grpc++_unsecure)
    find_package(gRPC CONFIG REQUIRED)
endif()
if (TARGET grpc++_unsecure AND NOT TARGET gRPC::grpc++_unsecure)
    add_library(gRPC::grpc++_unsecure ALIAS grpc++_unsecure)
endif()

find_package(Threads REQUIRED)

if (gRPC_VERSION)
    message(STATUS "gRPC version found: ${gRPC_VERSION}")
else()
    message(STATUS "gRPC target found from parent project")
endif()]=])

  string(FIND "${AGONES_CMAKELISTS_CONTENT}" "gRPC target found from parent project" AGONES_GRPC_ALREADY_PATCHED_POS)
  if(AGONES_GRPC_ALREADY_PATCHED_POS GREATER -1)
    message(STATUS "Agones CMakeLists: patch gRPC déjà présent")
  else()
    string(FIND "${AGONES_CMAKELISTS_CONTENT}" "${AGONES_GRPC_BLOCK_ORIGINAL}" AGONES_GRPC_ORIGINAL_POS)
    if(AGONES_GRPC_ORIGINAL_POS GREATER -1)
      string(REPLACE "${AGONES_GRPC_BLOCK_ORIGINAL}" "${AGONES_GRPC_BLOCK_PATCHED}" AGONES_CMAKELISTS_CONTENT "${AGONES_CMAKELISTS_CONTENT}")
      set(AGONES_CMAKELISTS_CHANGED TRUE)
      message(STATUS "Agones CMakeLists: patch gRPC appliqué")
    else()
      message(WARNING "Agones CMakeLists: bloc gRPC original introuvable, patch non appliqué")
    endif()
  endif()

  # Patch bloc installation/export Agones
  set(AGONES_INSTALL_PATCH_MARKER "option(AGONES_INSTALL \"Enable Agones install/export rules\"")
  string(FIND "${AGONES_CMAKELISTS_CONTENT}" "${AGONES_INSTALL_PATCH_MARKER}" AGONES_INSTALL_ALREADY_PATCHED_POS)
  if(AGONES_INSTALL_ALREADY_PATCHED_POS GREATER -1)
    message(STATUS "Agones CMakeLists: patch install déjà présent")
  else()
    set(AGONES_INSTALL_BLOCK_ORIGINAL [=[# CMake package generation
include(CMakePackageConfigHelpers)

set(_INCLUDE_DIRS "agones/include")
set(_CMAKE_CONFIG_DESTINATION "agones/cmake")

# Config for find_package
configure_package_config_file(
    cmake/${PROJECT_NAME}Config.cmake.in
    ${PROJECT_NAME}Config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_PREFIX}/${_CMAKE_CONFIG_DESTINATION}
    PATH_VARS _INCLUDE_DIRS PROJECT_VERSION
    NO_SET_AND_CHECK_MACRO
)

# Build artifacts
install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}
    LIBRARY DESTINATION  ${PROJECT_NAME}/lib
    ARCHIVE DESTINATION  ${PROJECT_NAME}/lib
    RUNTIME DESTINATION  ${PROJECT_NAME}/bin
    INCLUDES DESTINATION ${_INCLUDE_DIRS}
)
install(EXPORT ${PROJECT_NAME} DESTINATION ${_CMAKE_CONFIG_DESTINATION} FILE ${PROJECT_NAME}Targets.cmake)
# Package config
install(
    FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    DESTINATION ${_CMAKE_CONFIG_DESTINATION}
)
# Agones header files
install(
    FILES ${HEADER_FILES} "${CMAKE_CURRENT_BINARY_DIR}/${EXPORT_HEADER}" "${CMAKE_CURRENT_BINARY_DIR}/${GLOBAL_HEADER}"
    DESTINATION ${_INCLUDE_DIRS}/${PROJECT_NAME}
)
# Generated header files
install(
    FILES ${GENERATED_HEADER_FILES}
    DESTINATION ${_INCLUDE_DIRS}/${PROJECT_NAME}
)
# Google header files
install(
    FILES ${GENERATED_GOOGLE_HEADER_FILES}
    DESTINATION ${_INCLUDE_DIRS}/google/api
)
# grpc-gateway header files
install(
    FILES ${GENERATED_GRPC_HEADER_FILES}
    DESTINATION ${_INCLUDE_DIRS}/protoc-gen-openapiv2/options
)

unset(_INCLUDE_DIRS)
unset(_CMAKE_CONFIG_DESTINATION)]=])

    set(AGONES_INSTALL_BLOCK_PATCHED [=[set(AGONES_INSTALL_DEFAULT ON)
if (NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    set(AGONES_INSTALL_DEFAULT OFF)
endif()
option(AGONES_INSTALL "Enable Agones install/export rules" ${AGONES_INSTALL_DEFAULT})

if (AGONES_INSTALL)
    # CMake package generation
    include(CMakePackageConfigHelpers)

    set(_INCLUDE_DIRS "agones/include")
    set(_CMAKE_CONFIG_DESTINATION "agones/cmake")

    # Config for find_package
    configure_package_config_file(
        cmake/${PROJECT_NAME}Config.cmake.in
        ${PROJECT_NAME}Config.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_PREFIX}/${_CMAKE_CONFIG_DESTINATION}
        PATH_VARS _INCLUDE_DIRS PROJECT_VERSION
        NO_SET_AND_CHECK_MACRO
    )

    # Build artifacts
    install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}
        LIBRARY DESTINATION  ${PROJECT_NAME}/lib
        ARCHIVE DESTINATION  ${PROJECT_NAME}/lib
        RUNTIME DESTINATION  ${PROJECT_NAME}/bin
        INCLUDES DESTINATION ${_INCLUDE_DIRS}
    )
    install(EXPORT ${PROJECT_NAME} DESTINATION ${_CMAKE_CONFIG_DESTINATION} FILE ${PROJECT_NAME}Targets.cmake)
    # Package config
    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        DESTINATION ${_CMAKE_CONFIG_DESTINATION}
    )
    # Agones header files
    install(
        FILES ${HEADER_FILES} "${CMAKE_CURRENT_BINARY_DIR}/${EXPORT_HEADER}" "${CMAKE_CURRENT_BINARY_DIR}/${GLOBAL_HEADER}"
        DESTINATION ${_INCLUDE_DIRS}/${PROJECT_NAME}
    )
    # Generated header files
    install(
        FILES ${GENERATED_HEADER_FILES}
        DESTINATION ${_INCLUDE_DIRS}/${PROJECT_NAME}
    )
    # Google header files
    install(
        FILES ${GENERATED_GOOGLE_HEADER_FILES}
        DESTINATION ${_INCLUDE_DIRS}/google/api
    )
    # grpc-gateway header files
    install(
        FILES ${GENERATED_GRPC_HEADER_FILES}
        DESTINATION ${_INCLUDE_DIRS}/protoc-gen-openapiv2/options
    )

    unset(_INCLUDE_DIRS)
    unset(_CMAKE_CONFIG_DESTINATION)
endif()]=])

    string(FIND "${AGONES_CMAKELISTS_CONTENT}" "${AGONES_INSTALL_BLOCK_ORIGINAL}" AGONES_INSTALL_ORIGINAL_POS)
    if(AGONES_INSTALL_ORIGINAL_POS GREATER -1)
      string(REPLACE "${AGONES_INSTALL_BLOCK_ORIGINAL}" "${AGONES_INSTALL_BLOCK_PATCHED}" AGONES_CMAKELISTS_CONTENT "${AGONES_CMAKELISTS_CONTENT}")
      set(AGONES_CMAKELISTS_CHANGED TRUE)
      message(STATUS "Agones CMakeLists: patch install appliqué")
    else()
      message(WARNING "Agones CMakeLists: bloc install original introuvable, patch non appliqué")
    endif()
  endif()

  if(AGONES_CMAKELISTS_CHANGED)
    # Restitue les fins de ligne CRLF si le fichier original en utilisait.
    string(FIND "${AGONES_CMAKELISTS_CONTENT_RAW}" "\r\n" AGONES_HAS_CRLF)
    if(AGONES_HAS_CRLF GREATER -1)
      string(REPLACE "\n" "\r\n" AGONES_CMAKELISTS_CONTENT "${AGONES_CMAKELISTS_CONTENT}")
    endif()

    file(WRITE "${AGONES_CMAKELISTS}" "${AGONES_CMAKELISTS_CONTENT}")
  endif()
endif()
