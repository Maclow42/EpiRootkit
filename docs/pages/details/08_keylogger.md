\page keylog Keylogger
\tableofcontents

Dans le contexte d'Epirootkit, le keylogger est un outil essentiel pour surveiller les frappes au clavier. Il est utilisé pour capturer les entrées de l'utilisateur, ce qui peut être utile pour détecter des pattern de frappes d'identification ou pour surveiller l'activité de l'utilisateur.

## Fonctionnement technique

Le keylogger d’Epirootkit est implémenté sous forme de module noyau Linux, utilisant le système de notification clavier (`keyboard_notifier`) et l’interface `debugfs` pour exposer les frappes capturées.

### Architecture

- **Capture des frappes** :  
  Le module s’enregistre auprès du noyau via un `notifier_block` (`epikeylog_blk`). À chaque événement clavier (appui ou relâchement), la fonction de rappel `epikeylog_callback(struct notifier_block *nblock, unsigned long code, void *_param)` est appelée.
  ```c
  // Notifier block structure for keyboard events
  static struct notifier_block epikeylog_blk = {
      .notifier_call = epikeylog_callback,
  };
  ```
- **Traduction des codes touches** :  
  Les codes de touches (`keycode`) sont convertis en chaînes lisibles grâce à la fonction `keycode_to_string(int keycode, int shift_mask, char *buf)`, qui s’appuie sur un tableau de correspondance (`char *keymap[][2]`) pour gérer les touches normales et avec majuscule.
- **Stockage circulaire** :  
  Les frappes sont stockées dans un tampon circulaire (`keys_buf`) de taille `BUF_LEN`. Si le tampon est plein, il est réinitialisé pour éviter les débordements.
- **Exposition via debugfs** :  
  Les frappes sont accessibles en lecture dans un fichier `keys` situé dans un répertoire caché sous `/sys/kernel/debug/`, créé dynamiquement à l’initialisation du module.
- **Envoi au serveur** :  
  La fonction `epikeylog_send_to_server` lit le contenu du tampon et l’envoie à un serveur distant via une fonction dédiée.

### Principales fonctions

- `epikeylog_init` : Initialise le keylogger, crée le répertoire et le fichier debugfs, et enregistre le notifier clavier.
- `epikeylog_callback` : Fonction appelée à chaque événement clavier, convertit et stocke la touche.
- `epikeylog_send_to_server` : Exporte le contenu du keylogger vers un serveur.
- `epikeylog_exit` : Nettoie les ressources et désactive le keylogger.

### Exemple de flux

1. **Activation** : Le module est chargé et `epikeylog_init` est appelé.
2. **Capture** : À chaque frappe, `epikeylog_callback` enregistre la touche dans le tampon.
3. **Consultation** : Un utilisateur autorisé peut lire `/sys/kernel/debug/<prefix>klg/keys` pour voir les frappes.
4. **Export** : Le contenu peut être envoyé à un serveur via `epikeylog_send_to_server`.
5. **Désactivation** : Le module est déchargé via `epikeylog_exit`.

> **Note** : Ce keylogger fonctionne au niveau noyau, ce qui le rend difficile à détecter par des outils utilisateurs classiques.


<img 
  src="logo_no_text.png" 
  style="
    display: block;
    margin: 100px auto;
    width: 30%;
    overflow: hidden;
  "
/>