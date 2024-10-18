Here’s the updated `README.md` with instructions on how to use the DHCP server and client, added at the end of the document:

# DHCP Server y Cliente Implementación

## Introducción
Este proyecto implementa un servidor y un cliente DHCP utilizando el lenguaje de programación C. El servidor es capaz de escuchar solicitudes de dirección IP de clientes en una red local o remota y asignar direcciones IP de manera dinámica. El cliente, por su parte, envía solicitudes de dirección IP y maneja la renovación y liberación de la dirección IP asignada.

![UML](https://github.com/user-attachments/assets/309ca7ce-9819-4915-9dab-7ad29ad0624a)

## Desarrollo
### Tecnologías Utilizadas
- Lenguaje: C
- Bibliotecas: 
  - `<stdio.h>`, `<stdlib.h>`, `<string.h>`, `<unistd.h>`, `<arpa/inet.h>`, `<sys/socket.h>`, `<pthread.h>`, `<time.h>`
- Sistema Operativo: Ubuntu Linux
- API: Berkeley Sockets para comunicación de red

### Estructura del Proyecto
- `server.c`: Implementación del servidor DHCP.
- `client.c`: Implementación del cliente DHCP.
- `dhcp_server.log`: Archivo de registro para las asignaciones de IP.

### Funcionalidades Implementadas
#### Servidor DHCP
- Escucha solicitudes DHCP (DHCPDISCOVER, DHCPREQUEST).
- Asigna direcciones IP disponibles a los clientes.
- Maneja un rango de direcciones IP definidas por el usuario.
- Gestiona las concesiones (leases) de direcciones IP, incluyendo renovación y liberación.
- Maneja múltiples solicitudes simultáneamente utilizando hilos.
- Registra todas las asignaciones de direcciones IP y el tiempo de arrendamiento en un archivo de registro.
- Soporta las fases principales del proceso DHCP: DISCOVER, OFFER, REQUEST, ACK.

#### Cliente DHCP
- Envía solicitudes DHCPDISCOVER al servidor.
- Recibe y muestra la dirección IP asignada, máscara de red, puerta de enlace predeterminada y servidor DNS.
- Solicita renovación de la dirección IP cuando es necesario.
- Libera la dirección IP al finalizar la ejecución.

## Aspectos Logrados y No logrados
### Aspectos Logrados
- El servidor puede asignar direcciones IP de manera dinámica y registrar estas asignaciones.
- El cliente puede solicitar una dirección IP y manejar la renovación.
- Se logró el manejo concurrente de solicitudes mediante hilos.
- Manejo de errores más sofisticado para condiciones especiales, como la falta de direcciones IP disponibles.

### Aspectos No logrados
- Implementación de un DHCP relay para manejar clientes en diferentes subredes.

## Conclusiones
Este proyecto demuestra la capacidad de implementar un servicio básico de asignación de IP utilizando el protocolo DHCP. Aunque se lograron varias funcionalidades clave, hay áreas para mejoras futuras, como la integración de un DHCP relay y un manejo de errores más robusto. Esta implementación puede servir como base para un sistema más complejo y completo en el futuro.

## Instrucciones de Uso

### Compilación
1. Abre una terminal en tu sistema Ubuntu.
2. Navega al directorio donde se encuentran `server.c` y `client.c`.
3. Compila el servidor y el cliente utilizando el siguiente comando:

   ```bash
   gcc server.c -o dhcp_server -lpthread
   gcc client.c -o dhcp_client
   ```

### Ejecución del Servidor
1. Ejecuta el servidor DHCP en la terminal:

   ```bash
   sudo ./dhcp_server
   ```

2. Asegúrate de que el servidor esté escuchando las solicitudes en el puerto adecuado (usualmente el puerto 67).

### Ejecución del Cliente
1. En otra terminal, ejecuta el cliente DHCP:

   ```bash
   ./dhcp_client
   ```

2. El cliente enviará una solicitud DHCPDISCOVER y mostrará la dirección IP asignada, máscara de red, puerta de enlace predeterminada y servidor DNS en la consola.

3. Para liberar la dirección IP al finalizar, simplemente cierra el cliente o usa el comando de salida apropiado que hayas implementado.

## Referencias
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- [Beej's Guide to Socket Programming](https://beej.us/guide/bgc/)
- [TCP Server-Client Implementation in C - GeeksforGeeks](https://www.geeksforgeeks.org/tcp-server-client-implementation-in-c/)
- [RFC 2131 - Dynamic Host Configuration Protocol](https://datatracker.ietf.org/doc/html/rfc2131)
- [RFC 3456 - DHCP Options](https://datatracker.ietf.org/doc/rfc3456/)
- [Writing a Simple DHCP Server for Linux - LinuxQuestions](https://www.linuxquestions.org/questions/programming-9/writing-a-simple-dhcp-server-for-linux-4175439035/)
- [Socket Programming in C - GeeksforGeeks](https://www.geeksforgeeks.org/socket-programming-cc/)
- [DHCP Client Implementation - GitHub](https://github.com/ejt0062/dhcpserver-c/blob/master/DHCPclient.c)

---

# DHCP Server and Client Implementation

## Introduction
This project implements a DHCP server and client using the C programming language. The server is capable of listening to IP address requests from clients on a local or remote network and dynamically assigning IP addresses. The client, on the other hand, sends IP address requests and manages the renewal and release of the assigned IP address.

## Development
### Technologies Used
- Language: C
- Libraries: 
  - `<stdio.h>`, `<stdlib.h>`, `<string.h>`, `<unistd.h>`, `<arpa/inet.h>`, `<sys/socket.h>`, `<pthread.h>`, `<time.h>`
- Operating System: Ubuntu Linux
- API: Berkeley Sockets for network communication

### Project Structure
- `server.c`: Implementation of the DHCP server.
- `client.c`: Implementation of the DHCP client.
- `dhcp_server.log`: Log file for IP assignments.

### Implemented Features
#### DHCP Server
- Listens for DHCP requests (DHCPDISCOVER, DHCPREQUEST).
- Dynamically assigns available IP addresses to clients.
- Manages a range of IP addresses defined by the user.
- Handles leases of IP addresses, including renewal and release.
- Supports multiple simultaneous requests using threads.
- Logs all IP assignments and lease times in a log file.
- Supports the main phases of the DHCP process: DISCOVER, OFFER, REQUEST, ACK.

#### DHCP Client
- Sends DHCPDISCOVER requests to the server.
- Receives and displays the assigned IP address, subnet mask, default gateway, and DNS server.
- Requests IP renewal when necessary.
- Releases the IP address upon termination.

## Achievements and Limitations
### Achievements
- The server can dynamically assign IP addresses and log these assignments.
- The client can request an IP address and handle renewal.
- Achieved concurrent handling of requests using threads.
- More sophisticated error handling for special conditions, such as lack of available IP addresses.

### Limitations
- Implementation of a DHCP relay to handle clients on different subnets.

## Conclusions
This project demonstrates the capability to implement a basic IP allocation service using the DHCP protocol. While several key functionalities were achieved, there are areas for future improvements, such as integrating a DHCP relay and more robust error handling. This implementation can serve as a foundation for a more complex and complete system in the future.

## Usage Instructions

### Compilation
1. Open a terminal on your Ubuntu system.
2. Navigate to the directory where `server.c` and `client.c` are located.
3. Compile the server and client using the following command:

   ```bash
   gcc server.c -o dhcp_server -lpthread
   gcc client.c -o dhcp_client
   ```

### Running the Server
1. Run the DHCP server in the terminal:

   ```bash
   sudo ./dhcp_server
   ```

2. Ensure that the server is listening for requests on the appropriate port (usually port 67).

### Running the Client
1. In another terminal, run the DHCP client:

   ```bash
   ./dhcp_client
   ```

2. The client will send a DHCPDISCOVER request and display the assigned IP address, subnet mask, default gateway, and DNS server on the console.

3. To release the IP address upon termination, simply close the client or use the appropriate exit command you implemented.

## References
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- [Beej's Guide to Socket Programming](https://beej.us/guide/bgc/)
- [TCP Server-Client Implementation in C - GeeksforGeeks](https://www.geeksforgeeks.org/tcp-server-client-implementation-in-c/)
- [RFC 2131 - Dynamic Host Configuration Protocol](https://datatracker.ietf.org/doc/html/rfc2131)
- [RFC 3456 - DHCP Options](https://datatracker.ietf.org/doc/rfc3456/)
- [Writing a Simple DHCP Server for Linux - LinuxQuestions](https://www.linuxquestions.org/questions/programming-9/writing-a-simple-dhcp-server-for-linux-4175439035/)
- [Socket Programming in C - GeeksforGeeks](https://www.geeksforgeeks.org/socket-programming-cc/)
- [DHCP Client Implementation - GitHub](https://github.com/ejt0062/dhcpserver-c/blob/master/DHCPclient.c)
