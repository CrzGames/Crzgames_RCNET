# Crzgames - RCNETCore

## 🛠 Tech Stack
- C++ (Language)
- A C++ 17 compiler
- CI / CD (Github Actions)
- CMake (Build script)
- Compiler (GCC, CLANG, CL)

<br /><br />

---

<br /><br />

## 📁 Project Structure

```
📦 Crzgames_RCNET
├── 📁 .github                        # Configuration GitHub (workflows CI/CD)
├── 📁 build-scripts                  # Scripts de build, puis les scripts utilise le CMakelists.txt
├── 📁 cmake                          
│   └── 📄 setup_dependencies.cmake   # Script CMake chargé de lire `dependencies.txt` et cloner/configurer les dépendances dans `/dependencies`
├── 📁 dependencies (git ignored)     # Répertoire local contenant les dépendances clonées (ignoré par Git pour ne pas polluer le repo)
│   ├── 📁 cJSON                      # JSON
│   ├── 📁 cpp-httplib                # HTTP/HTTPS
│   ├── 📁 Crzgames_Libraries         # Librairies précompilée (OpenSSL, RCENet, libsodium, agones, grpc)
│   ├── 📁 Crzgames_RC2D              # Librairie client pour l'exemple du client
│   ├── 📁 nats                       # Broker message
├── 📁 docs                           # Documentation du moteur de serveur (pages Markdown, auto-générées)
├── 📁 example-client                 # Exemple d'un client
├── 📁 example-server                 # Exemple d'un serveur
├── 📁 include                        # En-têtes publics exposés aux utilisateurs de la lib (API du moteur de serveur)
├── 📁 src                            # Code source interne de la bibliothèque RCNET (implémentations .c)
├── 📄 .gitignore                     # Fichiers/dossiers à ignorer par Git (ex: /dependencies, builds temporaires)
├── 📄 CHANGELOG.md                   # Historique des versions avec les modifications apportées à chaque release
├── 📄 CMakeLists.txt                 # Point d’entrée de la configuration CMake
├── 📄 dependencies.txt               # Fichier listant les dépendances à cloner (format : nom=repo:version)
├── 📄 README.md                      # Page d’accueil du dépôt (description, installation, exemples d’usage)
├── 📄 release-please-config.json     # Configuration pour `release-please` (outil Google de génération automatique de releases)
├── 📄 version.txt                    # Contient la version actuelle du moteur de serveur (utilisé dans le build ou les releases)

```

<br /><br />

---

<br /><br />

## 📋 Plateformes supportées
- 🟢 supporté
- 🟡 en cours
- 🔴 non supporté

| Platform | Architectures | System Version | Compatible |
|----------|---------------|----------------|------------|
| **Windows** | x64 | Windows 10+ | 🟢 |
| **macOS** | Apple Silicon arm64 | macOS 15.0+ | 🟢 |
| **Linux** | x64/arm64 | glibc 2.35+ | 🟢 |

<br /><br />

---

<br /><br />

## 📱 Appareils compatibles par plateforme

### **Linux (glibc 2.35+)**
- Ubuntu 22.04 et plus récent.
- Debian 12 et plus récent.
- Fedora 36 et plus récent.
- Linux Mint 21 et plus récent.
- elementary OS 7 et plus récent.
- CentOS/RHEL 10 et plus récent.

### **Windows (10+)**
- Windows 10 et plus récent.

### **macOS (15.0+)**
- Tous les modèles macOS Apple Silicon (M1, M2, M3, M4, M5) et plus récent.

<br /><br />

---

<br /><br />

## 🎯 Raisons techniques des versions minimales et autres par plateforme

### Linux x64/arm64
- **Version minimale** : glibc 2.35+
- **Raison** :
  - CI/CD basée sur Ubuntu 22.04 LTS (donc librairie RCNET + dépendences construite sur glibc 2.35)

### Windows x64
- **Version minimale** : Windows 10+
- **Raison** :

### macOS arm64
- **Version minimale** : macOS 15.0+ / M1+
- **Raison** :

<br /><br />

---

<br /><br />

## 📦 Dépendances principales

> Les versions sont verrouillées afin de garantir des builds reproductibles sur toutes les plateformes.

