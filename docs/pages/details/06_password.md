\page password Password
\tableofcontents

## Gestion {#gestion-du-mot-de-passe}

Une fois que le rootkit est connecté au serveur d'attaque, il est nécessaire de s'authentifier en entrant un mot de passe afin de pouvoir exécuter des commandes. L'implémentation du mot de passe utilise un hashage SHA-256 afin d'éviter de stocker le mot de passe en clair ou de l'hardcoder dans le code source. Le hash du mot de passe est comparé à une valeur de référence, et seul un mot de passe correct permet l'accès aux fonctionnalités sensibles.
- Le hash du mot de passe est stocké sous forme hexadécimale dans un fichier de configuration persistant, nommé `passwd.cfg`. Si ce fichier n'existe pas, le mot de passe par défaut est utilisé, en l'occurrence `evannounet`.
- Lors de la vérification, le mot de passe fourni est hashé avec SHA-256 puis comparé au hash stocké.
- La modification du mot de passe met à jour le hash dans le fichier de configuration, toujours sans jamais stocker le mot de passe en clair.

Cette approche renforce la sécurité en évitant l'exposition directe du mot de passe dans le code ou sur le disque.

## Exemple

```c
// Hash SHA-256 du mot de passe par défaut (exemple)
u8 access_code_hash[PASSWD_HASH_SIZE] = {
  0x5e, 0x7e, 0x56, 0x44, 0xa5, 0xeb, 0xfd, 0x8e, 0x3f, 0xd4, 0x2a,
  0x26, 0xf1, 0x5b, 0xe3, 0xe7, 0x16, 0x6a, 0xc0, 0x22, 0x53, 0xb5,
  0xb4, 0x2a, 0x99, 0x43, 0x11, 0xed, 0x09, 0x54, 0x99, 0x9d
};
```

> Le hash du mot de passe est stocké en mémoire et peut être ainsi directement utilisé pour la vérification. Il s'agit du hash SHA-256 du mot de passe par défaut `evannounet`.

```c
int passwd_verify(const char *password) {
  u8 digest[PASSWD_HASH_SIZE];
  int err;

  err = hash_string(password, digest);
  if (err < 0)
    return err;

  return are_hash_equals(digest, access_code_hash) ? 1 : 0;
}
```

> Lorsqu'un mot de passe est fourni, il est hashé puis comparé au hash de référence. L'accès est accordé uniquement si les deux valeurs correspondent.

```c
int passwd_set(const char *new_password) {
  u8 digest[PASSWD_HASH_SIZE];
  char hexout[PASSWD_HASH_SIZE * 2 + 2];
  int err, len;
  char cfgpath[256];

  err = hash_string(new_password, digest);
  if (err < 0)
    return err;

  // Mise à jour du hash en mémoire
  memcpy(access_code_hash, digest, PASSWD_HASH_SIZE);

  // Conversion du hash en chaîne hexadécimale et sauvegarde dans le fichier de configuration
  hash_to_str(digest, hexout);
  hexout[PASSWD_HASH_SIZE * 2] = '\n';
  hexout[PASSWD_HASH_SIZE * 2 + 1] = '\0';
  len = PASSWD_HASH_SIZE * 2 + 1;

  build_cfg_path(PASSWD_CFG_FILE, cfgpath, sizeof(cfgpath));

  // Écriture du hash dans le fichier de configuration
  return _write_file(cfgpath, hexout, len);
}
```

> Cette fonction permet de changer le mot de passe : le nouveau mot de passe est hashé, stocké en mémoire et sauvegardé dans le fichier de configuration sous forme hexadécimale.

Pour plus d’informations sur la procédure d’authentification et l’utilisation du mot de passe lors de la connexion, consultez la section [Connexion](#connexion). Cette section détaille le processus d’accès à l’interface web, la saisie du mot de passe (`evannounet` par défaut), ainsi que les étapes nécessaires pour accéder au tableau de bord principal après authentification.

> Pour plus de détail sur le changement de mot de passe, consultez la section [Liste des commandes](#liste-des-commandes) et particulièrement la commande `passwd`.

<img 
  src="logo_no_text.png" 
  style="
    display: block;
    margin: 100px auto;
    width: 30%;
    overflow: hidden;
  "
/>