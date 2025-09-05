# Crzgames - RCNETCore

## 🛠 Tech Stack
- C++ (Language)
- A C++ 17 compiler
- CI / CD (Github Actions)
- CMake (Build script)
- Compiler (GCC, CLANG, CL)

<br /><br /><br /><br />


## 📦 Versionning
On utilise la convention SemVer : https://semver.org/lang/fr/ <br /><br />
Pour une Release classique : MAJOR.MINOR.PATCH <br />
Pour une Pre-Release, exemples : MAJOR.MINOR.PATCH-rc.0 OR MAJOR.MINOR.PATCH-beta.3 <br /><br />

Nous utilison release-please de Google pour versionner, via Github Actions. <br />
Pour que cela sois pris en compte il faut utiliser les conventionnal commits : https://www.conventionalcommits.org/en/v1.0.0/ <br />
Release Please crée une demande d'extraction de version après avoir remarqué que la branche par défaut contient des « unités publiables » depuis la dernière version. Une unité publiable est un commit sur la branche avec l'un des préfixes suivants : `feat` et `fix`. <br /><br />

La première Release que créer release-please automatiquement est la version : 1.0.0 <br />
Pour créer une Pre-Release faire un commit vide, par exemple si on'ai à la version 1.0.0, on peut faire : 
```bash
git commit --allow-empty -m "chore: release 1.1.0-rc.0" -m "Release-As: 1.1.0-rc.0"
```

<br /><br /><br /><br />


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
│   ├── 📁 jwt-cpp                    # JWT
│   ├── 📁 nlohmann_json              # JSON
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

<br /><br /><br /><br />


## 📋 Plateformes supportées
| Platform | Architectures | System Version | Compatible |
|----------|---------------|----------------|------------|
| **Linux** | x64 | glibc 2.35+ | ✓ |
| **Windows** | x64 | Windows 10+ | ✓ |
| **Windows** | arm64 | macOS 15.0+ | ✓ |

<br /><br /><br /><br />


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
- Tous les modèles macOS Apple Silicon (M1, M2, M3, M4) et plus récent.

<br /><br /><br /><br />


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

<br /><br /><br /><br />


## 📦 Dépendances principales

| Librairie              | Utilisation principale                                       | Intégration                |
|------------------------|--------------------------------------------------------------|----------------------------|
| **AES**                | Implémentation légère d’AES-128 pour le chiffrement/déchiffrement, utilisée par la librairie `rres` | `Statique – Fichiers intégrés directement dans le code source, pas besoin de compilation séparée` |
| **LZ4**                | Compression/décompression ultra-rapide, utilisée par la librairie `rres` et le module `RC2D_data` | `Statique – Fichiers intégrés directement dans le code source, pas besoin de compilation séparée` |
| **Monocypher**         | 	Librairie de cryptographie moderne (hashs, signatures, échange de clés), utilisée par la librairie `rres` | `Statique – Fichiers intégrés directement dans le code source, pas besoin de compilation séparée` |
| **SDL3**               | Moteur principal, gestion entrée/sortie, rendu GPU           | `Obligatoire`                |
| **Crzgames_RCENet**             | Fork de ENet (Communication UDP)                             | `Activé par défault mais optionnel`, mais le module `RC2D_net` ne sera pas utilisable si désactiver. Passé à CMake : RC2D_NET_MODULE_ENABLED=OFF/ON |
| **OpenSSL**            | Hashing, Chiffrement, Compression..etc                       | `Activé par défault mais optionnel`, mais le module `RC2D_data` ne sera pas utilisable si désactiver. Passé à CMake : RC2D_DATA_MODULE_ENABLED=OFF/ON |
| **hiredis**            | Hashing, Chiffrement, Compression..etc                       | `Activé par défault mais optionnel`, mais le module `RC2D_data` ne sera pas utilisable si désactiver. Passé à CMake : RC2D_DATA_MODULE_ENABLED=OFF/ON |
| **Nats**            | Hashing, Chiffrement, Compression..etc                       | `Activé par défault mais optionnel`, mais le module `RC2D_data` ne sera pas utilisable si désactiver. Passé à CMake : RC2D_DATA_MODULE_ENABLED=OFF/ON |
| **jwt-cpp**            | Hashing, Chiffrement, Compression..etc                       | `Activé par défault mais optionnel`, mais le module `RC2D_data` ne sera pas utilisable si désactiver. Passé à CMake : RC2D_DATA_MODULE_ENABLED=OFF/ON |
| **nlohmann_json**            | Hashing, Chiffrement, Compression..etc                       | `Activé par défault mais optionnel`, mais le module `RC2D_data` ne sera pas utilisable si désactiver. Passé à CMake : RC2D_DATA_MODULE_ENABLED=OFF/ON |
| **cpp-httplib**            | Hashing, Chiffrement, Compression..etc                       | `Activé par défault mais optionnel`, mais le module `RC2D_data` ne sera pas utilisable si désactiver. Passé à CMake : RC2D_DATA_MODULE_ENABLED=OFF/ON |

