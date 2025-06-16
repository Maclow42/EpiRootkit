\page transfer Transfert
\tableofcontents

Le rootkit intègre un mécanisme complet de **transfert de fichiers** entre la machine attaquante et la machine victime. Ce système permet d’exfiltrer et d’injecter des fichiers de manière rapide, simple et discrète. L’ensemble est contrôlable en quelques clics depuis l’interface graphique. Deux fonctionnalités sont proposées :
- *download* : exfiltration d’un fichier depuis la victime
- *upload* : injection d’un fichier depuis l’attaquant vers la victime

## 1. 🛠️ Présentation

Le système repose sur deux composantes complémentaires :
- une **interface web** permettant à l’attaquant de naviguer, envoyer ou recevoir des fichiers,
- une partie **noyau**, capable de lire ou écrire sur le système de fichiers distant de manière totalement furtive.

Le protocole est simple :
1. Une commande est envoyée au rootkit via TCP (`upload` ou `download`),
2. Le rootkit prépare le transfert et répond `READY`,
3. Le fichier est ensuite transmis de manière encodée (hexadécimal) par paquets,
4. Une fois terminé, le module libère la mémoire utilisée.

Les transferts utilisent un format hexadécimal pour assurer une compatibilité réseau maximale tout en simplifiant l’analyse et le traitement.

```c
// === UPLOAD ===
upload_handler("/tmp/payload.bin 2048", TCP);
// => READY
handle_upload_chunk(data_chunk1, 1024, TCP);
handle_upload_chunk(data_chunk2, 1024, TCP);
// => Fichier écrit sur disque par le rootkit

// === DOWNLOAD ===
download_handler("/etc/passwd", TCP);
// => SIZE 1234
download("READY");
// => Contenu envoyé en hexadécimal
```

## 2. 📤 Téléversement

### 2.1 Interface web

La partie upload du système de transfert de fichiers permet de choisir un fichier local et de spécifier son chemin cible sur la victime.  
Une fois validé, l’interface web Flask :
- lit le fichier en mémoire,
- prépare une commande `upload <remote_path> <size>`,
- envoie le fichier en **chunks successifs** dès réception du mot-clé `READY` du rootkit.

Chaque étape est contrôlée et confirmée via un système de messages.

### 2.2 Rootkit

À la réception de la commande `upload`, le rootkit :
- alloue dynamiquement un buffer mémoire,
- initialise le chemin cible,
- répond `READY` à l’interface Flask,
- reçoit ensuite les données via `handle_upload_chunk()` en plusieurs morceaux jusqu’à ce que la taille annoncée soit atteinte.

Une fois le fichier complet, il est écrit sur disque, et les ressources sont automatiquement nettoyées.

```c
// upload_handler : initialise l’upload
int upload_handler(char *args, enum Protocol protocol) {
    // Parse le chemin et la taille
    // Alloue la mémoire avec vmalloc
    // Répond "READY" si tout est prêt
}

// handle_upload_chunk : reçoit les données
int handle_upload_chunk(const char *data, size_t len, enum Protocol protocol) {
    // Copie les chunks dans le buffer
    // Une fois complet : écrit le fichier sur disque
}
```

## 3. 📥 Téléchargement

### 3.1 Interface web

Le téléversement inversé (download) est déclenché depuis l’interface graphique à travers l'explorateur de fichiers.  
Une fois lancée :
- le rootkit envoie un message `SIZE <octets>`,
- l’interface répond `READY`,
- le fichier est reçu et enregistré dans un dossier sécurisé (`downloads`).

### 3.2 Rootkit

Lors d’un téléchargement, le rootkit :
- ouvre le fichier demandé,
- lit entièrement son contenu en mémoire,
- encode le tout en hexadécimal,
- attend la commande `READY` pour lancer le transfert vers l’attaquant.

Le tout est effectué de manière silencieuse, sans logs visibles ni traces dans les systèmes de fichiers utilisateurs.

```c
// download_handler : prépare la lecture
int download_handler(char *args, enum Protocol protocol) {
    // Ouvre le fichier
    // Lit son contenu en mémoire
    // Répond avec "SIZE <taille>"
}

// download : envoie le fichier après "READY"
int download(const char *command) {
    // Encode en hexadécimal
    // Envoie le buffer via send_to_server_raw
    // Libère la mémoire allouée
}
```

## 4. 🗂️ Explorateur de fichiers

La page `/explorer` de l’interface web permet de naviguer à distance dans le **système de fichiers de la victime**, en s’appuyant sur des commandes `ls` successives envoyées via le rootkit. L’exploration n’est pas persistante : à chaque requête, une commande est envoyée au rootkit pour lister le contenu du répertoire actuel. C’est uniquement lorsqu’on utilise le **reverse shell** que l'envoi de commandes devient persistant.

Le chemin courant est maintenu côté interface (frontend) afin de reconstituer une expérience de navigation cohérente. Chaque clic sur un dossier envoie une nouvelle commande `ls <chemin>` au rootkit, qui retourne la liste des fichiers ou sous-dossiers présents à cet emplacement.

Cette fonctionnalité permet à l’attaquant de :
- repérer rapidement les fichiers intéressants sur la victime,
- initier un téléchargement (`download`) ou un upload vers un répertoire spécifique,
- analyser la structure du système distant sans laisser de trace apparente côté utilisateur.

Un historique des tranferts successifs des fichiers est également disponible.

## 5. 🔐 Sécurité

Tous les échanges réseau se font via le canal TCP déjà chiffré (AES). L’utilisation d’un format hexadécimal permet d’éviter les problèmes de transport binaire tout en simplifiant le traitement côté rootkit. Les transferts sont atomiques : un seul fichier à la fois, avec contrôle de taille, accusé de réception et gestion mémoire stricte.

## 6. 💡 Pistes d’amélioration

- Ajout d’un checksum (SHA256) pour vérifier l’intégrité
- Compression légère (gzip, LZ4) pour réduire la taille des transferts

<img 
  src="logo_no_text.png" 
  style="
    display: block;
    margin: 100px auto;
    width: 30%;
    overflow: hidden;
  "
/>
