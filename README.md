<h1 align="center"> DHCP-PROTOCOL</h1>


## Contribuidores


- Kristian Restrepo Osorio


- Evelyn Alejandra Zapata Tobón


- Valentina Giraldo Noreña

## Conocimientos Previos

Para comprender este proyecto, es necesario tener un conocimiento básico de los siguientes conceptos:

### 1. Sockets:
Un socket es una abstracción que permite a las aplicaciones enviar y recibir datos a través de la red. Es la interfaz entre la aplicación y el sistema de red de bajo nivel. En este proyecto, se utilizan sockets tipo SOCK_DGRAM (Datagramas) para la implementación de un servidor y un cliente que se comunican mediante el protocolo UDP. Los sockets de tipo SOCK_RAW permiten el acceso a los protocolos de red subyacentes de bajo nivel. Con ellos, una aplicación puede enviar y recibir paquetes de red de forma más directa, sin la sobrecarga de un protocolo de transporte como TCP o UDP. Esto es útil para aplicaciones que necesitan manipular los encabezados de los paquetes o implementar protocolos personalizados.

### 2. API de Sockets Berkeley:
La *API Sockets Berkeley* es una interfaz para la comunicación en red que permite a los desarrolladores gestionar la transmisión de datos entre aplicaciones. Proporciona las herramientas para crear, enlazar, escuchar, enviar y recibir datos a través de sockets, ya sea en el protocolo TCP o UDP.

### 3. Protocolo DHCP:
El *protocolo DHCP* (Dynamic Host Configuration Protocol) permite la asignación dinámica de direcciones IP a dispositivos en una red. El servidor DHCP asigna una dirección IP a cada dispositivo que lo solicita. Los mensajes clave en este protocolo son:
- DHCPDISCOVER: El cliente envía este mensaje para buscar servidores DHCP en la red.
- DHCPOFFER: El servidor responde con una oferta de dirección IP.
- DHCPREQUEST: El cliente confirma que acepta la IP ofrecida.
- DHCPACK: El servidor confirma la asignación de la dirección IP al cliente.