<br /><br /><br /><br />


## ⚙️ Setup Environment
1. Clone project :
  ```bash
  git clone git@github.com:CrzGames/Crzgames_RCNET.git
  ```
2. (optional just for documentation rcnet) Download and Install Node.js >= 18.0.0
3. Run command :
  ```bash  
  # Linux - x64 :
  1. Requirements : glibc >= 2.35 (Ubuntu >= 22.04 OR Debian >= 12.0)
  2. Run command (replace debian for name) : sudo usermod -aG sudo debian
  3. Download and Install brew : /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
  4. Après l'installation de homebrew il faut importer les variables d'environnement et installer les deux librairies : 
    echo '# Set PATH, MANPATH, etc., for Homebrew.' >> /home/debian/.bashrc && 
    echo 'eval "$(/home/linuxbrew/.linuxbrew/bin/brew shellenv)"' >> /home/debian/.bashrc && 
    eval "$(/home/linuxbrew/.linuxbrew/bin/brew shellenv)" &&
    sudo apt-get install -y build-essential &&
    brew install gcc
  5. Download and Install CMake >= 3.25.0 : brew install cmake
  6. Run command for install dependencies : cmake -P cmake/setup_dependencies.cmake

  # macOS - arm64 :

  # Windows - x64 :
  ```
  
<br /><br /><br /><br />


## 🔄 Updating Dependencies
Pour mettre à jour une ou des dépendance :
1. Modifiez le tag/commit_sha dans `dependencies.txt` de la librairie souhaiter.
2. Exécutez le script à la racine du projet :
```bash
cmake -P cmake/setup_dependencies.cmake
```

<br /><br /><br /><br />


## 🔄 Cycle Development
1. Générer l'exemple pour tester l'executable
```bash
# Linux - x64
chmod +x ./build-scripts/generate-example/linux-x64.sh
./build-scripts/generate-example/linux-x64.sh

# Windows - x64
.\build-scripts\generate-example\windows-x64.bat

# macOS - arm64
chmod +x ./build-scripts/generate-example/macos-arm64.sh
./build-scripts/generate-example/macos-arm64.sh
```

<br /><br /><br /><br />


## Production
### ⚙️➡️ Automatic Distribution Process (CI / CD)
#### Si c'est un nouveau projet suivez les instructions : 
1. Ajoutées les SECRETS_GITHUB pour :
   - O2SWITCH_FTP_HOST
   - O2SWITCH_FTP_PASSWORD
   - O2SWITCH_FTP_PORT
   - O2SWITCH_FTP_USERNAME
   - PAT (crée un nouveau token si besoin sur le site de github puis dans le menu du "Profil" puis -> "Settings" -> "Developper Settings' -> 'Personnal Access Tokens' -> Tokens (classic))

<br /><br />

### ✋ Manual Distribution Process
1. Generate librarie RCNET for Release and Debug, run command :
```bash
# Linux - x64
chmod +x ./build-scripts/generate-lib/linux-x64.sh
./build-scripts/generate-lib/linux-x64.sh

# Windows - x64
.\build-scripts\generate-lib\windows-x64.bat

# macOS - arm64
chmod +x ./build-scripts/generate-lib/macos-arm64.sh
./build-scripts/generate-lib/macos-arm64.sh
```
2. Get librarie RCNET, steps for different systems :
```bash
# Linux - x64
1. Go directory 'dist/lib/linux/'
2. Go in directory 'Release' OR 'Debug'
3. Get librarie RCNET : librcnet_static.a

# Windows x64
1. Go directory 'dist/lib/windows/'
2. Go in directory 'Release' OR 'Debug'
3. Get librarie RCNET : rcnet_static.lib

# macOS - arm64
1. Go directory 'dist/lib/macos/'
2. Go in directory 'Release' OR 'Debug'
3. Get librarie RCNET : librcnet_static.a
```