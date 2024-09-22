#include "painlessMesh.h"
#include <Arduino_JSON.h>

// MESH Details
#define MESH_PREFIX "whateveryoulike" // Nombre para tu red Mesh
#define MESH_PASSWORD "somethingSneak" // Contraseña para tu red Mesh
#define MESH_PORT 1515 // Puerto por defecto

// Pines para el LED RGB
#define RED_PIN 13
#define GREEN_PIN 14
#define BLUE_PIN 12

Scheduler userScheduler; // Para manejar tareas en la malla
painlessMesh mesh;

// Variables para almacenar datos recibidos
double temperature = 0;
double humidity = 0;
int nodeNumber = 1;
int redValue = 0, greenValue = 0, blueValue = 0; // Valores del LED RGB

// Prototipos de funciones
void controlLedRGB(double temp, double hum);
  /*Esta función ajusta el color del LED RGB según el valor de la temperatura:
    Si la temperatura es menor a 20°C, el LED se enciende en azul (frío).
     - Si la temperatura está entre 20°C y 30°C, el LED se enciende en verde (temperatura moderada).
     - Si la temperatura es mayor o igual a 30°C, el LED se enciende en rojo (calor).
     Los valores ajustados para los colores se asignan a los pines correspondientes
     y se usan para controlar el estado del LED RGB. */

void sendInfoToNode1(); 
  /* Esta función crea un objeto JSON que contiene información del nodo actual (Nodo 2)
     y los valores de los colores del LED RGB (redValue, greenValue, blueValue).
     Luego, esta información se envía a todos los nodos de la red mesh usando
     la función sendBroadcast() para mantener sincronizados los estados del LED RGB.
  */

void receivedCallback(uint32_t from, String &msg);
  /* Esta función se activa automáticamente cuando el nodo recibe un mensaje de otro nodo.
     1. El mensaje recibido es impreso en el monitor serial.
     2. Se intenta interpretar el mensaje como un objeto JSON.
     3. Si el JSON es válido, extrae los valores de nodo, temperatura y humedad del mensaje.
     4. Muestra estos valores en el monitor serial.
     5. Controla el LED RGB con la función controlLedRGB() según la temperatura y humedad recibidas.
     6. Finalmente, envía información de vuelta al Nodo 1 usando la función sendInfoToNode1().
  */
void newConnectionCallback(uint32_t nodeId);
  /* Esta función se llama cuando un nuevo nodo se conecta a la red mesh.
     Simplemente imprime en el monitor serial el ID del nodo que se ha conectado.
  */

void changedConnectionCallback();
  /* Esta función se llama cuando hay cambios en las conexiones entre nodos en la red mesh.
     Solo imprime un mensaje en el monitor serial indicando que ha habido cambios en las conexiones.
  */
void nodeTimeAdjustedCallback(int32_t offset);
  /* Esta función se activa cuando la hora del nodo es ajustada en la red mesh.
     Imprime en el monitor serial la nueva hora del nodo y el desfase (offset) aplicado.
  */

// Función para controlar el color del LED RGB según la temperatura
void controlLedRGB(double temp, double hum) {
  // Ajustar valores de color según la temperatura
  if (temp < 20) {
    redValue = 0;
    greenValue = 0;
    blueValue = 255;  // Azul para frío
  // Control del LED RGB
    analogWrite(RED_PIN, redValue);
    analogWrite(GREEN_PIN, greenValue);
    analogWrite(BLUE_PIN, blueValue);    
  } else if (temp >= 20 && temp < 30) {
    redValue = 0;
    greenValue = 255;
    blueValue = 0;   // Verde para temperatura moderada
   // Control del LED RGB
    analogWrite(RED_PIN, redValue);
    analogWrite(GREEN_PIN, greenValue);
    analogWrite(BLUE_PIN, blueValue); 
  } else {
    redValue = 255;
    greenValue = 0;
    blueValue = 0;   // Rojo para calor

    // Control del LED RGB
    analogWrite(RED_PIN, redValue);
    analogWrite(GREEN_PIN, greenValue);
    analogWrite(BLUE_PIN, blueValue);
  }

  // Control del LED RGB
  //analogWrite(RED_PIN, redValue);
  //analogWrite(GREEN_PIN, greenValue);
  //analogWrite(BLUE_PIN, blueValue);
}

// Función para enviar información de vuelta al Nodo 1
void sendInfoToNode1() {
  JSONVar jsonResponse;

  // Información sobre el estado del LED RGB
  jsonResponse["node"] = 2; // Identificador de este nodo (Nodo 2)
  jsonResponse["ledRed"] = redValue;
  jsonResponse["ledGreen"] = greenValue;
  jsonResponse["ledBlue"] = blueValue;

  String response = JSON.stringify(jsonResponse);
  mesh.sendBroadcast(response); // Enviar el mensaje a todos los nodos

  Serial.println("Enviando estado del LED RGB al Nodo 1: ");
  Serial.println(response);
}

// Callback al recibir un mensaje
void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Mensaje recibido desde %u: %s\n", from, msg.c_str());

  // Parsear el mensaje recibido como JSON
  JSONVar myObject = JSON.parse(msg.c_str());

  // Verificar si el JSON es válido
  if (JSON.typeof(myObject) == "undefined") {
    Serial.println("Error al parsear JSON");
    return;
  }

  // Extraer los valores del JSON
  nodeNumber = (int)myObject["node"];
  temperature = (double)myObject["temp"];
  humidity = (double)myObject["hum"];

  // Mostrar los valores recibidos en el monitor serial
  Serial.print("Nodo: ");
  Serial.println(nodeNumber);
  Serial.print("Temperatura: ");
  Serial.print(temperature);
  Serial.println(" °C");
  Serial.print("Humedad: ");
  Serial.print(humidity);
  Serial.println(" %");

  // Controlar el LED RGB según la temperatura y humedad
  controlLedRGB(temperature, humidity);

  // Enviar información de vuelta al Nodo 1
  sendInfoToNode1();
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("Nueva conexión, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.println("Cambios en las conexiones");
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Hora ajustada %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

// Función de configuración inicial
void setup() {
  Serial.begin(115200);

  // Inicializar pines del LED RGB como salida
  pinMode(RED_PIN, OUTPUT);
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  // Inicializar la red mesh
  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
}

void loop() {
  // Actualizar el mesh
  mesh.update();
}
