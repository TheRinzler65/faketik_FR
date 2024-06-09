# faketik

faketik génère et installe des "faux tickets" pour faire réapparaître des titres manquants sur les systèmes de la famille Nintendo 3DS.

Licence : MIT.

## Questions fréquemment posées

### Mon jeu n'est pas apparu / aucun titre à réparer n'a été trouvé !
La raison la plus probable est que le système ne sait pas que le titre existe.

Si un titre Nintendo DSiWare n'est pas apparu, c'est probablement parce qu'une ancienne image CTRNAND a été restaurée, ce qui signifie qu'un ancien fichier `/dbs/title.db` a été restauré ne contenant pas les informations du titre DSiWare. La méthode la plus simple pour résoudre ce problème est de sauvegarder les sauvegardes de TWLNAND, de supprimer et de réinstaller les titres.

### Le message "La base de données de titres externe n'est pas disponible." est apparu !
Cela se produit parce que `title.db` n'existe pas à l'endroit où la Nintendo 3DS tente de le charger (`/Nintendo 3DS/{id0}/{id1}/dbs/title.db`). La base de données doit être restaurée ou reconstruite. Les outils pour effectuer cela facilement n'existent pas encore, donc cela doit être fait un peu manuellement.

### Il installe une multitude de tickets 0000000000000000 / le message "Échec d'allocation de mémoire pour le tampon de ticket." est apparu !
Cela signifie que la base de données de titres externe n'existe pas. Ce problème a été corrigé dans la version 1.1.

### Mon jeu ne fonctionne pas !
Ce n'est pas dû à faketik. Il ne crée que des tickets pour faire réapparaître des titres. Il ne répare pas les titres cassés ou les graines manquantes.

Il existe un cas potentiel pour les titres DLC avec plus de 256 contenus, car le ticket actuel ne couvre que 256 bits dans l'index de contenu. La meilleure façon de résoudre ce problème est de supprimer le ticket et de réinstaller un ticket légitime depuis Nintendo eShop.

