# Cloud90: CTR Nitro-Fueled

Módulo local para Cloud90 Game Link 1.2.2. Se basa en OCTR Windows **v0.6.4**
(commit `ac73258dfee67263657b0722faf2262d40bea826`). No contiene archivos del juego.

## Compatibilidad e instalación

Requiere el portable de OCTR y una copia propia de CUSA13795 1.21 con esta
imagen cargable de `eboot.bin` (no el hash del contenedor SELF):

`9952a20911931dcd0a54fe19135117d96934d17c43b7ced103a4aa7fda783c31`

Es la imagen comprobada en el equipo de desarrollo. Otras imágenes, incluso
la referencia pública 5.05 con hash `2f2115…`, se rechazan hasta validarlas.
No basta con que el nombre de la carpeta o `param.sfo` diga 1.21.

1. Cierra CTR. Compila este repositorio en MSYS2 CLANG64 con `bash build.sh`,
   siguiendo los requisitos de Windows del proyecto original.
2. Ejecuta `python tools/package_cloud90.py`.
3. Extrae `dist/CTRCloud90.zip` y ejecuta:
   `powershell -ExecutionPolicy Bypass -File install-cloud90.ps1 -PortablePath "D:\ruta\al\portable"`.
4. En Cloud90 Game Link, pestaña CTR Nitro-Fueled, selecciona ese portable,
   pega el token de CTR y abre el cliente desde el conector. Se abre el mismo
   `Crash™ Team Racing Nitro-Fueled.exe`; pulsa Jugar en ese cliente. Sus ajustes
   de idioma, pantalla y controles se aplican como en el arranque habitual.
5. Juega Arcade local, carrera normal, un jugador contra CPU. Configura y
   activa las reglas en Cloud90; los tres ejemplos iniciales empiezan apagados.

Se instalan exclusivamente `bin/sacredfire-cloud90.exe`, `bin/cloud90-ctr.json`
y `cloud90_run.py`. Se conservan el runtime original, la configuración, los
datos del juego y los guardados. El instalador valida las sumas y revierte una
instalación incompleta. Rechaza sobrescribir un módulo que el usuario editó.
La desinstalación del conector borra únicamente esos tres archivos validados.
Las DLL del portable v0.6.4 deben estar presentes en `bin`.
El manifiesto actualizado incluye `launcherBridgeVersion=1`. El ejecutable del
cliente, `run.py`, `launcher.ini` y `bbport.ini` no se sustituyen ni se editan.
El modo offline se fuerza solo en el entorno del proceso iniciado por Cloud90.

## Efectos

| Acción | Comportamiento |
|---|---|
| Wumpa | Sumar 1/3/5, llenar a 10, quitar 3 o vaciar; ajuste nativo con actualización del HUD |
| Dar objeto | Misil, bomba, TNT, vaso, máscara, turbo, escudo, reloj u orbe; rechaza inventario ocupado y ruleta activa |
| Quitar objeto | Retira el objeto guardado mediante el ciclo nativo de componentes |
| Turbo inmediato | Añade el boost nativo normal o potenciado según Wumpa; conserva el objeto guardado; el juego puede cancelarlo al chocar |
| Dirección invertida | Máximo 10 segundos |
| Sin acelerar | Máximo 3 segundos, botón y gatillo analógico |
| Sin usar objetos | Máximo 10 segundos, controles estándar del juego |
| Escudo Cloud90 | Bloquea efectos troll hasta 30 segundos; distinto del objeto escudo del kart |
| Ruleta | El backend elige un efecto permitido; nunca llega una orden `roulette` al runtime |

No se anuncian reservas ni fuego azul: necesitan una validación independiente.
No se habilitan aventura, contrarreloj, batalla, multijugador local ni online.
`BB_ONLINE=0` se fuerza al arrancar. Los efectos de controles se aplican a P1.
F9 y «Pausar efectos» detienen los efectos. El escudo no convierte al kart en
invulnerable frente a las armas normales del juego.

## Transporte y límites