![image](https://github.com/user-attachments/assets/ab38c3aa-e4fe-48c0-ba2d-8f9d17baca9a)


### 4. Programación en C y Concurrencia:
Se deben tener conocimientos básicos de *programación en C, manejo de estructuras, punteros y gestión de memoria. Además, es importante comprender los principios de la **concurrencia* y el uso de *hilos* para manejar múltiples conexiones al servidor DHCP de forma simultánea.



## Introducción dhcp_multiple_networks


La implementación de un DHCP relay es crucial para el correcto funcionamiento de servidores y clientes en diferentes subredes y redes, ya que permite la transmisión de mensajes DHCP entre clientes y servidores que no se encuentran en la misma red local. Esto es especialmente importante en redes de gran escala, donde no es eficiente ni práctico tener un servidor DHCP en cada subred.

El relay actúa como intermediario, reenviando las solicitudes de los clientes (como DHCPDISCOVER y DHCPREQUEST) al servidor DHCP, y posteriormente devolviendo las respuestas del servidor (como DHCPOFFER y DHCPACK) al cliente. Este proceso facilita la asignación centralizada de direcciones IP y otros parámetros de configuración de red, independientemente de la ubicación física de los dispositivos en la red.

En el proyecto en la parte en la que se incluye múltiples subredes, el uso de un DHCP relay asegura que los clientes en diferentes subredes puedan recibir correctamente sus configuraciones de red, mejorando así la eficiencia y escalabilidad del sistema. Asimismo es importante explicar, el contenido establecido dentro de la carpeta dhcp_multiple_networks, esta se divide en 3 subcarpetas, cada una con una estructura moduralizada semejante entre ellas, encontrando como primera la carpeta client, luego relay y por último server.

Es indispensable resaltar que dentro de cada archivo contenido en cada carpeta se especifica línea por línea mediante comentarios el funcionamiento del código, ya sea sus métodos, funciones, sockets, uso de puertos y demás funcionalidades implementadas en el proyecto. Por ello, se recomienda darle un vistazo a cada uno de los archivos contenidos en las carpetas del código.


# Implementación del Cliente DHCP

## 1. Programa Utilizado

Este proyecto implementa un *cliente DHCP* en *C*, diseñado para manejar la asignación, renovación y liberación de direcciones IP mediante sockets y multihilos. El cliente interactúa con un servidor DHCP para obtener una dirección IP y la configuración de red para una interfaz específica.

## 2. Dependencias de Configuración

Para compilar y ejecutar el cliente DHCP, es necesario instalar las siguientes dependencias:

- *Compilador GCC*: para compilar el código en C.
- *Bibliotecas de desarrollo*:
  - `pthread`: para manejo de hilos.
  - `netinet/in.h`, `arpa/inet.h`: para manipulación de direcciones IP y sockets.
  - `sys/socket.h`: para creación y manejo de sockets.
  - `net/if.h`: para manipulación de interfaces de red.
  - `termios.h`, `fcntl.h`: para control de terminal y archivos.
  - *Otras bibliotecas estándar de C*.

Puedes instalar las dependencias con el siguiente comando:

```bash
sudo apt-get update
sudo apt-get install build-essential
```

## 3. Configuración de Subredes y Adaptadores

### Configuración de Subredes

Para probar el cliente DHCP, asegúrate de que tu red esté configurada correctamente:

- **Subred**: 192.168.1.0/24
- **Puerta de enlace**: 192.168.1.1
- **Servidor DNS**: 8.8.8.8
- **Rango DHCP**: 192.168.1.100 - 192.168.1.200

Configura tu servidor DHCP con este rango y parámetros.

### Adaptadores e Interfaces

El cliente interactúa con interfaces de red como `eth0` o `enp0s3`. Asegúrate de que la interfaz de red que vas a utilizar:

- Está disponible y habilitada.
- No tiene una dirección IP asignada previamente.

Verifica tus interfaces de red con:

```bash
ip addr show
```

Para bajar y subir una interfaz:

```bash
sudo ip link set dev enp0s3 down
sudo ip link set dev enp0s3 up
```

## 4. Tipos de Sockets Utilizados

### Socket del Servidor

El servidor DHCP utiliza un socket `SOCK_DGRAM` (UDP) en el puerto 67 para enviar y recibir mensajes de los clientes.

### Socket del Cliente

El cliente DHCP utiliza:

- `SOCK_DGRAM`: para enviar mensajes broadcast al servidor en el puerto 67.
- `SOCK_RAW`: para recibir mensajes broadcast del servidor en el puerto 68, especialmente cuando el cliente no tiene una dirección IP asignada.

## 5. Estructura del Proyecto

El proyecto está organizado en las siguientes carpetas:

- **socket/**: Contiene el código para la creación y gestión de sockets.
  - `socket.h`: Declaraciones de funciones para manejo de sockets.
  - `socket.c`: Implementación de funciones para inicializar sockets UDP y RAW, y para enviar y recibir mensajes.

- **dhcp/**: Maneja la lógica específica del protocolo DHCP.
  - `dhcp.h`: Declaraciones de funciones relacionadas con DHCP.
  - `dhcp.c`: Implementación de funciones para configurar y procesar mensajes DHCP (DISCOVER, REQUEST, ACK).

- **utils/**: Proporciona funciones utilitarias para soporte del programa principal.
  - `utils.h`: Declaraciones de funciones utilitarias.
  - `utils.c`: Implementación de funciones como procesamiento de paquetes, obtención de dirección MAC, asignación de IP a interfaces y manejo de entrada del usuario.

- **constants/**: Define constantes utilizadas en todo el proyecto.
  - `constants.h`: Definición de constantes como puertos, tamaños de buffer y direcciones IP de broadcast.

- **include/**: Contiene recursos compartidos y variables globales para sincronización.
  - `shared_resources.h`: Declaraciones de mutexes y variables de condición para manejo de hilos.

- **error/**: Maneja la gestión de errores en el programa.
  - `error.h`: Declaraciones para manejo de errores.
  - `error.c`: Implementación de funciones para imprimir mensajes de error y terminar el programa en caso de fallos críticos.

- **structs/**: Define las estructuras de datos utilizadas en el programa.
  - `structs.h`: Definiciones de estructuras como `dhcp_message` y argumentos para funciones de hilos.

## 6. Cómo Compilar y Ejecutar el Cliente

### Compilación

Utiliza el compilador GCC para compilar el cliente:

```bash
gcc -o compiled main.c socket/socket.c dhcp/dhcp.c utils/utils.c error/error.c 
```

### Ejecución

Ejecuta el cliente con privilegios de administrador:

```bash
sudo ./compiled
```

El programa iniciará el proceso DHCP DISCOVER para obtener una dirección IP para la interfaz especificada (por defecto `enp0s3`).




# Implementación Relay DHCP

## Descripción del Programa

El proyecto implementa un *relay DHCP* (Dynamic Host Configuration Protocol) diseñado para reenviar mensajes entre clientes y servidores DHCP en diferentes subredes. Un relay DHCP es necesario cuando los clientes y servidores DHCP no están en la misma red física o lógica, lo que impide la comunicación directa. Este relay actúa como intermediario, recibiendo mensajes DHCP de los clientes y reenvíandolos al servidor DHCP, y viceversa.

El programa está diseñado para ser robusto y eficiente, gestionando correctamente las conexiones entre el servidor DHCP y los clientes a través de diferentes interfaces de red. Soporta varias fases del protocolo DHCP, incluyendo *DHCPDISCOVER, DHCPOFFER, DHCPREQUEST, DHCPACK* y *DHCPRELEASE*.

## Dependencias de Configuración

Para ejecutar este proyecto, es necesario tener un entorno Linux con las siguientes dependencias:

### Herramientas y Bibliotecas

- *Compilador GCC*: Se necesita para compilar el código fuente en C.
- *Biblioteca de Sockets*: El programa utiliza `sys/socket.h` para crear y gestionar sockets de red.
- `arpa/inet.h`: Proporciona funciones para la manipulación de direcciones IP y su conversión entre diferentes formatos (por ejemplo, `inet_addr`).
- `netinet/in.h`: Necesaria para trabajar con direcciones de red en la familia de protocolos `AF_INET` (usado para IPv4).
- `unistd.h`: Permite cerrar los descriptores de archivos y realizar otras operaciones del sistema operativo como el control de procesos.
- `string.h`: Para manipular cadenas y gestionar bloques de memoria, como `memset` y `memcpy`.
- `net/if.h`: Proporciona definiciones y estructuras necesarias para configurar interfaces de red, como `struct ifreq`, utilizado con `ioctl()`.

### Configuración del Sistema Operativo

Es recomendable que el proyecto se ejecute en un sistema operativo Linux, preferiblemente una distribución que facilite el manejo de redes como Ubuntu, Fedora o Arch Linux. Además, es importante tener privilegios de superusuario para poder gestionar los sockets y las interfaces de red.

### Instalación de Dependencias

Para instalar las dependencias necesarias en un sistema basado en Debian/Ubuntu, ejecuta lo siguiente:

```bash
sudo apt-get update
sudo apt-get install build-essential net-tools
```

## Configuración de Subredes

El relay DHCP debe configurarse correctamente para operar entre distintas subredes. Esto incluye la configuración de las interfaces de red que van a gestionar las solicitudes y respuestas entre el servidor DHCP y los clientes.

### Ejemplo de Configuración de Subredes

- **Subred 1 (Clientes)**: 192.168.56.0/24
  - Los clientes DHCP están en esta subred y enviarán solicitudes DHCP.
  - El relay escucha en esta subred para recibir los mensajes provenientes de los clientes.

- **Subred 2 (Servidor)**: 192.168.57.0/24
  - El servidor DHCP está ubicado en esta subred y no puede recibir mensajes directamente de la subred 1.
  - El relay reenviará los mensajes de los clientes a esta subred.

El relay se asegura de modificar el campo `giaddr` (Gateway IP Address) en los mensajes DHCP, indicando la IP del relay para que el servidor sepa a qué subred reenviar la respuesta.

### Configuración IP de las Interfaces

Debes asegurarte de que las interfaces de red estén configuradas correctamente en tu máquina para que el relay pueda operar. Ejemplo de configuración:

```bash
sudo ifconfig enp0s3 192.168.56.1 netmask 255.255.255.0
sudo ifconfig enp0s8 192.168.57.1 netmask 255.255.255.0
```

## Adaptadores e Interfaces

Este proyecto utiliza dos adaptadores de red o interfaces específicas para manejar la comunicación entre el cliente y el servidor DHCP.

- **CLIENT_ASSOCIATED_INTERFACE**: Esta interfaz está destinada a la subred donde se encuentran los clientes. En el proyecto, es representada por `enp0s3` y es utilizada para escuchar y reenviar mensajes provenientes de los clientes DHCP.

- **SERVER_ASSOCIATED_INTERFACE**: Esta interfaz está asociada con la subred donde se encuentra el servidor DHCP. En este proyecto, es representada por `enp0s8` y se utiliza para reenviar los mensajes de los clientes al servidor y recibir respuestas desde el servidor.

El relay asegura que cada mensaje DHCP se envíe a través de la interfaz de red adecuada, dependiendo de su origen y destino.

## Tipo de Socket del Servidor

El servidor DHCP utiliza un socket de tipo UDP (`SOCK_DGRAM`), que es ideal para mensajes cortos que no requieren una conexión establecida (como los mensajes DHCP). Este socket escucha en el puerto 1067, que es el puerto donde el relay reenvía las solicitudes desde los clientes.

UDP es adecuado para DHCP porque se basa en un protocolo sin conexión, lo que lo hace más ligero y rápido que TCP, que requiere el establecimiento de una conexión antes de transmitir datos.

## Tipo de Socket del Cliente

El cliente también utiliza un socket de tipo UDP (`SOCK_DGRAM`), pero este escucha en el puerto 1068. El relay se asegura de que los mensajes reenviados desde el servidor DHCP lleguen a este puerto del cliente, utilizando broadcast si es necesario.

UDP es crucial en el lado del cliente ya que permite que múltiples clientes envíen solicitudes al mismo tiempo sin la necesidad de establecer conexiones dedicadas, lo que sería ineficiente.

## Contenido de las Carpetas

El proyecto sigue una estructura modular para facilitar el desarrollo y mantenimiento. Aquí se presenta un resumen del contenido de cada carpeta principal:

- **/socket/**:
  - Contiene el archivo `socket.c` que define las funciones para crear, inicializar y gestionar los sockets. También contiene funciones para recibir y enviar mensajes a través del socket.

- **/structs/**:
  - Contiene las estructuras de datos necesarias para el manejo de los mensajes DHCP, como la estructura `dhcp_message`, que define el formato del mensaje DHCP según el protocolo.

- **/constants/**:
  - Almacena las constantes importantes como los puertos utilizados por el servidor y el cliente DHCP, la dirección IP del servidor, el tamaño del buffer y las interfaces de red asociadas.

- **/error/**:
  - Implementa una función para manejar los errores en el sistema. Utiliza `fprintf` para enviar mensajes a `stderr` cuando ocurre un error crítico, y finaliza la ejecución con `exit()`.

- **/dhcp/**:
  - Contiene las funciones específicas para el protocolo DHCP, incluyendo la función `get_dhcp_message_type` que permite determinar el tipo de mensaje DHCP recibido.

## Saberes Previos

Este proyecto requiere ciertos conocimientos previos para ser comprendido y ejecutado correctamente. A continuación se enumeran los saberes necesarios:

### Conocimientos en Programación en C

Es necesario tener un buen entendimiento de la programación en C, especialmente en las áreas de:

- **Manipulación de Punteros**: El manejo de punteros es esencial para trabajar con buffers y estructuras de datos.
- **Manejo de Sockets**: Familiaridad con la creación, vinculación y uso de sockets, particularmente sockets UDP.
- **Manejo de Errores**: Es importante comprender cómo capturar y manejar errores en C utilizando mecanismos como `stderr`, `fprintf`, y `exit()`.

### Redes y Protocolo DHCP

Debes tener un conocimiento básico de las redes y, específicamente, de cómo funciona el protocolo DHCP. Esto incluye:

- **Mensajes DHCP**: Conocer los tipos de mensajes como DHCPDISCOVER, DHCPOFFER, DHCPREQUEST, DHCPACK, y DHCPRELEASE.
- **Funcionamiento del Relay**: Comprender cómo un relay DHCP actúa como intermediario entre diferentes subredes y cómo modifica los mensajes para que se reenvíen correctamente.
- **Subredes y Rutas**: Comprender cómo se configuran subredes y enrutamientos en Linux, y cómo el relay facilita la comunicación entre estas subredes.

### Administración de Sistemas Linux

Se requiere un nivel intermedio de conocimientos en la administración de sistemas Linux, incluyendo:

- **Configuración de Interfaces de Red**: Saber cómo configurar y gestionar interfaces de red utilizando herramientas como `ifconfig` o `ip`.
- **Permisos de Superusuario**: Entender cómo ejecutar comandos con privilegios de root para abrir sockets en puertos específicos.

## Ejecución del Proyecto

Para compilar y ejecutar el proyecto:

### Compilación

Compila el código con el siguiente comando:

```bash
gcc -o dhcp_relay main.c socket/socket.c dhcp/dhcp.c error/error.c -o relay
```

Asegúrate de tener configuradas las interfaces de red correctamente (como se explicó en la sección de configuración de subredes).

### Ejecución

Ejecuta el programa con permisos de superusuario:

```bash
sudo ./relay
```

El relay comenzará a escuchar en las interfaces configuradas y reenviará mensajes DHCP entre los clientes y el servidor.

---



## Introducción dhcp_same_network


La implementación de un servidor y cliente corriendo en una misma red es fundamental para asegurar una comunicación eficiente y controlada dentro de cualquier sistema de red. En este proyecto, la conexión entre el cliente y el servidor a través del protocolo DHCP permite que los dispositivos dentro de la misma red local se configuren automáticamente, sin necesidad de intervención manual.

El servidor DHCP, al estar ubicado en la misma red que los clientes, puede responder rápidamente a las solicitudes de estos, asignando direcciones IP y otros parámetros de red (como la máscara de subred, puerta de enlace y servidores DNS) de manera dinámica y automática. Esto optimiza la administración de recursos en redes locales, evitando conflictos de IP y garantizando que todos los dispositivos conectados puedan comunicarse de manera adecuada dentro de la red.

El hecho de que el cliente y el servidor corran en una misma red también minimiza los tiempos de latencia en la asignación de direcciones IP, ya que la comunicación no necesita pasar por routers o dispositivos externos, lo que agiliza el proceso de configuración inicial de la red. Además, al correr en un entorno controlado, permite realizar pruebas eficientes de concurrencia y manejo de solicitudes, asegurando que el servidor pueda gestionar múltiples clientes de manera simultánea sin perder estabilidad ni rendimiento.

Asimismo es importante explicar, el contenido establecido dentro de la carpeta dhcp_same_networks, esta se divide en 2 subcarpetas, cada una con una estructura moduralizada semejante entre ellas, encontrando como primera la carpeta client, y luego server.

Es indispensable resaltar que dentro de cada archivo contenido en cada carpeta se especifica línea por línea mediante comentarios el funcionamiento del código, ya sea sus métodos, funciones, sockets, uso de puertos y demás funcionalidades implementadas en el proyecto. Por ello, se recomienda darle un vistazo a cada uno de los archivos contenidos en las carpetas del código.

# Implementación del Server DHCP

## Desarrollo

### Servidor DHCP

#### Funcionalidades
El servidor DHCP es el núcleo de este proyecto. Se encarga de gestionar las solicitudes de los clientes y asignarles direcciones IP de manera dinámica, registrando y gestionando los arrendamientos (leases) de estas direcciones. El servidor funciona con sockets UDP y responde a múltiples clientes de forma concurrente usando hilos (threads).

Las principales funcionalidades del servidor DHCP incluyen:
- *Escuchar solicitudes DHCPDISCOVER* en la red local o remota.
- *Responder con un DHCPOFFER*, ofreciendo una dirección IP disponible.
- *Procesar mensajes DHCPREQUEST* del cliente, confirmando la asignación de la dirección IP.
- *Registrar los arrendamientos de direcciones IP*, incluyendo la dirección IP asignada, la duración del arrendamiento y el estado (libre, reservada, ocupada).
- *Renovar arrendamientos* cuando el cliente lo solicita.
- *Liberar direcciones IP* cuando los clientes finalizan su uso o el arrendamiento expira.

#### Tipos de Sockets
El servidor utiliza un socket de tipo *SOCK_DGRAM*, que corresponde a sockets UDP (protocolo de datagramas). Esto es adecuado para el protocolo DHCP, que no requiere una conexión establecida como en el protocolo TCP, sino que funciona con mensajes individuales (datagramas).

#### Estructura de Datos y Componentes
El proyecto utiliza varias estructuras para gestionar las conexiones y los arrendamientos:
- *struct lease_entry*: Esta estructura almacena las direcciones IP asignadas, las direcciones MAC de los clientes, el tiempo de inicio del arrendamiento, la duración del mismo y su estado.
- *struct dhcp_message*: Esta estructura define el formato de los mensajes DHCP enviados y recibidos, como DHCPDISCOVER, DHCPOFFER, DHCPREQUEST, y DHCPACK.
  
Además, se implementan varias funciones para manejar los diferentes tipos de mensajes DHCP, configurar direcciones IP, y gestionar arrendamientos. El servidor utiliza *hilos* para gestionar las conexiones de forma concurrente, permitiendo que múltiples clientes puedan conectarse y solicitar direcciones IP simultáneamente.

#### Configuración de Subredes y Adaptadores
- El servidor se asocia a una interfaz de red específica, definida en el archivo de configuración constants.h como SERVER_ASSOCIATED_INTERFACE.
- La asignación de direcciones IP se gestiona mediante un rango definido en el archivo de configuración (START_IP, MAX_LEASES), lo que asegura que el servidor pueda proporcionar un número determinado de direcciones IP dentro de una subred específica.
- La máscara de subred, puerta de enlace y servidores DNS se configuran en las opciones DHCP enviadas a los clientes.

#### Dependencias
- Librerías para la manipulación de sockets (sys/socket.h, arpa/inet.h).
- Estructuras de red (sockaddr_in, in_addr).
- Manejo de memoria y concurrencia (pthread.h para hilos).
- Conversión de direcciones IP (inet_aton, inet_ntoa).

## Estructura de Archivos del Proyecto

A continuación se describe el propósito y el contenido de cada archivo en este proyecto:

### 1. *main.c*
   - *Propósito*: Archivo principal del servidor DHCP. Aquí se inicializa el socket, se gestionan las solicitudes de los clientes y se manejan los hilos para procesar cada solicitud.
   - *Explicación*: El servidor espera mensajes DHCP de los clientes y, al recibir uno, crea un hilo que se encargará de procesar la solicitud y asignar una dirección IP si es necesario. Este archivo es el punto de entrada del servidor.

### 2. *dhcp.h y dhcp.c*
   - *Propósito*: Definir y manejar las operaciones específicas del protocolo DHCP, como el procesamiento de los mensajes DHCPDISCOVER, DHCPOFFER, DHCPREQUEST, y DHCPACK.
   - *Explicación*: Estas funciones procesan las solicitudes de los clientes, gestionan la tabla de arrendamientos, y configuran los mensajes DHCP que el servidor envía a los clientes.

### 3. *socket.h y socket.c*
   - *Propósito*: Gestionar la creación, configuración y operación del socket utilizado por el servidor.
   - *Explicación*: Estas funciones permiten la inicialización del socket, la recepción de mensajes de los clientes y el envío de respuestas. Además, configuran el socket para trabajar con la interfaz de red adecuada y habilitan el modo broadcast para la comunicación DHCP.

### 4. *structs.h*
   - *Propósito*: Definir las estructuras necesarias para gestionar los arrendamientos y los mensajes DHCP.
   - *Explicación*: Aquí se definen estructuras como lease_entry (para almacenar información sobre las IPs arrendadas) y dhcp_message (para gestionar los mensajes enviados entre el cliente y el servidor).

### 5. *constants.h*
   - *Propósito*: Definir las constantes que se usan en el proyecto, como los puertos de servidor y cliente, el tamaño del buffer, y el rango de direcciones IP a ser asignadas.
   - *Explicación*: Este archivo contiene valores predefinidos que se utilizan en el servidor, como la IP inicial para el pool de direcciones, la duración del arrendamiento, y la dirección del servidor DHCP.

### 6. *error.h y error.c*
   - *Propósito*: Manejar errores y proporcionar mensajes informativos cuando ocurre un problema en la ejecución del servidor.
   - *Explicación*: La función error imprime el mensaje de error y termina la ejecución del programa cuando algo falla, como la creación de un socket o el envío de un mensaje.

### 7. *utils.h y utils.c*
   - *Propósito*: Proporcionar funciones utilitarias para la conversión de direcciones IP de cadena de texto a formato binario y viceversa.
   - *Explicación*: Estas funciones (ip_to_int e int_to_ip) se utilizan para manejar las direcciones IP en el servidor de manera eficiente.


### Logrados
- Comunicación funcional entre cliente y servidor usando sockets UDP.
- Asignación dinámica de direcciones IP a los clientes.
- Implementación de arrendamientos con tiempo limitado, permitiendo la renovación o liberación de IPs.
- Gestión de múltiples solicitudes simultáneas mediante hilos.
- Manejo de errores básicos (sin direcciones IP disponibles, cliente desconectado, etc.).

### No Logrados
- No se ha implementado un mecanismo de seguridad para la autenticación de clientes, lo que podría ser un área de mejora para proteger la red.
- No se implementó la gestión avanzada de logs ni la interacción con bases de datos para almacenar arrendamientos históricos, lo que podría mejorar el seguimiento y análisis de los arrendamientos.

## Conclusiones

Este proyecto permite la implementación básica de un servidor DHCP capaz de gestionar solicitudes de clientes y asignar direcciones IP de manera dinámica. Mediante el uso de sockets UDP y hilos, se consigue una aplicación concurrente que puede manejar múltiples conexiones simultáneamente.

## Referencias
- https://beej.us/guide/bgnet/
- https://beej.us/guide/bgc/
- https://www.geeksforgeeks.org/tcp-server-client-implementation-in-c/
- https://datatracker.ietf.org/doc/html/rfc2131
- https://datatracker.ietf.org/doc/rfc3456/
