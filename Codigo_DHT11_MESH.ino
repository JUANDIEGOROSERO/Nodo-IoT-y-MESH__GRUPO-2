#include <Adafruit_Sensor.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>
#include "DHT.h"
#include "esp_sleep.h"  // Librería para controlar el deep sleep

// MESH Details
#define MESH_PREFIX "whateveryoulike"  // Nombre para tu MESH
#define MESH_PASSWORD "somethingSneak"    // Contraseña para tu MESH
#define MESH_PORT 1515            // Puerto por defecto

// Configuración del sensor DHT
#define DHTPIN 4       
#define DHTTYPE DHT11  

DHT dht(DHTPIN, DHTTYPE);

int nodeNumber = 2;  // Número de este nodo
String readings;     // String para almacenar las lecturas

Scheduler userScheduler;  // Para manejar las tareas
painlessMesh mesh;

// Prototipos de funciones
void sendMessage();
  /* Esta función obtiene las lecturas del sensor llamando a la función getReadings(),
     y envía la información a todos los nodos en la red mesh utilizando la función
     mesh.sendBroadcast(). También imprime el mensaje en el monitor serial. 
     (Opcionalmente, se puede habilitar el modo de deep sleep después de enviar el mensaje).
  */

String getReadings();
  /* Esta función recoge los datos del sensor DHT11, específicamente la
     temperatura y la humedad. Luego, estos valores se almacenan en un 
     objeto JSON, junto con el número del nodo. El objeto JSON se convierte
     a una cadena (string) para ser enviada a otros nodos. Los valores también 
     se imprimen en el monitor serial para facilitar la depuración.
  */

void goToSleep(); 
  /* Esta función pone el microcontrolador ESP32 en modo de bajo consumo (deep sleep) 
     durante 10 segundos. Configura el temporizador de sueño profundo utilizando
     esp_sleep_enable_timer_wakeup() para despertar al ESP32 después del tiempo
     especificado. Luego, inicia el sueño profundo con esp_deep_sleep_start().
  */

// Crear tareas para enviar mensajes y obtener lecturas
Task taskSendMessage(TASK_SECOND * 5, TASK_FOREVER, &sendMessage);  

// Función para obtener lecturas del sensor DHT
String getReadings() {
  JSONVar jsonReadings;

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  
  // Mostrar los valores en el monitor serial
  Serial.print(F("Humidity: "));
  Serial.print(humidity);
  Serial.print(F("%  Temperature: "));
  Serial.print(temperature);
  Serial.println(F("°C "));

// Asignar valores de temperatura y humedad al objeto JSON
  jsonReadings["node"] = nodeNumber;
  jsonReadings["temp"] = temperature;
  jsonReadings["hum"] = humidity;
  
  readings = JSON.stringify(jsonReadings);
  return readings;
}

// Función para enviar el mensaje a la red mesh
void sendMessage() {
  String msg = getReadings();
  Serial.println(msg);
  mesh.sendBroadcast(msg);
  
  // Después de enviar el mensaje, entra en deep sleep
  //goToSleep();
}

// Función para entrar en deep sleep
void goToSleep() {
  Serial.println("Entrando en Deep Sleep durante 10 segundos...");
  
  // Configurar el temporizador para despertar después de 10 segundos
  esp_sleep_enable_timer_wakeup(10 * 1000000);  // 10 segundos (en microsegundos)
  
  // Poner al ESP32 en modo deep sleep
  esp_deep_sleep_start();
}

void receivedCallback(uint32_t from, String &msg) {
  /* Esta función se activa cuando un mensaje es recibido desde otro nodo de la red mesh.
     Muestra en el monitor serial el ID del nodo emisor y el mensaje recibido.
  */
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
  /* Esta función se ejecuta cuando un nuevo nodo se conecta a la red mesh.
     Muestra en el monitor serial el ID del nodo recién conectado.
  */
  Serial.printf("New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  /* Esta función se activa cuando hay cambios en las conexiones de la red mesh,
     como cuando un nodo se desconecta o una conexión cambia. Se imprime en el 
     monitor serial que hubo cambios en las conexiones.
  */
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
  /* Esta función se activa cuando la hora del nodo se ajusta dentro de la red mesh.
     Imprime en el monitor serial la hora ajustada y el desfase (offset) aplicado.
  */
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void setup() {
  Serial.begin(115200);
  
  // Iniciar el sensor DHT
  dht.begin();

  // Configurar la malla mesh
  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  // Agregar la tarea de enviar mensaje
  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();  // Habilitar la tarea para que se ejecute una vez
}

void loop() {
  // Actualizar la malla
  mesh.update();
}