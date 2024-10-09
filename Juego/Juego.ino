#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <TaskScheduler.h>
#include <ArduinoJson.h>
#include <SD.h>
#include <SPI.h>
#include <Audio.h>
#include <FS.h>

uint8_t carro1[] = {
  B00000,
  B00100,
  B01110,
  B00100,
  B00100,
  B01110,
  B00100,
  B00000
};

uint8_t carro2[] = {
  B00000,
  B00000,
  B00000,
  B01110,
  B01010,
  B11111,
  B11111,
  B01010
};

uint8_t persona[] = { 
  B01110,
  B01110,
  B00100,
  B11111,
  B00100,
  B00100,
  B01010,
  B10001
};

uint8_t reloj[] = {  
  B01110,
  B10101,
  B10101,
  B10101,
  B10011,
  B10001,
  B01110,
  B00000
};

uint8_t pico[] = {  
  B00000,
  B00000,
  B00000,
  B00000,
  B00000,
  B00100,
  B01110,
  B11111
};

uint8_t borde[] = {  
  B01110,
  B01110,
  B01110,
  B01110,
  B01110,
  B01110,
  B01110,
  B01110
};

LiquidCrystal_I2C lcd (0x27, 20, 4);

#define VERT_PIN 32
#define HORZ_PIN 35
#define SEL_PIN 17
#define BUTPAUSE_PIN 16
#define BUTSEL_PIN 4
#define BUTRES_PIN 14
#define BUZ_PIN 2

// microSD Card Reader connections
#define SD_CS          5
#define SPI_MOSI      23 
#define SPI_MISO      19
#define SPI_SCK       18

// I2S Connections
#define I2S_DOUT      15
#define I2S_BCLK      26
#define I2S_LRC       25

int arriba = 10;  // Posición inicial del carro en X
int abajo = 3;   // Posición inicial del carro en Y
float velocidad = 1.0;
int timer = 30;  // 30 segundos para el temporizador
unsigned long ultimaColision = 0;  // Tiempo de la última colisión con una persona
bool gameOver = false;
bool juegoIniciado = false;
bool boostActivo = false;
unsigned long boostStart = 0;
unsigned long boostCooldown = 0; // Tiempo de cooldown para el boost
unsigned long tiempoInicioJuego = 0;  // Tiempo de inicio del juego para controlar la generación de picos
bool cuentaRegresivaCompletada = false;  // Nuevo estado para la cuenta regresiva
bool carritoImprimido = false;  // Bandera para controlar la impresión del carro
bool juegoPausado = false; // Bandera para controlar el estado de pausa
bool menuActualizado = false;  // Bandera para verificar si el menú ha sido actualizado
int vertPerder = 4000; // Provocar un movimiento involuntario cada vez que se genere el menu al perder y no provocar bugs visuales
int opcionSeleccionada = 1;  // 1 = Jugar, 0 = HighScore
bool enMenu = true;
unsigned long tiempoInicioRestore = 0;  // Variable global para el tiempo inicial
bool botonesPresionados = false; // Variable global para indicar si los botones están presionados

const int MAX_SCORES = 20; // Puntajes limitados a 20
const char* SCORES_FILE = "/highscores.json";
int scores[MAX_SCORES];      // Puntajes más altos
char names[MAX_SCORES][4];  // Nombres de los jugadores
int puntaje = 0;  // Contador de personas atropelladas
char currentName[4] = "AAA";  // Nombre del jugador actual
int currentLetterIndex = 0;  // Índice para cambiar letras del nombre
bool adjustingName = false;

struct Posicion { // Estado de personas, relojes y picos en el display
  int x, y;
  unsigned long tiempoGeneracion;  // Tiempo en que se generó el pico para saber cuándo eliminarlo
};

Posicion personas[8] = {};
Posicion relojes[8] = {};
Posicion picos[8] = {};
unsigned long ultimaPersona = 0;
unsigned long ultimoReloj = 0;
unsigned long ultimoPico = 0;

Audio audio;

