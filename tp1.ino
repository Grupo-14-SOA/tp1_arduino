// DEBUG MODE
#define SERIAL_DEBUG_ENABLED 0
#if SERIAL_DEBUG_ENABLED
  #define DebugPrint(str)\
    {\
      Serial.println(str);\
    }
#else
  #define DebugPrint(str)
#endif
#define DebugPrintEstado(estado,evento)\
  {\
    String str;\
    str = "======================================================";\
    DebugPrint(str);\
    str = "Estado " + estado + " ...";\
    DebugPrint(str);\
    str = "Evento " + evento + " ...";\
    DebugPrint(str);\
    str = "======================================================";\
    DebugPrint(str);\
  }

// LIBRERIA PARA MANEJAR LA PANTALLA LCD
#include <LiquidCrystal.h>
// LIBRERIA PARA MANEJAR EL TECLADO 4X4
#include <Keypad.h>
// LIBRERIA PARA MANEJAR EL SERVOMOTOR
#include <Servo.h>

// ESTADOS DEL SISTEMA EMBEBIDO
#define ESTADO_PREPARADO 0
#define ESTADO_PUERTA_CERRADA 1
#define ESTADO_INGRESANDO_CODIGO 2
#define ESTADO_PUERTA_ABIERTA 3
#define CANT_ESTADOS 4
// Defino un vector de funciones para llamar a cada verificación de sensores según el estado correspondiente
void (*verificar_sensor[CANT_ESTADOS])() = {verificar_distancia_a_objeto, verificar_sensor_de_fuerza, verificar_fuerza_y_codigo, verificar_distancia_a_objeto};
String nombres_estados [] = { "ESTADO_PREPARADO", "ESTADO_PUERTA_CERRADA", "ESTADO_INGRESANDO_CODIGO", "ESTADO_PUERTA_ABIERTA"};

// VARIABLE GLOBAL DE ESTADO DEL SISTEMA EMBEBIDO
int estado;
int evento;

// EVENTOS DEL SISTEMA EMBEBIDO
#define EVENTO_OBJETO_DETECTADO 0
#define EVENTO_CAMBIO_EN_DISTANCIA 1
#define EVENTO_ENCIMA_D_UMBRAL_MINIMO_DE_FUERZA 2
#define EVENTO_DEBAJO_D_UMBRAL_MINIMO_DE_FUERZA 3
#define EVENTO_CODIGO_INCORRECTO 4
#define EVENTO_CODIGO_CORRECTO 5
#define EVENTO_OBJETO_NO_DETECTADO 6
#define EVENTO_CODIGO_INCOMPLETO 7
#define EVENTO_LIBERAR 8
String nombres_eventos [] = { "EVENTO_OBJETO_DETECTADO", "EVENTO_CAMBIO_EN_DISTANCIA", "EVENTO_ENCIMA_D_UMBRAL_MINIMO_DE_FUERZA", "EVENTO_DEBAJO_D_UMBRAL_MINIMO_DE_FUERZA", \
                              "EVENTO_CODIGO_INCORRECTO", "EVENTO_CODIGO_CORRECTO", "EVENTO_OBJETO_NO_DETECTADO", "EVENTO_CODIGO_INCOMPLETO", "EVENTO_LIBERAR"};


// PIN SEMILLA
#define PIN_CERO 0

// SENSOR DISTANCIA
#define PIN_TRIGGER 19
#define PIN_ECHO 19
#define ANCHO_PULSO_TRIGGER_MICRO_LOW 2
#define ANCHO_PULSO_TRIGGER_MICRO_HIGH 5
// DEFINO LOS RANGOS, EN CM, DE DETECCION DEL SENSOR DE PROXIMIDAD
#define DISTANCIA_MAX_D_LECTURA 320
#define DISTANCIA_MIN_D_LECTURA 3

// SENSOR DE FUERZA
#define PIN_SENSOR_D_FUERZA 18
// FUERZA MINIMA
// SE LE PONE 858 (5N) PORQUE SE CONVIERTE UNA ESCALA DE 0N~10N A UNA ESCALA 0~914
#define FUERZA_MIN 855

