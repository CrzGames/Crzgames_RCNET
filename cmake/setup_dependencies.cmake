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

# ============================================================
# Patch Agones CMakeLists.txt pour compatibilité add_subdirectory(grpc)
# ============================================================
set(AGONES_CMAKELISTS "${VENDORED_DIR}/agones/sdks/cpp/CMakeLists.txt")

if(EXISTS "${AGONES_CMAKELISTS}")
  file(READ "${AGONES_CMAKELISTS}" AGONES_CMAKE_CONTENT)

  set(AGONES_OLD_BLOCK [=[
# gRPC
find_package(Protobuf REQUIRED)
find_package(gRPC CONFIG REQUIRED)
find_package(Threads REQUIRED)
]=])

  set(AGONES_NEW_BLOCK [=[
# gRPC
if(NOT TARGET protobuf::libprotobuf)
  find_package(Protobuf REQUIRED)
endif()

if(NOT TARGET gRPC::grpc++_unsecure)
  find_package(gRPC CONFIG REQUIRED)
endif()

if(NOT TARGET Threads::Threads)
  find_package(Threads REQUIRED)
endif()
]=])

  string(FIND "${AGONES_CMAKE_CONTENT}" "${AGONES_NEW_BLOCK}" AGONES_ALREADY_PATCHED_POS)
  if(NOT AGONES_ALREADY_PATCHED_POS EQUAL -1)
    message(STATUS "✅ Patché le CMakeLists.txt d'Agones déjà appliqué")
  else()
    string(FIND "${AGONES_CMAKE_CONTENT}" "${AGONES_OLD_BLOCK}" AGONES_OLD_BLOCK_POS)
    if(NOT AGONES_OLD_BLOCK_POS EQUAL -1)
      string(REPLACE "${AGONES_OLD_BLOCK}" "${AGONES_NEW_BLOCK}" AGONES_CMAKE_CONTENT "${AGONES_CMAKE_CONTENT}")
      file(WRITE "${AGONES_CMAKELISTS}" "${AGONES_CMAKE_CONTENT}")
      message(STATUS "✅ Patché le CMakeLists.txt d'Agones appliqué avec succès")
    else()
      message(WARNING "⚠️ Bloc gRPC attendu introuvable dans ${AGONES_CMAKELISTS} ; patch non appliqué")
    endif()
  endif()
endif()