void setup() {

  // Set microSD Card CS as OUTPUT and set HIGH
  pinMode(SD_CS, OUTPUT);      
  digitalWrite(SD_CS, HIGH); 
    
  // Initialize SPI bus for microSD Card
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);
  
  // Start microSD Card
  if(!SD.begin(SD_CS))
  {
    Serial.println("Error accessing microSD card!");
    while(true); 
  }
  
  // Setup I2S 
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    
  // Set Volume
  audio.setVolume(10);
    
  // Open music file
  audio.connecttoFS(SD,"/endpos.mp3");
  Serial.println("Reproduciendo musica de menu!");
  
  lcd.init();
  lcd.createChar(1, carro2);
  lcd.createChar(2, carro1);
  lcd.createChar(3, persona);
  lcd.createChar(4, reloj);
  lcd.createChar(5, pico);
  lcd.createChar(6, borde);

  pinMode(VERT_PIN, INPUT);
  pinMode(HORZ_PIN, INPUT);
  pinMode(SEL_PIN, INPUT_PULLUP);
  pinMode(BUTPAUSE_PIN, INPUT_PULLUP);
  pinMode(BUTSEL_PIN, INPUT_PULLUP);
  pinMode(BUTRES_PIN, INPUT_PULLUP);
  pinMode(BUZ_PIN, OUTPUT);
  
  // Inicializar la pantalla
  lcd.backlight();
  
  // Mostrar las pantallas de inicio y el menú
  mostrarPantallasInicio();
  mostrarMenu();
  
  Serial.begin(115200);
}

void loop() {

  if (enMenu) {
    manejarMenu();
    audio.loop();
  } 
  
  else if (!gameOver && juegoIniciado) {

    if (digitalRead(BUTPAUSE_PIN) == 0) { // Si el botón de pausa se presiona
      selectOptionSound();
      juegoPausado = !juegoPausado; // Alternar el estado de pausa
      if (juegoPausado) {
        lcd.setCursor(15, 3);
        lcd.print("PAUSA"); // Mostrar "PAUSA" en la pantalla
        audio.connecttoFS(SD, "/.."); // Quitar musica
        Serial.println("Quitando musica de juego!");
      } else {
        lcd.setCursor(15, 3);
        lcd.print("VAMOS"); // Mostrar "VAMOS" al reanudar el juego
        audio.connecttoFS(SD, "/ratdoom.mp3"); // Reproducir música al reanudar
        Serial.println("Reproduciendo musica de juego!");
      }
      delay(500); // Retardo para evitar múltiples detecciones
    }

    if (!cuentaRegresivaCompletada) {
      mostrarCuentaRegresiva();  // Mostrar cuenta regresiva antes de empezar
      audio.connecttoFS(SD, "/ratdoom.mp3"); // Reproducir música al reanudar
      Serial.println("Reproduciendo musica de juego!");
    }
    
    else if (!juegoPausado) {
      
      // Lógica del juego cuando no está pausado

      if (digitalRead(BUTRES_PIN) == 0){
        selectOptionSound();
        regresarMenu();
      }

      // Leer joystick
      int vert = analogRead(VERT_PIN);
      int horz = analogRead(HORZ_PIN);
      int boton = digitalRead(SEL_PIN);

      moverCarrito(vert, horz);

      // Actualizar el temporizador cada segundo
      static unsigned long lastTimeUpdate = 0;
      if (millis() - lastTimeUpdate >= 1000) {
        timer--;
        mostrarTimer();
        lastTimeUpdate = millis();
      }

      // Verificar si el tiempo ha terminado
      if (timer <= 0) {
        gameOver = true;
        mostrarGameOver();
      }

      // Se genera una persona cada cierto tiempo
      if (millis() - ultimaPersona > random(750, 1500)) {
        generarPersona();
        ultimaPersona = millis();
      }

      // Se genera un reloj cada cierto tiempo
      if (millis() - ultimoReloj > random(750, 3000)) {
        generarReloj();
        ultimoReloj = millis();
      }

      // Se genera un pico cada cierto tiempo
      if (millis() - ultimoPico > random(750, 1500)) {
        generarPico();
        ultimoPico = millis();
      }

      // Restaurar velocidad si ha pasado el tiempo de colisión
      if (millis() - ultimaColision > 3000 && velocidad > 1.0) {
        velocidad = 1.0;
        mostrarVelocidad();
      }

      // Verificar colisiones
      verificarColisiones();

      // Funcion para gestionar el boost extra con el select del Joystick
      manejarBoostExtra(boton);

      // Eliminar picos, personas y relojes en ciertas cantidades de tiempo
      eliminarPicos();
      eliminarRelojes();
      eliminarPersonas();
      
      if (!carritoImprimido) {
        imprimirCarritoInicio();  // Imprimir el carro si no ha sido impreso aún
        carritoImprimido = true;  // Marcar como impreso
      }

    }
    audio.loop(); // Mantener el audio activo
  }
}