// PARLANTE
#define PIN_PARLANTE 10
// DEFINIMOS UNA RAZON DE PROPORCION PARA CONVERTIR UNA DISTANCIA EN CM
// A UNA FRECUENCIA EN HZ, YA QUE LA DISTANCIA QUE PUEDE LEER EL SENSOR DE PROXIMIDAD
// ESTA MAS O MENOS ENTRE 3cm~336cm
#define RAZON_D_PROPORCION_CM_A_HZ 10
// DEFINIMOS EL VALOR MAXIMO DEL PARLANTE EN FUNCION DE LA DISTANCIA MAXIMA DEL SENSOR DE PROXIMIDAD
// MAS LA DISTANCIA MINIMA DE LECTURA, y MULTIPLCAMOS POR LA RAZON DE PROPORCION
// ESTO EN NUESTRO CASO NOS DA COMO RESULTADO UN VALOR MINIMO DEL PARLANTE DE 30HZ Y UN
// VALOR MAXIMO DE 3360HZ
#define VALOR_PARLANTE ((DISTANCIA_MAX_D_LECTURA + DISTANCIA_MIN_D_LECTURA) * RAZON_D_PROPORCION_CM_A_HZ)

// SENSOR DE DISTANCIA
#define PIN_SENSOR_D_DISTANCIA 18
// VELOCIDAD DEL SONIDO/2
#define VELOCIDAD_D_SONIDO 0.01723

// PANTALLA LCD 16x2
#define ANCHO_PANTALLA_LCD 16
#define ALTO_PANTALLA_LCD 2
#define PIN_PANTALLA_LCD_RS 13
#define PIN_PANTALLA_LCD_E 12
#define PIN_PANTALLA_LCD_DB4 14
#define PIN_PANTALLA_LCD_DB5 15
#define PIN_PANTALLA_LCD_DB6 16
#define PIN_PANTALLA_LCD_DB7 17
#define PRIMERA_COLUMNA_DISPLAY 0
#define PRIMERA_FILA_DISPLAY 0
#define SEGUNDA_FILA_DISPLAY 1
LiquidCrystal lcd(PIN_PANTALLA_LCD_RS, PIN_PANTALLA_LCD_E, PIN_PANTALLA_LCD_DB4, PIN_PANTALLA_LCD_DB5, PIN_PANTALLA_LCD_DB6, PIN_PANTALLA_LCD_DB7);
//DEFINO CADA CUANDO TIEMPO SE VA A ACTUALIZAR EL DISPLAY DE LA PANTALLA
#define TIEMPO_MAX_MILIS 5000
// VARIABLES GLOBALES DE TEMPORIZADOR POR SOFTWARE PARA ACTUALIZACIÓN DE CÓDIGO EN PANTALLA
unsigned long tiempo_actual, tiempo_anterior;

// TECLADO 4X4
#define FIL_TECLADO 4
#define COL_TECLADO 4
byte pin_fil[] = {9, 8, 7, 6};
byte pin_col[] = {5, 4, 3, 2};
// DEFINO LA MATRIZ DE CARACTERES QUE VA A REPRESENTAR EL TECLADO 4X4
const char teclas[FIL_TECLADO][COL_TECLADO] =
    {
        {'1', '2', '3', 'A'},
        {'4', '5', '6', 'B'},
        {'7', '8', '9', 'C'},
        {'*', '0', '#', 'D'}};
// MAPEO LA MATRIZ DE CARACTERES CON LOS PINES DEL TECLADO 4X4
Keypad teclado4x4 = Keypad(makeKeymap(teclas), pin_fil, pin_col, FIL_TECLADO, COL_TECLADO);
// CANTIDAD MINIMA Y MAXIMA DE CARACTERES DEL CODIGO A INGRESAR
#define MIN_CARACTERES 4
#define MAX_CARACTERES 8
// CANTIDAD DE CARACTERES A INGRESAR
#define CANTIDAD_D_CARACTERES 6

//MICROSERVOMOTOR
#define PIN_MICROSERVOMOTOR 11
#define ANCHO_PULSO_MIN_MICROSERVOMOTOR 500
#define ANCHO_PULSO_MAX_MICROSERVOMOTOR 2500
#define PUERTA_ABIERTA 0
#define PUERTA_CERRADA 90
Servo microservomotor;

//VARIABLE GLOBAL DE CODIGO A INGRESAR
String codigo_correcto;
//VARIABLE GLOBAL DE CODIGO INGRESADO
String codigo_ingresado;

