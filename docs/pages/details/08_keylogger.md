\page keylog Keylogger
\tableofcontents

Dans le contexte d'Epirootkit, le keylogger est un outil essentiel pour surveiller les frappes au clavier. Il est utilisé pour capturer les entrées de l'utilisateur, ce qui peut être utile pour détecter des pattern de frappes d'identification ou pour surveiller l'activité de l'utilisateur.

## ⚙️ Fonctionnement technique

Le keylogger d’Epirootkit est implémenté sous forme de module noyau Linux, utilisant le système de notification clavier (`keyboard_notifier`) et l’interface `debugfs` pour exposer les frappes capturées.

### Architecture

- **Capture des frappes** :  
  Le module s’enregistre auprès du noyau via un `notifier_block` (`epikeylog_blk`). À chaque événement clavier (appui ou relâchement), la fonction de rappel epikeylog_callback() est appelée.
  ```c
  // Notifier block structure for keyboard events
  static struct notifier_block epikeylog_blk = {
      .notifier_call = epikeylog_callback,
  };
  ```
- **Traduction des codes touches** :  
  Les codes de touches (`keycode`) sont convertis en chaînes lisibles grâce à la fonction `keycode_to_string()`, qui s’appuie sur un tableau de correspondance (`char *keymap[][2]`) pour gérer les touches normales et avec majuscule.
- **Exposition via debugfs** :  
  Les frappes sont accessibles en lecture dans un fichier `keys` situé dans un répertoire caché sous `/sys/kernel/debug/`, créé dynamiquement à l’initialisation du module.

> **Note** : L'idée d'écrire dans le debugfs vient des différents projets vu sur Github et il est utilisé pour sa facilité d'utilisation et son api riche déjà présente dans le kernel Linux. L'objectif initial était de stocker les frappes clavier dans un fichier à la racine du répertoire caché du rootkit. Cependant, au moment de ces tests nous n'avons pas réussi à écrire dans un fichier de la manière dont nous parvenons à le faire ici avec un fichier debugfs. AInsi , pour protéger malgré tout le fichier, nous l'avons nommé avec le préfix `stdbool_bypassed_ngl_` qui est un préfix par défaut utilisé par les [hooks](#hooks-introduction) pour désigner les fichiers cachés par défaut. Ainsi, le fichier sera invisible pour un utilisateur lambda, mais accessible par le rootkit.

- **Envoi au serveur** :  
  La fonction `epikeylog_send_to_server` lit le contenu du tampon et l’envoie à un serveur distant via une fonction dédiée.

### Principales fonctions

| Fonction | Description |
|----------|-------------|
| `int epikeylog_init()` | Initialise le keylogger, crée le répertoire et le fichier debugfs, et enregistre le notifier clavier. |
| `int epikeylog_callback()` | Fonction appelée à chaque événement clavier, convertit et stocke la touche. |
| `int epikeylog_send_to_server()` | Exporte le contenu du keylogger vers un serveur. |
| `int epikeylog_exit()` | Nettoie les ressources et désactive le keylogger. |

### Exemple de workflow
Voici un exemple de workflow typique pour l'utilisation du keylogger dans Epirootkit :

1. **Activation** : Le keylogger est activé et `epikeylog_init` est appelé.
2. **Capture** : À chaque frappe, `epikeylog_callback` enregistre la touche dans le tampon.
3. **Consultation** : L'attaquant peut lire `/sys/kernel/debug/stdbool_bypassed_ngl_klg/keys` pour voir les frappes capturées.
4. **Export** : Le contenu peut être envoyé à un serveur via `epikeylog_send_to_server`.
5. **Désactivation** : Le keylogger est désactivé via `epikeylog_exit`. Cette désactivation supprime le fichier debugfs et désenregistre le notifier clavier.

<img 
  src="logo_no_text.png" 
  style="
    display: block;
    margin: 100px auto;
    width: 30%;
    overflow: hidden;
  "
/>