void imprimirCarritoInicio() {
  lcd.setCursor(arriba, abajo);  // Posición inicial del carro
  lcd.write(1);  // Imprimir el ícono del carro
}

// Acciones del menú
void changeMenuOptionSound() {
  tone(BUZ_PIN, 440); // Tono suave
  delay(10);
  noTone(BUZ_PIN);
}

void selectOptionSound() {
  tone(BUZ_PIN, 523); // Tono fuerte
  delay(10);
  noTone(BUZ_PIN);
}

void scorePointSound() {
  tone(BUZ_PIN, 1000); // Tono alegre
  delay(10);
  noTone(BUZ_PIN);
}

void TurboUseSound() {
  tone(BUZ_PIN, 440); // Tono suave
  delay(10);
  noTone(BUZ_PIN);
}

void melodyLose() {
  int notes[] = {440, 494, 523}; // Notas de la melodía de perder
  for (int i = 0; i < 3; i++) {
    tone(BUZ_PIN, notes[i]);
    delay(300); // Duración de cada nota
    noTone(BUZ_PIN);
    delay(100); // Pausa entre notas
  }
  delay(10);
  noTone(BUZ_PIN);
}

void melodyWin() {
  int notes[] = {523, 587, 659}; // Notas de la melodía de ganar
  for (int i = 0; i < 3; i++) {
    tone(BUZ_PIN, notes[i]);
    delay(300); // Duración de cada nota
    noTone(BUZ_PIN);
    delay(100); // Pausa entre notas
  }
  delay(10);
  noTone(BUZ_PIN);
}

void moverCarrito(int vert, int horz) {
  // Lógica para mover el carrito con joystick
  if (horz < 1000 && arriba < 13) {  // Limitar el área de movimiento a 4x14 casillas
    lcd.setCursor(arriba, abajo);
    lcd.print(" ");
    arriba++;
    lcd.setCursor(arriba, abajo);
    lcd.write(1);
    delay(200 / velocidad); // Reducir velocidad
  }
  if (horz > 3000 && arriba > 0) {
    lcd.setCursor(arriba, abajo);
    lcd.print(" ");
    arriba--;
    lcd.setCursor(arriba, abajo);
    lcd.write(1);
    delay(200 / velocidad);
  }
  if (vert < 1000 && abajo < 3) {
    lcd.setCursor(arriba, abajo);
    lcd.print(" ");
    abajo++;
    lcd.setCursor(arriba, abajo);
    lcd.write(2);
    delay(200 / velocidad);
  }
  if (vert > 3000 && abajo > 0) {
    lcd.setCursor(arriba, abajo);
    lcd.print(" ");
    abajo--;
    lcd.setCursor(arriba, abajo);
    lcd.write(2);
    delay(200 / velocidad);
  }
}

void manejarBoostExtra(int boton) {
    if (boton == 0 && !boostActivo && millis() - boostCooldown >= 15000) {
        TurboUseSound();
        boostActivo = true;
        boostStart = millis();
        velocidad += 0.1; 
        mostrarVelocidad();
    }

    if (boostActivo && millis() - boostStart >= 3000) {
        boostActivo = false;
        velocidad -= 0.1;
        boostCooldown = millis(); // Iniciar el cooldown
        mostrarVelocidad();
    }
}

void mostrarTimer() {
  // Borrar el valor anterior del timer antes de mostrar el nuevo
  lcd.setCursor(14, 0);
  lcd.write(6);
  lcd.setCursor(16, 0);  // Ajustar a 17 para dejar espacio suficiente
  lcd.print("  ");  // Borrar lo que estaba antes
  lcd.setCursor(16, 0);  
  lcd.print(timer < 10 ? "0" : "");  // Si el número es menor que 10, mostrar un 0 a la izquierda
  lcd.print(timer);  // Mostrar el valor del timer

  mostrarVelocidad();  // Actualizar también el multiplicador de velocidad
  mostrarPuntaje();  // Actualizar también el puntaje
  mostrarAnuncio(); // Mostrar el nivel actual
}

