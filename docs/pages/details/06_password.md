\page password Password
\tableofcontents

## 1. Password

Le fichier passwd.c assure le chargement, la vérification et la mise à jour sécurisée d’un mot de passe stocké sous forme de hachage SHA-256. La valeur par défaut du hachage est initialisée dans le tableau global `passwd_hash`, ce qui garantit qu’une configuration inexistante ne laisse pas le système dans un état indéfini. Le mot de passe par défaut est `evannounet`.
```c
u8 passwd_hash[PASSWD_HASH_SIZE] = {
    0x5e, 0x7e, 0x56, 0x44, 0xa5, 0xeb, 0xfd,
    0x8e, 0x3f, 0xd4, 0x2a, 0x26, 0xf1, 0x5b,
    0xe3, 0xe7, 0x16, 0x6a, 0xc0, 0x22, 0x53,
    0xb5, 0xb4, 0x2a, 0x99, 0x43, 0x11, 0xed,
    0x09, 0x54, 0x99, 0x9d
};
```

## 2. Chargement

## 3. Vérification

## 4. Changement

<img 
  src="logo_no_text.png" 
  style="
    display: block;
    margin: 100px auto;
    width: 30%;
    overflow: hidden;
  "
/>