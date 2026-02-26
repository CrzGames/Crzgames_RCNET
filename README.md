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
│   ├── 📁 Crzgames_Libraries         # Librairies précompilées (OpenSSL, hiredis, nats) propres à Crzgames
│   ├── 📁 SDL                        # SDL3 
│   ├── 📁 hiredis                    # Redis
│   ├── 📁 Nats                       # Nats broker de message
│   ├── 📁 cJSON                      # JSON
│   ├── 📁 cpp-httplib                # HTTP
├── 📁 docs                           # Documentation du moteur de serveur (pages Markdown, auto-générées)
├── 📁 example                        # Exemples d’utilisation du moteur de serveur de jeu RCNET (projets de démo, test de fonctionnalités)
├── 📁 include                        # En-têtes publics exposés aux utilisateurs de la lib (API du moteur de serveur)
├── 📁 src                            # Code source interne de la bibliothèque RCNET (implémentations .c)
├── 📁 tests                          # Tests unitaires (avec Criterion) pour vérifier les modules du moteur
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
| **Linux** | x64 | glibc 2.35+ | 🟢 |

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

### Linux
- **Version minimale** : glibc 2.35+
- **Raison** :
  - CI/CD basée sur Ubuntu 22.04 LTS (donc librairie RCNET + dépendences construite sur glibc 2.35)

### Windows
- **Version minimale** : Windows 10+
- **Raison** :

### macOS
- **Version minimale** : macOS 15.0+ / M1+
- **Raison** :

<br /><br />

---

<br /><br />

## 📦 Dépendances principales

> Les versions sont verrouillées afin de garantir des builds reproductibles sur toutes les plateformes.

| Librairie | Version / Commit SHA utilisé par RCNET | Rôle dans RCNET | Statut / Intégration
|------------|----------------------------------------|----------------|----------------------|
| **LZ4** | v1.10.0 | Compression des packets UDP | ⭐ Obligatoire (intégré statiquement) |
| **cJSON** | v1.7.19 | JSON | ⭐ Obligatoire (intégré statiquement) |
| **SDL3** | commit `29ca920fdf7ccfd0a73f4cb3427bb84815f9b7f3` | Gestion des thread | ⭐ Obligatoire |
| **RCENet** | v1.4.6 | Communication réseau UDP (fork ENet) | ⭐ Obligatoire |
| **OpenSSL** | v3.6.1 | Hashing, chiffrement, crypto + cpp-httplib à besoin de OpenSSL >= 3.x.x | ⭐ Obligatoire |
| **cpp-httplib** | v1.34.0 | HTTP/HTPS | ⭐ Obligatoire |
| **NATS** | v3.12.0 | Communication inter services | ⭐ Obligatoire |
| **hiredis** | v1.3.0| Base de donnée en mémoire | ⭐ Obligatoire |

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
  1. Requirements : Windows >= 10
  2. Download and Install Visual Studio == 2022 (MSVC >= v143 + Windows SDK >= 10) : https://visualstudio.microsoft.com/fr/downloads/
  3. Download and Install CMake >= 3.28.0 : https://cmake.org/download/ and add PATH ENVIRONMENT.
  4. Activer le support long path dans Windows (Powershell en adminstrateur) : 
     reg add HKLM\SYSTEM\CurrentControlSet\Control\FileSystem /v LongPathsEnabled /t REG_DWORD /d 1 /f
  5. Activer long paths dans Git :
     git config --global core.longpaths true



  # Linux (x64) :
  1. Requirements : glibc >= 2.35.0 (Exemple : Ubuntu >= 22.04 OR Debian >= 12.0), checker via : ldd --version
  2. Download and Install (gcc, g++, make..) :
     sudo apt update
     sudo apt install -y build-essential
  3. Download and Install CMake >= 3.28.0 : sudo apt install -y cmake
  4. Download and Install dev dependencies for SDL3 :
    sudo apt-get update
    sudo apt-get -y install build-essential git make \
    pkg-config cmake ninja-build gnome-desktop-testing libasound2-dev libpulse-dev \
    libaudio-dev libfribidi-dev libjack-dev libsndio-dev libx11-dev libxext-dev \
    libxrandr-dev libxcursor-dev libxfixes-dev libxi-dev libxss-dev libxtst-dev \
    libxkbcommon-dev libdrm-dev libgbm-dev libgl1-mesa-dev libgles2-mesa-dev \
    libegl1-mesa-dev libdbus-1-dev libibus-1.0-dev libudev-dev libthai-dev \
    ibpipewire-0.3-dev libwayland-dev libdecor-0-dev liburing-dev



  # macOS (Apple Silicon arm64) :
  1. Requirements : MacOS X >= 15.0.0
  2. Download and Install xCode >= 16.4.0
  3. Download and Install Command Line Tools : xcode-select --install
  4. Download and Install brew : /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
  5. Download and Install CMake >= 3.28.0 : brew install cmake
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

## 🧱 Générer RCNET (lib statique) + Projet d'exemple
1. **Par défaut** : ces scripts **génèrent un projet CMake** dans `./build/`, puis **compilent RCNET en bibliothèque statique** et **construisent le projet d’exemple** pour la plateforme choisie.

   - ✅ **Si le projet est déjà généré** (ex: solution **Visual Studio 2022**, projet Xcode, Ninja, etc.) : vous pouvez simplement **recompiler depuis votre IDE** ou via votre outil de build (Build/Run) **sans relancer les scripts**, tant que la configuration CMake ne change pas.

   - 🔁 **Quand relancer les scripts (ou rerun CMake)** :
     - Si vous modifiez des options CMake / flags / dépendances (ex: activation d’un module, ajout de libs, changement de toolchain, mise à jour `dependencies.txt`, etc.)
     - Si vous supprimez le dossier `build/` ou changez de plateforme/architecture/générateur.

   - 🧩 **Qu’est-ce qui demande une recompilation ?**
     - Si vous modifiez `src/RCNET/**` ou `include/RCNET/**` → vous modifiez la **lib RCNET** → **recompiler RCNET** (IDE ou scripts).
     - Si vous modifiez `example/src/**` ou `example/include/**` → vous modifiez **l’exemple** → **recompiler l’exemple** (IDE ou scripts).

```bash
# Linux - x64
chmod +x ./build-scripts/generate-project/linux-x64.sh
./build-scripts/generate-project/linux-x64.sh


# macOS - Apple Silicon arm64
chmod +x ./build-scripts/generate-project/macos-arm64.sh
./build-scripts/generate-project/macos-arm64.sh


# Windows - x64
.\build-scripts\generate-project\windows-x64.bat
```
3. Il y a un dossier `build` à la racine qui est générer.
```bash
# Pour Windows x64 par exemple, un projet Visual Studio 2022 à été générer au path suivant :
.\build\windows\x64

# La librairie RCNET static + l'exemple générer dans le même dossier :
Release : .\build\windows\x64\Debug
Debug : .\build\windows\x64\Release
```
4. Ouvrir le projet générer dans votre IDE favoris.

<br /><br />

---

<br /><br />

## Production
### ⚙️➡️ Automatic Distribution Process (CI / CD)
#### Si c'est un nouveau projet suivez les instructions : 
1. Ajoutées les SECRETS_GITHUB pour :
   - ... TODO
   - PAT (crée un nouveau token si besoin sur le site de github puis dans le menu du "Profil" puis -> "Settings" -> "Developper Settings' -> 'Personnal Access Tokens' -> Tokens (classic))