void mostrarVelocidad() {
  // Borrar el valor anterior de la velocidad antes de mostrar el nuevo
  lcd.setCursor(14, 1);
  lcd.write(6);
  lcd.setCursor(16, 1);  // Debajo del timer
  lcd.print("   ");  // Borrar el valor anterior
  lcd.setCursor(16, 1);
  lcd.print(velocidad, 1);  // Mostrar la velocidad con un decimal
}

void mostrarPuntaje() {
  // Borrar el valor anterior del timer antes de mostrar el nuevo
  lcd.setCursor(14, 2);
  lcd.write(6);
  lcd.setCursor(16, 2);  // Ajustar a 17 para dejar espacio suficiente
  lcd.print("  ");  // Borrar lo que estaba antes
  lcd.setCursor(16, 2);
  lcd.print(puntaje); // Si el número es menor que 10, mostrar un 0 a la izquierda
  lcd.setCursor(18, 2);
  lcd.print("P");
}

void mostrarAnuncio() {
  // Borrar el valor anterior del timer antes de mostrar el nuevo
  lcd.setCursor(14, 3);
  lcd.write(6);
  lcd.setCursor(15, 3);  // Ajustar a 17 para dejar espacio suficiente
  lcd.print("  ");  // Borrar lo que estaba antes
  lcd.setCursor(15, 3);  
  lcd.print("VAMOS"); // Si el número es menor que 10, mostrar un 0 a la izquierda
}

void mostrarCuentaRegresiva() {
  // Mostrar el carro en su posición inicial
  lcd.setCursor(arriba, abajo);  // Mostrar el carro en su posición inicial
  lcd.write(1);                  // Mostrar el ícono del carro

  // Mostrar cuenta regresiva
  for (int i = 3; i > 0; i--) {
    lcd.setCursor(10, 1);  // Posición centrada para la cuenta regresiva
    lcd.print(i);
    tone(BUZ_PIN, 1000, 800);
    delay(1000);  // Esperar 1 segundo por número
  }

  // Mostrar mensaje de inicio
  lcd.setCursor(5, 1);
  lcd.print("Atropella!");
  tone(BUZ_PIN, 1500, 800);
  delay(1000);  // Esperar 1 segundo antes de borrar el mensaje

  // Borrar la palabra "Atropella!"
  lcd.setCursor(5, 1);
  lcd.print("          ");  // Borra la palabra con espacios

  timer = 30;
  tiempoInicioJuego = millis();  // Guardar el tiempo de inicio del juego
  cuentaRegresivaCompletada = true;  // Marcar como completada la cuenta regresiva
}

void generarPersona() {
  if (contarEntidades(personas, 8) < 8) {  // Limitar a 8 personas
    Posicion nuevaPos = obtenerPosicionAleatoria();

    // Evitar que los picos se generen en la posición actual del carro
    if (nuevaPos.x == arriba && nuevaPos.y == abajo) {
      return;  // Si el pico se generaría en la posición del carro, no lo generamos
    }

    nuevaPos.tiempoGeneracion = millis();
    if (!colisionaConEntidades(nuevaPos)) {
      agregarEntidad(personas, nuevaPos, 8);
      lcd.setCursor(nuevaPos.x, nuevaPos.y);
      lcd.write(3);  // Icono de persona
    }
  }
}

void generarReloj() {
  if (contarEntidades(relojes, 8) < 8) {  // Limitar a 8 relojes
    Posicion nuevaPos = obtenerPosicionAleatoria();
    
    // Evitar que los picos se generen en la posición actual del carro
    if (nuevaPos.x == arriba && nuevaPos.y == abajo) {
      return;  // Si el pico se generaría en la posición del carro, no lo generamos
    }

    nuevaPos.tiempoGeneracion = millis();
    if (!colisionaConEntidades(nuevaPos)) {
      agregarEntidad(relojes, nuevaPos, 8);
      lcd.setCursor(nuevaPos.x, nuevaPos.y);
      lcd.write(4);  // Icono de reloj
    }
  }
}

