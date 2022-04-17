# SOA-22-TC-2
El siguiente es un programa que simula un puente con tráfico en una dirección al que llegan vehículos por ambos extremos. Se utilizan semáforos y señales para sincronizar su entrada al puente. Los vehículos entran al puente si este se encuentra vacío o si comparte dirección con los que lo están cruzando. Cada vehículo se crea con una espera que sigue una distribución exponencial con media de 3 segundos, luego viaja a la entrada del puente y espera por su turno o entra directamente. Bajo esta simulación cada vehículo dura 3 segundos en cruzar el puente desde que logra entrar y le toma otros 3 segundos terminar su recorrido al salir del puente. 
Al brindar el comando con los debidos parámetros se generan vehículos con direcciones que se alternan aleatoria mente para que sea visible su espera en los cambios de dirección y disminuir la agrupación por direcciones. Esto hace más interesantes los experimentos.

## Prerequisitos
Sistema operativo de Linux, Ubuntu se usó para su desarrollo y pruebas.

## USO
Para compilar, ejecutar los siguientes comandos: 
- `make all`
Este comando compila todos los programas dentro de este proyecto.
Ejecute escribiendo ./main cantidadDesdeElEste cantidadDesdeElOeste dentro de la carpeta build. 