| Librairie | Version / Commit SHA utilisé par RCNET | Rôle dans RCNET | Statut / Intégration
|------------|----------------------------------------|----------------|----------------------|
| **LZ4** | v1.10.0 | Compression des packets UDP | ⭐ Obligatoire |
| **cJSON** | v1.7.19 | JSON | ⭐ Obligatoire |
| **RCENet** | v1.6.1 | Communication réseau UDP (fork ENet) | ⭐ Obligatoire |
| **OpenSSL** | v3.6.1 | cpp-httplib, nats client c et grpc à besoin de OpenSSL >= 3.x.x | ⭐ Obligatoire |
| **cpp-httplib** | v0.38.0 | HTTP/HTTPS | ⭐ Obligatoire |
| **nats** | v3.12.0 | Communication inter services | ⭐ Obligatoire |
| **libsodium** | v1.0.21 | Crypto | ⭐ Obligatoire |
| **agones sdk client** | v1.56.0 | Utiliser dans le cluster kubernetes entre le serveur de jeu et kubernetes | ⭐ Obligatoire |
| **grpc** | v1.76.0 | agones à besoin de grpc == 1.76.0 | ⭐ Obligatoire |

<br /><br />

---

<br /><br />

## ⚙️ Setup Environment Development
1. Cloner le projet :
  ```bash
  git clone git@github.com:CrzGames/Crzgames_RNET.git
  ```
2. Steps by Platform :
  ```bash  
  # Windows (x64) :
  1. Requirements : Windows >= 10.
  2. Download and Install Visual Studio == 2022 (composant MSVC == v143 + composant Windows SDK >= 10) : https://visualstudio.microsoft.com/fr/downloads/
  3. Download and Install CMake >= 3.28.0 : https://cmake.org/download/ and add PATH ENVIRONMENT.
  4. Activer le support long path dans Windows (Powershell en adminstrateur) : 
     reg add HKLM\SYSTEM\CurrentControlSet\Control\FileSystem /v LongPathsEnabled /t REG_DWORD /d 1 /f
  5. Activer long paths dans Git :
     git config --global core.longpaths true
  6. Pour utilisé Agones le temps du développment en local voir : https://github.com/CrzGames/Crzgames_Agones-Quilkin_Documentation/blob/main/docs/07-dev-local-out-of-cluster.md



  # Linux (x64/arm64) :
  1. Requirements : glibc >= 2.35.0 (Exemple : Ubuntu >= 22.04 OR Debian >= 12.0), checker via : ldd --version
  2. Download and Install (gcc, g++, make..) :
     sudo apt update
     sudo apt install -y build-essential
  3. Download and Install CMake >= 3.28.0 : sudo apt install -y cmake
  4. Download and Install Patchelf (pour l exemple du client qui utilise RC2D) : sudo apt install -y patchelf
  5. Download and Install dev dependencies for SDL3 (pour l exemple du client qui utilise RC2D) :
    sudo apt-get update
    sudo apt-get -y install git make \
    pkg-config cmake ninja-build gnome-desktop-testing libasound2-dev libpulse-dev \
    libaudio-dev libfribidi-dev libjack-dev libsndio-dev libx11-dev libxext-dev \
    libxrandr-dev libxcursor-dev libxfixes-dev libxi-dev libxss-dev libxtst-dev \
    libxkbcommon-dev libdrm-dev libgbm-dev libgl1-mesa-dev libgles2-mesa-dev \
    libegl1-mesa-dev libdbus-1-dev libibus-1.0-dev libudev-dev libthai-dev \
    ibpipewire-0.3-dev libwayland-dev libdecor-0-dev liburing-dev
  6. Pour utilisé Agones le temps du développment en local voir : https://github.com/CrzGames/Crzgames_Agones-Quilkin_Documentation/blob/main/docs/07-dev-local-out-of-cluster.md


  # macOS (Apple Silicon arm64) :
  1. Requirements : MacOS X >= 15.0.0
  2. Download and Install xCode >= 16.4.0
  3. Download and Install Command Line Tools : xcode-select --install
  4. Download and Install brew : /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
  5. Download and Install CMake >= 3.28.0 : brew install cmake
  6. Pour utilisé Agones le temps du développment en local voir : https://github.com/CrzGames/Crzgames_Agones-Quilkin_Documentation/blob/main/docs/07-dev-local-out-of-cluster.md
```
  
3. Avant toute compilation, exécute le script suivant :

```bash
cmake -P cmake/setup_dependencies.cmake
```

Ce script va :
- Lire `dependencies.txt`
- Cloner chaque dépôt dans `dependencies/`
- Faire un `git reset --hard` au commit_sha/tag fourni
- Initialiser les sous-modules si présents dans les librairies cloner

<br /><br />

---

<br /><br />