void generarPico() {
  if (contarEntidades(picos, 8) < 8) {  // Limitar a 8 picos
    Posicion nuevaPos = obtenerPosicionAleatoria();

    // Evitar que los picos se generen en la posición actual del carro
    if (nuevaPos.x == arriba && nuevaPos.y == abajo) {
      return;  // Si el pico se generaría en la posición del carro, no lo generamos
    }

    nuevaPos.tiempoGeneracion = millis();
    if (!colisionaConEntidades(nuevaPos)) {
      agregarEntidad(picos, nuevaPos, 8);
      lcd.setCursor(nuevaPos.x, nuevaPos.y);
      lcd.write(5);  // Icono de pico
    }
  }
}

void eliminarPicos() {
  for (int i = 0; i < 8; i++) {
    if (picos[i].x != -1 && millis() - picos[i].tiempoGeneracion > 15000) {  // 15 segundos de vida del pico
      lcd.setCursor(picos[i].x, picos[i].y);
      lcd.print(" ");  // Borrar el pico
      picos[i].x = -1;  // Marcar como eliminado
    }
  }
}

void eliminarRelojes() {
  for (int i = 0; i < 8; i++) {
    if (relojes[i].x != -1 && millis() - relojes[i].tiempoGeneracion > 5000) {  // 5 segundos de vida del reloj
      lcd.setCursor(relojes[i].x, relojes[i].y);
      lcd.print(" ");  // Borrar el reloj
      relojes[i].x = -1;  // Marcar como eliminado
    }
  }
}

void eliminarPersonas() {
  for (int i = 0; i < 8; i++) {
    if (personas[i].x != -1 && millis() - personas[i].tiempoGeneracion > 10000) {  // 20 segundos de vida de la persona
      lcd.setCursor(personas[i].x, personas[i].y);
      lcd.print(" ");  // Borrar la persona
      personas[i].x = -1;  // Marcar como eliminado
    }
  }
}

void verificarColisiones() {
  if (millis() - tiempoInicioJuego > 1000) {
    // Verificar si el carrito pasa por encima de una persona
    for (int i = 0; i < 8; i++) {
      if (arriba == personas[i].x && abajo == personas[i].y) {
        scorePointSound();
        velocidad = min(velocidad + 0.2, 2.0);
        ultimaColision = millis();  // Registrar el momento de la colisión
        mostrarVelocidad();  // Actualizar la velocidad en la pantalla
        personas[i].x = -1;  // Marcar la persona como eliminada
        
        // Incrementar el puntaje por cada persona atropellada
        puntaje++;  
        
        Serial.print("Puntaje: "); // Mostrar etiqueta
        Serial.println(puntaje);  // Mostrar puntaje actual

        // Mostrar mensaje de nivel superado si se alcanza 50 atropellos
        if (puntaje >= 50) {
          enMenu = true;  // Volver al menú
          gameOver = true;
          mostrarNivelSuperado();
        }
      }
    }

    // Verificar si el carrito pasa por encima de un reloj
    for (int i = 0; i < 8; i++) {
      if (arriba == relojes[i].x && abajo == relojes[i].y) {
        timer += 4;  // Aumentar 3 segundos
        relojes[i].x = -1;  // Marcar el reloj como eliminado
      }
    }

    // Verificar si el carrito pasa por encima de un pico
    for (int i = 0; i < 8; i++) {
      if (arriba == picos[i].x && abajo == picos[i].y) {
        timer = max(timer - 8, 0);  // Restar 9 segundos al contador
        picos[i].x = -1;  // Marcar el pico como eliminado
      }
    }
  }
}

Posicion obtenerPosicionAleatoria() {
  Posicion nuevaPos;
  nuevaPos.x = random(0, 13);  // Limitar la posición para que no pase sobre el timer
  nuevaPos.y = random(0, 4);
  return nuevaPos;
}

bool colisionaConEntidades(Posicion pos) {
  for (int i = 0; i < 8; i++) {
    if ((personas[i].x == pos.x && personas[i].y == pos.y) || (relojes[i].x == pos.x && relojes[i].y == pos.y) || (picos[i].x == pos.x && picos[i].y == pos.y)) {
      return true;  // Colisiona con otra entidad
    }
  }
  return false;
}

void agregarEntidad(Posicion entidades[], Posicion nuevaPos, int maxEntidades) {
  for (int i = 0; i < maxEntidades; i++) {
    if (entidades[i].x == -1) {  // Encontrar un lugar vacío
      entidades[i] = nuevaPos;
      break;
    }
  }
}

int contarEntidades(Posicion entidades[], int maxEntidades) {
  int contador = 0;
  for (int i = 0; i < maxEntidades; i++) {
    if (entidades[i].x != -1) {
      contador++;
    }
  }
  return contador;
}

