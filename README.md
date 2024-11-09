# aerographe-meteo
Panneau d'affichage de la meteo en temps réel.

Utilisation de LEDs adressables (type W2811 ou W2812 en 5V) pilotées par un esp32.

Ce type de leds fonctionnent bien en 3V3 sur esp32

les données (METAR) sont recueillies sur les services aeroportuaires.

On peut modifier les aéroports utilisés ici pour l'adapter

Les réglages sont personnels, à savoir :
Le rouge pour la pluie
le vert pour beau temp
le jaune nuage
l'orange pour averse
le bleu la neige
le blanc le brouillard
le violet la grèle
et pour le vent les leds clignotent