int requiere_interrupcion()
{
  // TOMO UNA MEDICION DEL TIEMPO ACTUAL Y ACTUALIZO LA VARIABLE GLOBAL
  tiempo_actual = millis();

  // COMPARAO EL DELTA DE TIEMPO CONTRA EL TIEMPO MAXIMO EL MILISEGUNDOS
  return (tiempo_actual - tiempo_anterior) > TIEMPO_MAX_MILIS;
}

void encender_lcd()
{
  lcd.display();
  tiempo_anterior = millis();
}

void apagar_lcd()
{
  lcd.clear();
  lcd.noDisplay();
}

void generar_codigo_correcto()
{
  int col1_random, col2_random, col3_random, col4_random, col5_random, col6_random;
  int fil1_random, fil2_random, fil3_random, fil4_random, fil5_random, fil6_random;
  codigo_correcto = "";
  lcd.setCursor(PRIMERA_COLUMNA_DISPLAY, PRIMERA_FILA_DISPLAY);
  // OBTENGO EL NUMERO DE COLUMNA Y EL NUMERO DE FILA DE CADA CARACTER A MOSTRAR
  col1_random = random(COL_TECLADO);
  col2_random = random(COL_TECLADO);
  col3_random = random(COL_TECLADO);
  col4_random = random(COL_TECLADO);
  col5_random = random(COL_TECLADO);
  col6_random = random(COL_TECLADO);
  fil1_random = random(FIL_TECLADO);
  fil2_random = random(FIL_TECLADO);
  fil3_random = random(FIL_TECLADO);
  fil4_random = random(FIL_TECLADO);
  fil5_random = random(FIL_TECLADO);
  fil6_random = random(FIL_TECLADO);
  // GENERO EL NUEVO CODIGO Y LO IMPRIMO POR PANTALLA
  codigo_correcto += teclas[fil1_random][col1_random];
  codigo_correcto += teclas[fil2_random][col2_random];
  codigo_correcto += teclas[fil3_random][col3_random];
  codigo_correcto += teclas[fil4_random][col4_random]; 
  codigo_correcto += teclas[fil5_random][col5_random];
  codigo_correcto += teclas[fil6_random][col6_random];
  lcd.print(codigo_correcto);
}

void manejar_actualizacion_de_codigo_en_pantalla()
{
  // ESTA FUNCION SE ENCARGA DE RENOVAR EL CODIGO MOSTRADO POR PANTALLA
  // Y DEVOLVER EL CODIGO ACTUAL COMO UN VECTOR DE CARACTERES CON EL CODIGO ACTUAL

  // VERIFICO SI TENGO QUE INTERRUMPIR EL CICLO PARA ACTUALIZAR LA PANTALLA
  if (requiere_interrupcion())
  {
    // ACTUALIZO EL TIEMPO ANTERIOR CON EL VALOR DEL TIEMPO ACTUAL
    tiempo_anterior = tiempo_actual;
    // LIMPIO EL CODIGO ACTUAL DE LA PANTALLA
    lcd.clear();
    generar_codigo_correcto();
    limpiar_codigo_ingresado();
  }
}

long leer_distancia(int trigger, int echo)
{
  // ESTA FUNCION SIRVE PARA CUALQUIER SENSOR DE DISTANCIA QUE SE IMPLEMENTE, SEA DE 3 O 4 PINES
  //int trigger: pin para configurar la salida de la señal de ultrasonido
  //int echo: pin para configurar la escuha de la señal de ultrasonido
  //return: retorna un long con la distancia en cm al objeto

  // Establecemos el pin del trigger porque no sabemos el estado anterior
  pinMode(trigger, OUTPUT);

  // Reseteamos el trigger
  digitalWrite(trigger, LOW);
  delayMicroseconds(ANCHO_PULSO_TRIGGER_MICRO_LOW);
  // Mandamos la señal trigger
  digitalWrite(trigger, HIGH);
  delayMicroseconds(ANCHO_PULSO_TRIGGER_MICRO_HIGH);
  digitalWrite(trigger, LOW);

  // Leemos la distancia a la que se encuentra el objeto
  pinMode(echo, INPUT);
  return VELOCIDAD_D_SONIDO * pulseIn(echo, HIGH);
}

void tono_en_funcion_de_proximidad(int distancia)
{
  // ESTA FUNCION ACTUALIZA EL TONO DEL PARLANTE SEGUN LA DISTANCIA EN CM EN EL PARAMETRO distancia

  // CONVIERTO LA DISTANCIA EN CM A UNA FRECUENCIA EN HZ, SEGUN
  // LA RAZON DE PROPORCION
  distancia *= RAZON_D_PROPORCION_CM_A_HZ;
  // INVIERTO EL VALOR DE DISTACIA PARA QUE A MENOR DISTANCIA LA FRECUENCIA SEA MAYOR
  int tono = VALOR_PARLANTE - distancia;
  tone(PIN_PARLANTE, tono);
}