void mostrarPantallasInicio() {
  // Pantalla de créditos
  lcd.clear();
  lcd.setCursor(2, 1);
  lcd.print("Hecho por:");
  lcd.setCursor(2, 2);
  lcd.print("Diego y Ernesto");
  delay(3000);

  // Pantalla del título con logo (carro, persona, reloj, pico)
  lcd.clear();
  lcd.setCursor(4, 1);
  lcd.print("Crazy Chevy"); // Título del juego
  
  // Mostrar el carrito, la persona, el reloj y el pico simulando el logo
  lcd.setCursor(6, 2);  // Carro en la parte de abajo
  lcd.write(1);          // Escribir el ícono del carro
  
  lcd.setCursor(8, 2);   // Persona al lado del carro
  lcd.write(3);          // Escribir el ícono de la persona

  lcd.setCursor(10, 2);   // Reloj a la derecha de la persona
  lcd.write(4);          // Escribir el ícono del reloj

  lcd.setCursor(12, 2);  // Pico a la derecha del reloj
  lcd.write(5);          // Escribir el ícono del pico

  delay(3000);
}

void mostrarMenu() {

  lcd.clear();
  lcd.setCursor(5, 1);
  lcd.print("> Jugar");
  lcd.setCursor(5, 2);
  lcd.print("  HighScore");

}

void manejarMenu() {
  int vert = analogRead(VERT_PIN);  // Lectura actual del joystick
  int boton = digitalRead(BUTSEL_PIN);
  int botonPause = digitalRead(BUTPAUSE_PIN);
  int botonRes = digitalRead(BUTRES_PIN);

  // Verificar si ambos botones se presionan al mismo tiempo
  if (botonPause == 0 && botonRes == 0) {  // Ambos botones presionados
    if (!botonesPresionados) {
      botonesPresionados = true;
      tiempoInicioRestore = millis();  // Guardar el tiempo inicial
    } else if (millis() - tiempoInicioRestore >= 3000) {  // Si ya pasaron 3 segundos
      restaurarHighScores();  // Llamar a la función para restaurar los puntajes

      // Limpiar la pantalla y mostrar el mensaje "Scores restaurados" centrado
      lcd.clear();
      lcd.setCursor(1, 1);  // Posición para centrar en una LCD 20x4
      lcd.print("Scores restaurados");

      delay(3000);  // Mostrar el mensaje por 3 segundos

      mostrarMenu();

      botonesPresionados = false;  // Resetear la variable de control
    }
  } 
  else {
    botonesPresionados = false;  // Resetear si los botones no están presionados simultáneamente
  }

  // Variable para indicar si hay un cambio en la opción seleccionada
  bool menuActualizado = false;

  // Mover hacia abajo (ahora selecciona "HighScore")
  if (vert < 1000 && opcionSeleccionada != 0 ) {
    opcionSeleccionada = 0;
    menuActualizado = true;
    changeMenuOptionSound();
  } 
  // Mover hacia arriba (ahora selecciona "Jugar")
  else if (vert > 3000 && opcionSeleccionada != 1 || vertPerder > 3000 && opcionSeleccionada != 0) {
    opcionSeleccionada = 1;
    menuActualizado = true;
    vertPerder = 0; // Esto es para evitar que se muestren bugs visuales 
    changeMenuOptionSound();
  }

  // Actualizar el menú solo si ha cambiado la opción seleccionada
  if (menuActualizado) {
    lcd.clear();
    if (opcionSeleccionada == 1) {
      lcd.setCursor(5, 1);
      lcd.print("> Jugar");
      lcd.setCursor(5, 2);
      lcd.print("  HighScore");
    } else {
      lcd.setCursor(5, 1);
      lcd.print("  Jugar");
      lcd.setCursor(5, 2);
      lcd.print("> HighScore");
    }
  }

  // Seleccionar la opción con el botón
  if (boton == 0) {
    if (opcionSeleccionada == 1) {
      // Iniciar el juego
      selectOptionSound();
      enMenu = false;
      juegoIniciado = true;
      tiempoInicioJuego = millis();
      lcd.clear();
    } else if (opcionSeleccionada == 0) {
      selectOptionSound();
      mostrarHighScores();
      mostrarMenu();
      opcionSeleccionada = 1;
    }
    delay(500);  // Para evitar múltiples selecciones rápidas
  }
}

