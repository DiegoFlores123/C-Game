El código de nuestro videojuego es un proyecto basado en Arduino que utiliza un ESP32, varios periféricos y bibliotecas como la pantalla LCD I2C, una tarjeta microSD, y un joystick para controlar el movimiento de un carro en una pantalla.

Objetivo del Juego
El jugador controla un carro que debe atropellar personas para sumar puntos y evitar obstáculos (picos) que reducen el tiempo. Además, puede recoger relojes que incrementan el tiempo disponible. El juego termina cuando se acaba el tiempo o el jugador alcanza el número máximo de puntos.

Características del Juego
1.	Control del Carro:
o	El carro es controlado mediante un joystick (conexiones a los pines VERT_PIN, HORZ_PIN, y SEL_PIN).
o	El jugador puede mover el carro en un área limitada de la pantalla (máximo 4 filas y 14 columnas).
o	El juego incluye la opción de activar un boost de velocidad de 0.1 (incremento temporal de la velocidad del carro) de 3 segundos al presionar el botón de selección del joystick. Se puede volver a usar después 15 segundos.
2.	Pantalla LCD:
o	El juego usa una pantalla LCD I2C para mostrar el carro, los obstáculos, y otros elementos (personas, relojes, picos).
o	Se utilizan caracteres personalizados (createChar()) para representar estos elementos:
	carro1 (Arriba y abajo) y carro2 (Izquierda y derecha): representan el carro.
	persona: personas que el jugador debe atropellar.
	reloj: relojes que suman tiempo.
	pico: picos que restan tiempo.
	borde: usado para mostrar bordes de ciertos elementos en la pantalla y separar los stats con el área de juego.
3.	Elementos en el Juego:
o	Personas: el objetivo es atropellarlas para obtener puntos. Aparecen aleatoriamente en la pantalla. Incrementan temporalmente la velocidad del carro si atropellas varios en cadena. La velocidad esta definida desde un inicio con un multiplicador de 1.0, que aumenta en 0.2 por cada monito atropellado, hasta un máximo de 2.0. 
o	Relojes: aumentan el tiempo disponible cuando se recogen en 4 segs.
o	Picos: disminuyen el tiempo cuando el carro los toca en 8 segs.
4.	Interfaz y Menú:
o	El juego cuenta con un menú inicial donde el jugador puede elegir entre comenzar el juego o revisar los puntajes más altos (high scores).
o	Para seleccionar entre todas las opciones del juego, se usa el botón de selección (BUTSEL_PIN).
o	Hay efectos de sonido activados por el buzzer para las acciones principales como cambio de opción en el menú, activación del boost, y ganar puntos.
o	Hay una música de inicio y otra que suena durante el juego.
o	Si el usuario quiere borrar todos los scores para almacenar nuevos, se presiona el botón de reinicio y pausa simultáneamente por 3 segundos. Esto es porque el juego tiene un limite de 20 scores guardados.
5.	Sistema de Puntuación y High Scores:
o	El sistema de puntuación cuenta las personas atropelladas y guarda los puntajes más altos en un archivo JSON (/highscores.json) en una tarjeta microSD.
o	Para seleccionar las letras a guardar o usar en tu nombre desde la interfaz de guardar scores, se usa el joystick de arriba abajo para cambiar de letras y el botón de selección (BUTSEL_PIN) para pasar entre letras.
o	Después de terminar el juego, el jugador puede ingresar su nombre (tres letras) para guardar su puntaje. Una vez seleccionada la 3er letra, se guarda tu puntaje.

6.	Pausa, Reinicio y Fin del Juego:
o	El jugador puede pausar el juego presionando el botón de pausa (BUTPAUSE_PIN) y despausar presionando el mismo botón.
o	El jugador puede reiniciar el juego presionando el botón de reinicio (BUTRES_PIN), regresándolo al menú.
o	El juego finaliza cuando se acaba el tiempo o si el jugador alcanza el objetivo (atropellar 50 personas).
Instrucciones para Jugar
1.	Inicio del Juego:
o	Al encender el dispositivo, se muestra el menú principal.
o	Usa el joystick para navegar entre las opciones del menú.
o	Selecciona "Jugar" para empezar el juego con el botón A.
o	Selecciona "Highscore" para ver los 4 puntajes más altos.
2.	Controles del Carro:
o	Usa el joystick para mover el carro en las cuatro direcciones.
o	Pulsa el botón del joystick para activar el boost, incrementando temporalmente la velocidad del carro.
3.	Objetivos:
o	Atropella personas para ganar puntos.
o	Recoge relojes para aumentar tu tiempo.
o	Evita los picos que disminuyen tu tiempo.
4.	Controles extras:
o	Puedes pausar y despausar con el botón P.
o	Puedes pausar y despausar con el botón R.
5.	Guardar Puntaje:
o	Al terminar el juego, introduce un nombre de tres letras usando el joystick y el botón A para ir seleccionando letras y para guardar tu puntaje.
o	El juego guarda los puntajes más altos en una tarjeta microSD.
6.	Opciones del Menú:
o	Jugar: Comienza una nueva partida.
o	HighScore: Revisa los puntajes más altos guardados.
o	Extra: Si quieres borrar todos los scores, deja presionado simultáneamente el botón P y R por 3 segundos.
Componentes Técnicos
•	Joystick: Controla el movimiento del carro.
•	Pantalla LCD I2C: Muestra el estado del juego y los elementos (carro, personas, picos, relojes).
•	Tarjeta microSD: Almacena los puntajes más altos.
•	Buzzer: Proporciona retroalimentación sonora al jugador (sonidos de selección, puntuación, y final del juego).
•	Módulo audio digital (Max-98357): Proporciona la funcionalidad de reproducir archivos .mp3 guardados en el SD para el soundtrack del juego.