## 🔄 Updating Dependencies
Pour mettre à jour une ou des dépendance :
1. Modifiez le tag/commit_sha dans `dependencies.txt` de la librairie souhaiter.
2. Exécutez le script à la racine du projet :
```bash
cmake -P cmake/setup_dependencies.cmake
```

<br /><br />

---

<br /><br />

## 🧱 Générer RCNET (lib statique) + Projet d'exemple Client/Serveur
0. **Si vous utilisez Agones en local** : lancez le SDK server avant de démarrer votre GameServer.  
   Si vous n'utilisez pas Agones, passez directement à l'étape 1.

```bash
# Linux - x64
chmod +x ./build-scripts/run-agones-sdk-server/linux-x64.sh
./build-scripts/run-agones-sdk-server/linux-x64.sh


# Linux - arm64
chmod +x ./build-scripts/run-agones-sdk-server/linux-arm64.sh
./build-scripts/run-agones-sdk-server/linux-arm64.sh


# macOS - Apple Silicon arm64
chmod +x ./build-scripts/run-agones-sdk-server/macos-arm64.sh
./build-scripts/run-agones-sdk-server/macos-arm64.sh


# Windows - x64
.\build-scripts\run-agones-sdk-server\windows-x64.bat
```

1. **Première fois uniquement** : utilisez les scripts `generate-project` pour **générer le projet CMake** dans `./build/` puis faire un build initial.

```bash
# Linux - x64
chmod +x ./build-scripts/generate-project/linux-x64.sh
./build-scripts/generate-project/linux-x64.sh


# Linux - arm64
chmod +x ./build-scripts/generate-project/linux-arm64.sh
./build-scripts/generate-project/linux-arm64.sh


# macOS - Apple Silicon arm64
chmod +x ./build-scripts/generate-project/macos-arm64.sh
./build-scripts/generate-project/macos-arm64.sh


# Windows - x64
.\build-scripts\generate-project\windows-x64.bat
```

2. **Développement quotidien (après génération initiale)** : utilisez les scripts du dossier `build-project-development-debug`.
   - Sans argument : build en mode **Debug** des 3 targets par défaut :
     - `rcnet`
     - `rcnet_example_server`
     - `rcnet_example_client`
   - Avec argument : build en mode **Debug** d'une target spécifique uniquement (plus rapide).

```bash
# Linux - x64 (3 targets par défaut)
chmod +x ./build-scripts/build-project-development-debug/linux-x64.sh
./build-scripts/build-project-development-debug/linux-x64.sh

# Linux - x64 (target spécifique)
./build-scripts/build-project-development-debug/linux-x64.sh rcnet_example_server


# Linux - arm64 (3 targets par défaut)
chmod +x ./build-scripts/build-project-development-debug/linux-arm64.sh
./build-scripts/build-project-development-debug/linux-arm64.sh

# Linux - arm64 (target spécifique)
./build-scripts/build-project-development-debug/linux-arm64.sh rcnet_example_server


# macOS - arm64 (3 targets par défaut)
chmod +x ./build-scripts/build-project-development-debug/macos-arm64.sh
./build-scripts/build-project-development-debug/macos-arm64.sh

# macOS - arm64 (target spécifique)
./build-scripts/build-project-development-debug/macos-arm64.sh rcnet_example_client


# Windows - x64 (3 targets par défaut)
.\build-scripts\build-project-development-debug\windows-x64.bat

# Windows - x64 (target spécifique)
.\build-scripts\build-project-development-debug\windows-x64.bat rcnet_example_server
```

3. **Quand relancer `generate-project` (ou rerun CMake)** :
   - Si vous modifiez des options CMake / flags / dépendances (activation de module, ajout de libs, changement de toolchain, mise à jour `dependencies.txt`, etc.).
   - Si vous supprimez le dossier `build/` ou changez de plateforme/architecture/générateur.

4. Dossier de sortie (exemple Windows x64) :
```bash
# Projet Visual Studio généré :
.\build\windows\x64

# Binaries générés :
Debug   : .\build\windows\x64\Debug
Release : .\build\windows\x64\Release
```

<br /><br />

---

<br /><br />

## Production
### ⚙️➡️ Automatic Distribution Process (CI / CD)
#### Si c'est un nouveau projet suivez les instructions : 
1. Ajoutées les SECRETS_GITHUB pour :
   - ... TODO
   - PAT (crée un nouveau token si besoin sur le site de github puis dans le menu du "Profil" puis -> "Settings" -> "Developper Settings' -> 'Personnal Access Tokens' -> Tokens (classic))