Se mantiene el protocolo WebSocket de Cloud90 (`hello`, `welcome`, `state`,
`effect`, `ack`). El conector escribe un JSON de hasta 20 órdenes, sesión
aleatoria por arranque, expiración de 10 segundos y heartbeat monotónico.
El runtime rechaza JSON inválido, sesión distinta, órdenes repetidas o vencidas,
y heartbeat con más de 2 segundos. Al pausar, cambiar de carrera o perder el
heartbeat se limpian los efectos temporales. No se acumulan regalos durante
una desconexión. Cada `ack` comunica el resultado real de la aplicación.
El cliente captura su propia consola. Para conservarlo, el runtime duplica las
líneas Cloud90 a `native-output.log` en la carpeta de la sesión del conector;
`cloud90_run.py --launcher` las reenvía al conector. Cada arranque usa una sesión
nueva; no se reutilizan registros anteriores. Si dejan de llegar estados durante
tres segundos, se anuncia `state 0`, incluso si el cliente sigue abierto.

El hook de `CClock` ejecuta las llamadas nativas en el hilo de actualización,
después de verificar hash y firma. No se escribe RAM desde un proceso externo.
Las direcciones siguientes son relativas a la imagen verificada:

| Ruta nativa | Dirección |
|---|---|
| CClock::Update y su slot virtual | `0xa31a10`, `0x254ca68` |
| Ajuste Wumpa de CRacer | `0xff4a00` |
| Búsqueda de componentes | `0x3dbf90` |
| Entrega de objeto | `0xe366b0` |
| Retirada de componente | `0xbaf1a0` |
| Añadir boost de CKartBoost | `0xe2dd10` |

Los punteros se consultan de nuevo en cada frame; se validan regiones de
memoria y tipos de componentes. Los handles pendientes no se resuelven por
la fuerza. El turbo usa los mismos recursos y liberación de handles que su
componente nativo, sin modificar los recursos compartidos del juego.

## Documentación utilizada

- [OCTR v0.6.4](https://github.com/Ryu7w7/OCTR-NitroFueled-PC/tree/v0.6.4): runtime, carga de imagen y controles.
- [Editor de pistas y herramientas de metadatos](https://github.com/Boooing/CTR-Nitro-Fueled-Track-Editor): clases y recursos del juego.
- [Activision: Power-Up Planning](https://blog.activision.com/crash-bandicoot/2019-06/Crash-Team-Racing-Nitro-Fueled-Power-Up-Planning): mecánicas de Wumpa y armas; no es documentación de una API.

La documentación pública no ofrece una API de regalos. Las llamadas y los
campos de este puente se investigaron y comprobaron con la copia local; los
archivos extraídos y los binarios del juego no se incorporan al repositorio.

## Verificación reproducible

En MSYS2 CLANG64:

```sh
mkdir -p out
clang -std=c11 -Wall -Wextra -Werror -I. -Isrc tests/test_cloud90_runtime.c -o out/cloud90-runtime-test.exe
out/cloud90-runtime-test.exe
clang -std=c11 -Wall -Wextra -Werror tests/test_cloud90_json.c -o out/cloud90-json-test.exe
out/cloud90-json-test.exe
python tests/test_cloud90_launcher.py
```

Estos tests no necesitan el juego. Comprueban regiones de memoria, parser,
limpieza de controles, gatillo analógico y el arranque con Python aislado.
El repositorio del conector comprueba el WebSocket, el JSON y las respuestas.
Las pruebas con el juego requieren una copia propia. Antes de publicar una
versión estable deben verificarse dos pistas, pausa, daño/respawn y transición
entre carreras. La variante probada no encuentra las firmas del parche 60 FPS
de OCTR; no se afirma que dicho parche esté validado.

## Licencia y distribución

Este módulo y el runtime conservan GPL-2.0-or-later. Cada paquete debe acompañarse
de su fuente correspondiente y referencias a los submódulos fijados, disponible
en este repositorio. No distribuir el dump, los guardados ni datos extraídos.
