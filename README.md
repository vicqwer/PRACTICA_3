Práctica 3 - Contador BCD Ascendente/Descendente con FreeRTOS, pvParameters y Task Manager

Integrantes

1. Victor Hugo Barrera Garcia
2. Sergio Garcia Hernandez

---

Descripción de la práctica

En esta práctica se desarrolló un contador BCD utilizando una tarjeta ESP32 y el sistema operativo FreeRTOS.  
El objetivo principal fue controlar un contador de 0 a 9 mediante tareas independientes, botones físicos y una tarea central encargada de administrar los eventos del sistema.

El contador se visualiza mediante cuatro LEDs, donde cada LED representa un bit del valor BCD mostrado. Además, se utilizan tres botones para modificar el comportamiento del sistema: uno para iniciar o pausar el conteo, otro para cambiar la dirección entre ascendente y descendente, y otro para modificar la velocidad de actualización.

La lógica del programa se dividió en módulos para mantener el código organizado. Se empleó `pvParameters` para reutilizar funciones de tarea con diferentes configuraciones, y `TaskHandle_t` para que el Task Manager pudiera suspender, reanudar y consultar el estado de las tareas.

---

Funcionamiento general

El sistema implementa las siguientes funciones:

1. Conteo BCD de `0` a `9`.
2. Conteo ascendente y descendente.
3. Cambio de velocidad entre `500 ms` y `250 ms`.
4. Inicio y pausa del sistema mediante un botón físico.
5. Conservación del último valor mostrado al pausar.
6. Reanudación del conteo desde el mismo valor.
7. Monitoreo de eventos y estados mediante mensajes UART.
8. Uso de `vTaskSuspend()` y `vTaskResume()` para controlar la ejecución del contador.
9. Uso de `pvParameters` para parametrizar tareas.
10. Uso de `TaskHandle_t` para administrar tareas desde el Task Manager.

---

Botones del sistema

| Botón | Función |
|------|---------|
| Start/Pause | Inicia o pausa el contador |
| Dirección | Alterna entre conteo ascendente y descendente |
| Velocidad | Alterna entre periodo de 500 ms y 250 ms |

Cuando el sistema se encuentra pausado, los botones de dirección y velocidad son ignorados.  
Esto evita que el usuario cambie la configuración mientras el contador está detenido.

---

Tabla de pines

| Elemento | GPIO |
|---------|------|
| LED B0 | GPIO2 |
| LED B1 | GPIO4 |
| LED B2 | GPIO16 |
| LED B3 | GPIO17 |
| Botón Dirección | GPIO18 |
| Botón Velocidad | GPIO19 |
| Botón Start/Pause | GPIO21 |

---

Estructura del proyecto

| Archivo | Descripción |
|--------|-------------|
| `main.c` | Punto de entrada del programa. Inicializa el estado del sistema y crea las tareas. |
| `app_task.c` / `app_task.h` | Creación de tareas y administración del Task Manager. |
| `buttons.c` / `buttons.h` | Configuración y lectura de los botones físicos. |
| `counter.c` / `counter.h` | Lógica del contador BCD ascendente y descendente. |
| `leds.c` / `leds.h` | Configuración y actualización de los LEDs del contador. |
| `system_state.c` / `system_state.h` | Manejo del estado global del sistema. |
| `app_config.h` | Definición de pines, periodos y parámetros generales. |
| `platformio.ini` | Configuración del proyecto en PlatformIO con ESP-IDF. |

---

Descripción del Task Manager

El Task Manager es la tarea encargada de revisar los eventos generados por los botones y aplicar los cambios correspondientes al sistema.

Sus funciones principales son:

1. Detectar si el usuario presionó el botón Start/Pause.
2. Suspender o reanudar la tarea del contador.
3. Permitir cambios de velocidad y dirección únicamente cuando el sistema está activo.
4. Consultar estados de tareas mediante `eTaskGetState()`.
5. Mostrar mensajes de depuración por UART.

De esta forma, la lógica de control no queda dispersa en todas las tareas, sino concentrada en una tarea central.