void mostrarNivelSuperado() {
  lcd.clear();
  lcd.setCursor(2, 1);
  lcd.print("Nivel 1 Superado");
  lcd.setCursor(4, 2);
  lcd.print("Score: "); 
  lcd.print(puntaje);  // Mostrar puntaje
  melodyWin();
  delay(3000);

  ingresarNombre();  // Ingresar nombre después de Game Over

}

void mostrarGameOver() {
  lcd.clear();
  lcd.setCursor(4, 1);
  lcd.print("GAME OVER");
  lcd.setCursor(4, 2);
  lcd.print("Score: "); 
  lcd.print(puntaje);  // Mostrar puntaje
  melodyLose();
  delay(3000);

  ingresarNombre();  // Ingresar nombre después de Game Over

}

void saveScore(int score, const char* username) {
    File file = SD.open(SCORES_FILE, FILE_READ);
    
    // Lee el contenido existente si el archivo existe
    DynamicJsonDocument doc(2048); // Crea un documento JSON
    JsonArray scoresArray;

    if (file) {
        DeserializationError error = deserializeJson(doc, file);
        if (error) {
            Serial.println("Error al leer el archivo JSON");
            file.close();
            return;
        }
        scoresArray = doc.as<JsonArray>();
        file.close(); // Cierra el archivo después de leer
    } else {
        Serial.println("Archivo no encontrado, creando uno nuevo.");
        scoresArray = doc.createNestedArray(); // Crea un nuevo array si el archivo está vacío
    }

    // Agregar el nuevo puntaje
    JsonObject scoreEntry = scoresArray.createNestedObject();
    scoreEntry["username"] = username;
    scoreEntry["score"] = score;

    // Abre el archivo nuevamente para sobrescribirlo
    file = SD.open(SCORES_FILE, FILE_WRITE);
    if (!file) {
        Serial.println("Error al abrir el archivo para escribir el JSON");
        return;
    }

    // Escribe el documento JSON actualizado
    if (serializeJson(doc, file) == 0) {
        Serial.println("Error al escribir en el archivo JSON");
    } else {
        Serial.println("Puntaje guardado correctamente");
    }

    file.close(); // Cierra el archivo después de escribir
}

void ingresarNombre() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Ingresa tu nombre:");

    currentLetterIndex = 0;  // Comienza desde la primera letra
    char letras[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";  // Letras disponibles
    int letraIndex = 0;  // Índice de la letra actual
    bool nombreCompleto = false;  // Flag para verificar si se completó el nombre

    while (!nombreCompleto) {  // Ciclo hasta completar las 3 letras
        lcd.setCursor(0, 1);
        lcd.print(currentName);  // Muestra el nombre actual
        lcd.setCursor(currentLetterIndex, 1);  // Coloca el cursor en la letra que se está editando
        lcd.print(letras[letraIndex]);  // Muestra la letra actual

        // Leer el joystick
        int botonConfirmar = digitalRead(BUTSEL_PIN);  // Botón para confirmar el nombre
        int vert = analogRead(VERT_PIN);

        // Cambiar letra hacia abajo
        if (vert < 1000) {  // Si se mueve hacia abajo
            letraIndex = (letraIndex + 1) % 26;  // Incrementa el índice de letra
            delay(200);  // Retardo para evitar múltiples entradas rápidas
        }

        // Cambiar letra hacia arriba
        else if (vert > 3000) {  // Si se mueve hacia arriba
            letraIndex = (letraIndex - 1 + 26) % 26;  // Decrementa el índice de letra
            delay(200);  // Retardo para evitar múltiples entradas rápidas
        }

        // Confirmar letra
        if (botonConfirmar == 0) {  // Si se presiona el botón de selección
            selectOptionSound();
            currentName[currentLetterIndex] = letras[letraIndex];  // Guarda la letra seleccionada
            currentLetterIndex++;  // Pasa a la siguiente letra
            letraIndex = 0;  // Reinicia el índice de letra para la próxima selección

            // Verificar si se completó el nombre
            if (currentLetterIndex == 3) {
                nombreCompleto = true;  // Marca el nombre como completo
            }

            delay(500);  // Retardo para evitar múltiples entradas rápidas
        }

        // Espera un pequeño tiempo antes de leer nuevamente el joystick
        delay(100);
    }

    // Mostrar mensaje de puntaje guardado
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Puntaje guardado");
    Serial.println(currentName);
    delay(1000);  // Esperar 1 segundo antes de regresar al menú

    // Guardar puntaje en el archivo JSON
    guardarYMostrarHighScores(puntaje, currentName);  // Llamar a guardar y mostrar puntajes altos
    Serial.println("Score guardado en JSON");

    // Regresar al menú
    regresarMenu();

}

