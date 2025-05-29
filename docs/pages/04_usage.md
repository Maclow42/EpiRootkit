# Usage
\tableofcontents

L’utilisation du rootkit peut se faire soit en ligne de commande, soit via l’interface graphique intégrée. Nous décrirons ici l’ensemble des commandes de base pour interagir à distance avec le rootkit, disponibles aussi bien en CLI chez l’attaquant que par des boutons ou des entrées de commandes dans l’interface web. Pour utiliser cette section de manière optimale, veuillez d'abord vous assurer que la mise en place a été correctement effectuée, comme expliqué précédemment.

## 1. ⌨️ CLI

### 1. help
```bash
help
```
Cette commande permet simplement d’afficher un menu récapitulatif de toutes les commandes disponibles pour l’attaquant. Certaines commandes affichées ont plus de sens et sont surtout utilisées dans le cadre de l’interface Web.

### 2. connect
```bash
connect [PASSWORD]
```
Cette commande permet d’authentifier l’attaquant pour pouvoir accéder au rootkit à distance. Le mot de passe peut ensuite être changé avec la commande `passwd`.

### 3. disconnect
```bash
disconnect
```
Cette commande est le complément de `connect` : elle permet de se déconnecter du rootkit distant. Il faudra donc se reconnecter pour pouvoir entrer de nouvelles commandes.

### 4. ping
```bash
ping
```
Cette commande permet de tester la connectivité du rootkit. Si la connexion est établie, la console devrait renvoyer `pong`.

### 5. passwd
```bash
passwd [PASSWORD]
```
Cette commande permet de changer le mot de passe actuellement utilisé pour se connecter au rootkit. Une fois le mot de passe modifié, il sera haché et stocké en interne sur la machine victime.

### 6. exec
```bash
exec [OPTIONS] [COMMAND]
```
Cette commande permet d'exécuter du code Bash dans l’espace utilisateur (userland) de la machine victime. Par défaut, elle renvoie le contenu de *stdout*, *stderr* ainsi que le code de sortie. Pour éviter cette sortie, il est possible d’ajouter l’option `-s`. Une fois exécutée, la commande se comporte comme si le code Bash était directement saisi dans le terminal de la victime.

#### Exemples
```bash
exec ls
exec man man
exec -s whoami
```

### 7. klgon
```bash
klgon
```

### 8. klgoff
```bash
klgoff
```

### 9. klg
```bash
klg
```

### 10. getshell
```bash
getshell
```

### 11. killcom
```bash
killcom
```
Cette commande est relativement intrusive : elle coupe la communication avec le rootkit et supprime le module via `rmmod`. Elle est principalement utilisée à des fins de test et de développement, car en conditions réelles, on ne souhaiterait pas nécessairement détruire le module.

### 12. hide_module
```bash
hide_module
```
Cette commande permet de masquer le module noyau en le retirant de la liste chaînée des modules maintenue par le noyau Linux, le rendant ainsi indétectable par les outils système classiques.

### 13. unhide_module
```bash
unhide_module
```
Cette commande est l’inverse de la précédente : elle permet de rétablir un module précédemment masqué en le réinsérant dans la liste des modules du noyau.

### 14. get_file

### 15. upload

### 16. sysinfo

### 17. is_in_vm
```bash
is_in_vm
```
Cette commande permet de détecter si le rootkit s'exécute dans un environnement virtualisé, tel qu’un hyperviseur ou un logiciel de virtualisation.

### 18. hooks


## 2. 🌐 Web

<img 
  src="logo_no_text.png" 
  style="
    display: block;
    margin: 100px auto;
    width: 30%;
    overflow: hidden;
  "
/>

<div class="section_buttons">

| Previous                          | Next                               |
|:----------------------------------|-----------------------------------:|
| [Architecture](03_archi.md)       |[Environnement](05_env.md)          |
</div>