void apagar_parlante()
{
  noTone(PIN_PARLANTE);
}

void actualizar_tono()
{
  int distancia = leer_distancia(PIN_TRIGGER, PIN_ECHO);
  tono_en_funcion_de_proximidad(distancia);
}

void mover_microservomotor(int angulo)
{
  microservomotor.write(angulo);
}

int leer_sensor_fuerza()
{
  return analogRead(PIN_SENSOR_D_FUERZA);
}

void verificar_sensor_de_fuerza()
{
  int fuerza_detectada = leer_sensor_fuerza();
  if (fuerza_detectada > FUERZA_MIN)
  {
    evento = EVENTO_ENCIMA_D_UMBRAL_MINIMO_DE_FUERZA;
  }
  else
  {
    evento = EVENTO_DEBAJO_D_UMBRAL_MINIMO_DE_FUERZA;
  }
}

void verificar_distancia_a_objeto()
{
  int distancia = leer_distancia(PIN_TRIGGER, PIN_ECHO);

  if (distancia <= DISTANCIA_MAX_D_LECTURA)
  {
    evento = EVENTO_OBJETO_DETECTADO;
  }
  else
  {
    evento = EVENTO_OBJETO_NO_DETECTADO;
  }
}

void limpiar_codigo_ingresado()
{
  codigo_ingresado = "";
}

void limpiar_codigo_ingresado_display()
{
  lcd.setCursor(PRIMERA_COLUMNA_DISPLAY, SEGUNDA_FILA_DISPLAY);
  lcd.print("                     ");
}

void lectura_codigo_teclado()
{
  if (codigo_ingresado.length() == CANTIDAD_D_CARACTERES)
  {
    limpiar_codigo_ingresado_display();
    limpiar_codigo_ingresado();
  }
  lcd.setCursor(codigo_ingresado.length(), SEGUNDA_FILA_DISPLAY);
  char tecla = teclado4x4.getKey();
  if (tecla)
  {
    codigo_ingresado += tecla;
    lcd.print(tecla);
  }
}

void verificar_codigo_ingresado()
{
  if (codigo_ingresado.length() == CANTIDAD_D_CARACTERES)
  {
    if (codigo_ingresado == codigo_correcto)
    {
      evento = EVENTO_CODIGO_CORRECTO;
    }
    else
    {
      evento = EVENTO_CODIGO_INCORRECTO;
    }
  }
  else
  {
    evento = EVENTO_CODIGO_INCOMPLETO;
  }
}

void verificar_fuerza_y_codigo()
{
  verificar_sensor_de_fuerza();
  if (evento == EVENTO_ENCIMA_D_UMBRAL_MINIMO_DE_FUERZA)
  {
    lectura_codigo_teclado();
    verificar_codigo_ingresado();
    if (evento == EVENTO_CODIGO_CORRECTO)
    {
      evento = EVENTO_LIBERAR;
    }
  }
}

void setup()
{
  // Esto es para poder tirar los logs
  Serial.begin(9600);

  // INICIALIZAMOS CON EL ESTADO PREPARADO
  estado = ESTADO_PREPARADO;

  // INICIALIZAMOS EL TIEMPO ANTERIOR
  tiempo_anterior = millis();

  // INICIALIZAMOS UNA SEMILLA RANDOM
  // AL SER UNA SIMULACION EL ANALOGREAD(0) DEVUELVE SIEMPRE 0
  // PERO SI ESTO SE LLEVARA A UN DISPOSITIVO FISICO, ENTONCES EL ANALOGREAD(0)
  // DEVOLVERIA VALORES RANDOM ENTRE 0~1023, POR UNA CUESTION DE RUIDO EN EL DISPOSITIVO
  randomSeed(analogRead(PIN_CERO));

  // INICIALIZAMOS LA PANTALLA LCD
  lcd.begin(ANCHO_PANTALLA_LCD, ALTO_PANTALLA_LCD);
  apagar_lcd();

  // INICIALIZAMOS EL PARLANTE
  pinMode(PIN_PARLANTE, OUTPUT);

  // INICIALIZAMOS EL SERVOMOTOR
  microservomotor.attach(PIN_MICROSERVOMOTOR, ANCHO_PULSO_MIN_MICROSERVOMOTOR, ANCHO_PULSO_MAX_MICROSERVOMOTOR);

  //INICIALIZAMOS LA VARAIBLE GLOBAL DE CODIGO CORRECTO
  codigo_correcto = "";
  //INICIALIZAMOS LA VARAIBLE GLOBAL DE CODIGO CORRECTO
  codigo_ingresado = "";
}