---

Uso de pvParameters

En esta práctica se utilizó `pvParameters` para enviar información específica a cada tarea sin tener que crear funciones diferentes para cada caso.

Esto permite que una misma función pueda comportarse de manera distinta dependiendo de los parámetros recibidos.  
Por ejemplo, una tarea puede recibir el número de botón que debe leer o el LED que debe controlar.

El uso de `pvParameters` ayuda a reducir código repetido y facilita que el programa sea más escalable.

---

Uso de TaskHandle_t

`TaskHandle_t` se utilizó para guardar referencias a las tareas creadas.  
Gracias a estos manejadores, el Task Manager puede controlar directamente ciertas tareas del sistema.

Con `TaskHandle_t` fue posible:

1. Suspender la tarea del contador.
2. Reanudar la tarea del contador.
3. Consultar el estado actual de una tarea.
4. Administrar el comportamiento del sistema desde una tarea central.

---

Estados de FreeRTOS observados

Durante la práctica se analizaron diferentes estados de las tareas:

| Estado | Descripción |
|-------|-------------|
| RUNNING | La tarea se está ejecutando en ese instante. |
| READY | La tarea está lista para ejecutarse, pero espera turno del planificador. |
| BLOCKED | La tarea está esperando un tiempo o evento, por ejemplo al usar `vTaskDelay()`. |
| SUSPENDED | La tarea fue detenida manualmente con `vTaskSuspend()` y no se ejecutará hasta usar `vTaskResume()`. |

La diferencia más importante observada fue que una tarea en estado `BLOCKED` puede regresar automáticamente a ejecución cuando termina su espera, mientras que una tarea en estado `SUSPENDED` necesita ser reanudada explícitamente.

---
## Salida esperada en monitor serial

Durante la ejecución, el programa muestra mensajes de depuración por UART generados principalmente por el Task Manager. 
Estos mensajes permiten observar el modo actual del sistema, el valor del contador, la dirección, la velocidad y el estado de las tareas.

Ejemplo de salida esperada:

```text
I MANAGER: Sistema PAUSADO
I MANAGER: ------ ESTADOS ------
I MANAGER: COUNTER: SUSPENDED
I MANAGER: BTN_START: BLOCKED
I MANAGER: BTN_DIR: SUSPENDED
I MANAGER: BTN_SPEED: SUSPENDED
I MANAGER: LED_B0: BLOCKED
I MANAGER: LED_B1: BLOCKED
I MANAGER: LED_B2: BLOCKED
I MANAGER: LED_B3: BLOCKED
I MANAGER: Valor=0 | Modo=PAUSED | Direccion=UP | Periodo=500 ms

I MANAGER: Sistema RUNNING
I MANAGER: Nueva direccion: DOWN
I MANAGER: Nueva velocidad: 250 ms
I MANAGER: Sistema PAUSADO
````

Cuando el sistema está pausado, el contador se encuentra en estado `SUSPENDED` y los botones de dirección y velocidad también
se suspenden. Solamente permanece activo el botón `BTN_START`, ya que este permite reanudar el sistema.

Cuando el sistema está en ejecución, el Task Manager permite cambiar la dirección y la velocidad del conteo. 
Además, cada cierto tiempo imprime el estado de las tareas mediante `eTaskGetState()`, lo que permite observar estados como 
`BLOCKED`, `READY`, `RUNNING` y `SUSPENDED`.

---

Compilación y carga

Para compilar el proyecto:

```bash
pio run
```

Para cargar el programa en la ESP32:

```bash
pio run --target upload
```

Para abrir el monitor serial:

```bash
pio device monitor -b 115200
```

---
 Preguntas guía

1. ¿Qué diferencia existe entre BLOCKED y SUSPENDED?

Una tarea en estado `BLOCKED` está detenida temporalmente porque espera que ocurra algo, por ejemplo que termine un
retardo con `vTaskDelay()` o que llegue algún evento. Cuando se cumple esa condición, la tarea puede volver automáticamente al estado `READY`.

Una tarea en estado `SUSPENDED` fue detenida manualmente mediante `vTaskSuspend()`. A diferencia de `BLOCKED`, 
no regresa automáticamente a ejecución; necesita que otra tarea la reactive usando `vTaskResume()`.

---

2. ¿Por qué `vTaskDelay()` coloca una tarea en estado BLOCKED?

`vTaskDelay()` coloca una tarea en estado `BLOCKED` porque le indica al planificador que esa tarea no debe ejecutarse durante cierto 
número de ticks. Mientras pasa ese tiempo, el CPU queda libre para ejecutar otras tareas que estén listas.

Esto es mejor que usar un retardo con ciclos `for`, porque un `for` mantiene ocupado al procesador sin permitir que FreeRTOS administre 
correctamente el tiempo de CPU.

---

3. ¿Qué diferencia existe entre `vTaskDelay()` y un Software Timer?

`vTaskDelay()` se usa dentro de una tarea para detener temporalmente su ejecución. La tarea queda bloqueada durante el tiempo indicado
y después vuelve a estar lista para ejecutarse.

Un Software Timer es un temporizador administrado por FreeRTOS que ejecuta una función de callback cuando se cumple un tiempo determinado.
No necesita que una tarea esté detenida esperando, por lo que es útil para ejecutar acciones periódicas o eventos temporizados de forma más ordenada.

---

4. ¿Qué función cumple el Idle Task?

El Idle Task es una tarea interna de FreeRTOS que se ejecuta cuando no hay otras tareas listas para correr. Su función principal
es mantener ocupado al sistema cuando no existe trabajo pendiente y permitir que FreeRTOS realice tareas internas, como limpieza de recursos de tareas eliminadas.

También sirve como referencia para observar que el procesador no siempre está ejecutando tareas de aplicación.

---

5. ¿Cómo decide FreeRTOS cuál tarea ejecutar cuando varias tienen la misma prioridad?

Cuando varias tareas tienen la misma prioridad y están en estado `READY`, FreeRTOS puede repartir el tiempo de CPU entre ellas 
mediante Round Robin. Esto significa que las tareas se van turnando para ejecutarse en intervalos definidos por el tick del sistema.

Si una tarea de mayor prioridad queda lista, FreeRTOS le dará preferencia sobre las tareas de menor prioridad.

---

6. ¿Qué ventajas aporta `pvParameters`?

`pvParameters` permite enviar información específica a una tarea al momento de crearla con `xTaskCreate()`. Esto hace 
posible reutilizar una misma función de tarea con diferentes configuraciones.

En esta práctica, el uso de `pvParameters` permite que una misma función pueda servir para diferentes LEDs o botones, 
cambiando únicamente los parámetros como GPIO, nombre o tipo de botón. Esto reduce código repetido y facilita ampliar el sistema.

---

7. ¿Qué ventajas aporta `TaskHandle_t`?

`TaskHandle_t` permite guardar una referencia a una tarea creada. Con esa referencia, otra parte del programa puede controlar o consultar esa tarea.

En esta práctica, el Task Manager utiliza `TaskHandle_t` para suspender y reanudar la tarea del contador con `vTaskSuspend()` 
y `vTaskResume()`. También permite consultar el estado de las tareas mediante `eTaskGetState()`.

---

8. ¿Qué ocurriría si el contador se implementara con variables globales únicamente?

Si el contador se implementara solo con variables globales, el programa sería menos modular y más difícil de mantener. Todas 
las partes del código dependerían directamente de las mismas variables, aumentando el riesgo de errores o cambios inesperados.

Además, no se aprovecharían correctamente las ventajas de FreeRTOS, como separar funciones en tareas independientes, suspender o 
reanudar tareas, monitorear estados y organizar la lógica mediante un Task Manager.
---

Estado

Práctica validada en ESP32.
El contador BCD muestra valores de 0 a 9, permite cambiar dirección, alternar velocidad y pausar o reanudar el sistema desde el último valor mostrado.

```
```