void regresarMenu() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Regresando al menu...");
  delay(1000);  // Esperar 1 segundo antes de regresar al menú
  enMenu = true;  // Volver al menú
  gameOver = false;
  juegoIniciado = false;
  cuentaRegresivaCompletada = false;
  timer = 30;
  puntaje = 0;
  arriba = 10;
  abajo = 3;
  vertPerder = 4000;
  mostrarMenu();

  // Open music file
  audio.connecttoFS(SD,"/endpos.mp3");
  Serial.println("Reproduciendo musica de menu!");
}

void cargarHighScores() {
    File file = SD.open(SCORES_FILE, FILE_READ); // Abre el archivo en modo de escritura (permite también leer)
    
    if (!file) {
        Serial.println("Error al abrir el archivo de puntajes");
        return;
    }

    Serial.println("Abriendo highscores.json");
    DynamicJsonDocument doc(2048); // Crear el documento JSON
    DeserializationError error = deserializeJson(doc, file);
    
    if (error) {
        Serial.println("Error al leer el archivo JSON");
        Serial.print("Error: ");
        Serial.println(error.f_str()); // Imprime el mensaje de error
        file.close();
        return;
    }
    
    file.close();

    JsonArray scoresArray = doc.as<JsonArray>();

    // Inicializa los puntajes a 0 y los nombres a "AAA"
    for (int i = 0; i < MAX_SCORES; i++) {
        scores[i] = 0;
        strlcpy(names[i], "AAA", sizeof(names[i]));
    }

    // Copia los puntajes y nombres del JSON al array
    int count = 0;
    for (JsonObject scoreEntry : scoresArray) {
        if (count < MAX_SCORES) {
            scores[count] = scoreEntry["score"];
            strlcpy(names[count], scoreEntry["username"], sizeof(names[count]));
            count++;
        }
    }

    // Ordenar los puntajes de mayor a menor
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (scores[j] < scores[j + 1]) {
                // Intercambiar puntajes
                int tempScore = scores[j];
                scores[j] = scores[j + 1];
                scores[j + 1] = tempScore;

                // Intercambiar nombres
                char tempName[20];
                strlcpy(tempName, names[j], sizeof(tempName));
                strlcpy(names[j], names[j + 1], sizeof(names[j]));
                strlcpy(names[j + 1], tempName, sizeof(names[j + 1]));
            }
        }
    }
}

// Función para mostrar los puntajes altos
void mostrarHighScores() {
    cargarHighScores(); 
    lcd.clear();  // Limpiar la pantalla

    // Mostrar los 4 puntajes más altos
    for (int i = 0; i < 4; i++) {
        lcd.setCursor(0, i);  // Mover el cursor a la fila correspondiente
        lcd.print(i + 1);  // Mostrar el ranking (1, 2, 3, 4)
        lcd.print(": ");
        lcd.print(scores[i]);  // Mostrar el puntaje
        lcd.print(" - ");
        lcd.print(names[i]);  // Mostrar el nombre del jugador
    }

    delay(5000);  // Mostrar durante 5 segundos
}

void guardarYMostrarHighScores(int puntaje, const char* username) {
    saveScore(puntaje, username);  // Guardar el puntaje
    mostrarHighScores();  // Mostrar los puntajes altos
}

void restaurarHighScores() {
  // Ruta al archivo highscores.json
  const char* filename = "/highscores.json";

  // Asegúrate de que la tarjeta SD esté inicializada antes de abrir el archivo
  if (!SD.begin()) {
    Serial.println("Error al inicializar la tarjeta SD");
    return;
  }

  // Abrir el archivo en modo de escritura
  File file = SD.open(filename, FILE_WRITE);

  if (file) {
    // Escribir el array vacío en el archivo
    file.println("[]");
    file.close();  // Cerrar el archivo después de escribir
  }
}