# Usage

\tableofcontents

L’utilisation du rootkit peut se faire soit en ligne de commande, soit via l’interface graphique intégrée. Nous décrirons ici l’ensemble des commandes de base pour interagir à distance avec le rootkit, disponibles aussi bien en CLI chez l’attaquant que par des boutons ou des entrées de commandes dans l’interface web.

## 1. CLI

### help
```bash
help
```
Cette commande permet simplement d’afficher un menu récapitulatif de toutes les commandes disponibles pour l’attaquant. Certaines commandes affichées ont plus de sens et sont surtout utilisées dans le cadre de l’interface Web.

### connect
```bash
connect [PASSWORD]
```
Cette commande permet d’authentifier l’attaquant pour pouvoir accéder au rootkit à distance. Le mot de passe peut ensuite être changé avec la commande `passwd`.

### disconnect


### ping


### passwd


### exec


### klgon


## 2. Web

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