void generar_evento()
{
  verificar_sensor[estado]();
}

void maquina_de_estados_finita_insane_room()
{
  generar_evento();
  switch (estado)
  {
    case ESTADO_PREPARADO:
    {
      switch (evento)
      {
        case EVENTO_OBJETO_DETECTADO:
        {
          DebugPrintEstado(nombres_estados[estado], nombres_eventos[evento]);
          mover_microservomotor(PUERTA_CERRADA);
          estado = ESTADO_PUERTA_CERRADA;
        }
        break;

        case EVENTO_OBJETO_NO_DETECTADO:
        {
          DebugPrintEstado(nombres_estados[estado], nombres_eventos[evento]);
          mover_microservomotor(PUERTA_ABIERTA);
        }
        break;

        default:
          DebugPrintEstado(nombres_estados[estado], nombres_eventos[evento]);
        break;
      }
    }
    break;

    case ESTADO_PUERTA_CERRADA:
    {
      switch (evento)
      {
        case EVENTO_ENCIMA_D_UMBRAL_MINIMO_DE_FUERZA:
        {
          DebugPrintEstado(nombres_estados[estado], nombres_eventos[evento]);
          actualizar_tono();
          encender_lcd();
          generar_codigo_correcto();
          limpiar_codigo_ingresado_display();
          limpiar_codigo_ingresado();
          estado = ESTADO_INGRESANDO_CODIGO;
        }
        break;

        case EVENTO_DEBAJO_D_UMBRAL_MINIMO_DE_FUERZA:
        {
          DebugPrintEstado(nombres_estados[estado], nombres_eventos[evento]);
          actualizar_tono();
          apagar_lcd();
        }
        break;

        default:
          DebugPrintEstado(nombres_estados[estado], nombres_eventos[evento]);
        break;
      }
    }
    break;

    case ESTADO_INGRESANDO_CODIGO:
    {
      switch (evento)
      {
        case EVENTO_DEBAJO_D_UMBRAL_MINIMO_DE_FUERZA:
        {
          DebugPrintEstado(nombres_estados[estado], nombres_eventos[evento]);
          actualizar_tono();
          apagar_lcd();
          estado = ESTADO_PUERTA_CERRADA;
        }
        break;

        case EVENTO_LIBERAR:
        {
          DebugPrintEstado(nombres_estados[estado], nombres_eventos[evento]);
          apagar_parlante();
          mover_microservomotor(PUERTA_ABIERTA);
          estado = ESTADO_PUERTA_ABIERTA;
        }
        break;

        case EVENTO_CODIGO_INCORRECTO:
        {
          DebugPrintEstado(nombres_estados[estado], nombres_eventos[evento]);
          manejar_actualizacion_de_codigo_en_pantalla();
        }
        break;

        case EVENTO_CODIGO_INCOMPLETO:
        {
          DebugPrintEstado(nombres_estados[estado], nombres_eventos[evento]);
          manejar_actualizacion_de_codigo_en_pantalla();
        }
        break;

        default:
          DebugPrintEstado(nombres_estados[estado], nombres_eventos[evento]);
        break;
      }
    }
    break;

    case ESTADO_PUERTA_ABIERTA:
    {
      switch (evento)
      {
        case EVENTO_OBJETO_NO_DETECTADO:
        {
          DebugPrintEstado(nombres_estados[estado], nombres_eventos[evento]);
          apagar_lcd();
          estado = ESTADO_PREPARADO;
        }
        break;

        case EVENTO_OBJETO_DETECTADO:
        {
          DebugPrintEstado(nombres_estados[estado], nombres_eventos[evento]);
        }
        break;

        default:
          DebugPrintEstado(nombres_estados[estado], nombres_eventos[evento]);
        break;
      }
    }
    break;
    }
}

// CICLO PRINCIPAL
void loop()
{
  maquina_de_estados_finita_insane_room();
}
