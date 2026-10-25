# Plan: Crash Team Racing Nitro-Fueled en Cloud90 Game Link

Fecha: 2026-10-25.
Estado: implementación en ramas independientes de runtime, conector, backend y frontend. Pruebas locales y pruebas dentro de CTR realizadas; publicación y merge pendientes.

Actualización 2026-10-25: investigación en internet incorporada. **Wumpa, turbo y entrega de objetos/armas pasan a ser objetivos principales de la primera entrega**, sujetos a las pruebas de viabilidad de la fase 0B. Las reservas y el fuego azul se investigan en paralelo, con validación propia.

## Avance de implementación — 2026-10-25

- Implementados `ctrnf`, token propio, catálogo, reglas de regalos, compatibilidad con Game Link 1.2.0 y pestaña del conector. Se conservan las versiones anteriores para los otros juegos. Las reglas de ejemplo CTR empiezan apagadas.
- Runtime nativo compilado desde OCTR v0.6.4, instalación junto al original, wrapper con comprobación de imagen, JSON con sesión, heartbeat, expiración y confirmaciones reales.
- Imagen **realmente comprobada**: `9952a20911931dcd0a54fe19135117d96934d17c43b7ced103a4aa7fda783c31`. La referencia pública `2f2115…` no se habilita por suposición. El hash corresponde a la imagen cargable, no al contenedor SELF.
- Juego real: +3 Wumpa visible en HUD; entrega de misil, bomba, TNT, vaso, máscara, escudo, reloj, orbe y turbo; retirada nativa de objetos. Wumpa lleno y vacío también comprobados.
- Prueba de conector completo contra servidor WebSocket local: `hello ctrnf`, Wumpa y misil confirmados por el runtime; reconexión comprobada. Un segundo misil con inventario ocupado se rechaza correctamente.
- Turbo directo: boost nativo activo observado en memoria, velocidad de boost hasta 540 y terminación natural; conserva el misil guardado. Al inicio de la carrera el juego puede rechazarlo y la respuesta informa el rechazo.
- Escudo: rechaza retirar Wumpa y dirección invertida durante su duración. Pausa: rechaza nuevas acciones y limpia temporizadores; reanudar funciona. Los tres efectos de controles fueron aceptados en carrera; máscaras de botones, inversión y gatillo analógico comprobados por pruebas del runtime.
- Corregida cancelación prematura de temporizadores causada por carreras entre el heartbeat y la lectura del JSON. Lectura Win32 con Unicode y permiso para reemplazo atómico; tolerancia de hasta 2 s a fallos transitorios de apertura.
- Verificación: 39 tests del backend; 20 del frontend; compilación backend y frontend (webpack); compilación del conector, pruebas Lua y protocolo WebSocket de CTR/Hades-MAME; parser C, memoria/temporizadores/Unicode, wrapper con Python aislado e instalador con preservación de guardados.

Pendiente de validación de publicación: regalo real de una transmisión, matriz completa de dos pistas/daño/respawn/cambio de carrera, pantalla dividida y parche 60 FPS. La variante local no encuentra las firmas del parche 60 FPS de OCTR; no se afirma soporte comprobado. Reservas/fuego azul siguen como investigación y **no se publican como efectos disponibles**. Estos límites se documentan en los PR, sin hacer merge.

## Objetivo

Agregar **Crash Team Racing: Nitro-Fueled** a `cloud90-game-link` para que regalos y los demás eventos de live soportados activen efectos temporales durante carreras, usando las reglas, permisos, token, historial, overlay y voz existentes de Cloud90.

ID propuesto: `ctrnf`. Primera versión: carreras locales contra CPU, un jugador. El soporte de multijugador local se evalúa después; los efectos quedan desactivados en partidas en línea.

## Lo que ya se comprobó

- El portable está en `D:\CTR\CTR-NitroFueled-PC-portable`.
- Contiene `bin\sacredfire.exe`, `run.py`, Python incluido, `user`, `mods`, `patches` y documentación de mods.
- `bin\VERSION` declara `v0.6.4-windows`. Es una etiqueta local: falta identificar el commit y comprobar que el código fuente elegido reproduce este ejecutable y sus funciones.
- El `LEEME.txt` identifica el juego como PS4 USA **CUSA13795, actualización 1.21**, ejecutado mediante un port para Windows con Vulkan. También hay dumps y actualizaciones en otras carpetas de `D:\CTR`; falta validar la combinación efectiva que usa el launcher.
- La documentación local de mods exige reiniciar el juego al cambiar su selección. Admite recursos y parches de ejecutable; no documenta una API de efectos dinámicos por regalos. Esto no impide construirla en el runtime, pero instalar un XML no resuelve la integración en vivo.
- `cloud90-game-link` ya tiene `GameHost`, pestañas por juego, tokens cifrados con DPAPI, conexión WebSocket, pausa de efectos y confirmaciones del juego.
- Hades y MAME usan un buzón Lua. CTR no tiene un intérprete Lua demostrado: no se debe asumir que puede ejecutar ese buzón.
- Backend y frontend ya tienen catálogo y configuración por juego. No hace falta otro servicio de regalos.

Fuente pública del port: [GetLostIn/CTR-NitroFueled-PC](https://github.com/GetLostIn/CTR-NitroFueled-PC). Confirma el enfoque de runtime para Windows y dispone de código fuente. Sus cambios actuales no demuestran compatibilidad con el portable local; esa correspondencia se verifica en fase 0.

La investigación posterior encontró una referencia más cercana: [OCTR v0.6.4 de Ryu7w7](https://github.com/Ryu7w7/OCTR-NitroFueled-PC/releases/tag/v0.6.4), commit `ac73258dfee67263657b0722faf2262d40bea826`. Su versión coincide con la etiqueta local y su documentación incluye el sistema de mods observado. **Es el primer candidato para reproducir el portable**, sin dar por demostrada la identidad de los binarios por el número de versión.

## Arquitectura propuesta

```text
Regalo / evento del live
          |
Backend Cloud90: regla ctrnf, permisos, cooldown y límites
          |
WebSocket existente /game-link/ws
          |
Cloud90GameLink.exe — pestaña CTR Nitro-Fueled
          |
Canal local de mensajes JSON, limitado a la sesión
          |
Runtime CTR modificado: cola -> punto seguro -> efecto
          |
Resultado real -> Game Link -> backend -> overlay y voz
```

**Decisión propuesta:** conservar la conexión a Cloud90 en Game Link y agregar un módulo al runtime del port. Así el usuario mantiene la misma app y el token no necesita entrar al proceso del juego. Mario Galaxy sirve de referencia para la ejecución de efectos y la restauración; su cliente de red no necesita copiarse.

Transporte local preferido: named pipe de Windows con acceso limitado al usuario y una identidad nueva por sesión. Confirmar soporte y coste en fase 0. Si complica la primera prueba, usar archivos JSON con reemplazo atómico y canal de respuestas, aislados por sesión. Elegir uno antes de implementar la fase 1.

El canal acepta solamente comandos declarados; no ejecuta scripts, comandos de consola ni direcciones arbitrarias enviadas por el servidor. Un hilo de comunicación encola solicitudes. Las modificaciones se aplican en los puntos seguros de entrada/simulación del runtime que se identifiquen al revisar su código.

## Fase 0 — Viabilidad y versión reproducible

1. Identificar rutas efectivas del juego preparado, configuración y guardados, sin imprimir tokens ni claves del portable. Registrar hashes del runtime y del ejecutable del juego para la matriz de compatibilidad.
2. Respaldar guardados y configuración. Preparar una instalación de prueba separada; no escribir sobre los dumps ni sus enlaces duros.
3. Obtener el código del port y localizar la revisión que corresponde al portable. Si no se consigue reproducir esa versión, evaluar una migración explícita a una versión compilable y probar sus funciones antes de añadir Cloud90.
4. Compilar el runtime y completar una carrera local sin modificaciones. Verificar controles, audio, guardado, launcher, mods seleccionados y parche de 60 FPS.
5. Localizar lectura de mandos/teclado, menú del port, ciclo de actualización y señales para distinguir carrera, pausa, carga y menú. Identificar cómo detectar y rechazar el modo en línea; si no hay señal fiable, limitar el prototipo a una configuración local verificable.
6. Añadir una prueba local: invertir únicamente la dirección del jugador 1 durante 5 segundos. Restaurar al vencer el tiempo, salir de carrera o pulsar pánico.
7. Registrar commit, compilador, opciones de build y matriz de versiones probadas. Decidir el transporte local.

**Criterio de salida:** carrera completada con runtime compilado, efecto temporal comprobado y restauración consistente. Si esto falla, no publicar CTR como compatible. La investigación documental de la fase 0B puede avanzar en paralelo; las modificaciones en carrera necesitan esta base reproducible.

## Investigación documental — turbo, Wumpa y armas

Consulta realizada el 2026-10-25. Se revisaron fuentes del editor del juego y repositorios de sus autores. No se ejecutaron cheats ni mods descargados. Las observaciones del código son una base de investigación, no pruebas en el portable de `D:\CTR`.

### Fuentes encontradas y utilidad

| Fuente | Qué aporta | Límite |
|---|---|---|
| [Activision: Power-Up Planning](https://blog.activision.com/crash-bandicoot/2019-06/Crash-Team-Racing-Nitro-Fueled-Power-Up-Planning) | Mecánicas de Wumpa, turbo y arsenal; diferencias de modo | No documenta API, IDs internos ni RAM |
| [OCTR v0.6.4](https://github.com/Ryu7w7/OCTR-NitroFueled-PC/releases/tag/v0.6.4) | Código de una versión candidata para este portable | Requiere comparar binarios y build |
| [Nitro Creator Modding Handbook](https://github.com/Ryu7w7/OCTR-NitroFueled-PC/blob/30611310865f0d3e98251d38914a43177df56f47/docs/native-handbook/Nitro-Creator-Handbook.html) | Recursos nativos y lógica de zonas turbo, con capítulos de pruebas | Enfocado en creación de contenido, no regalos en vivo |
| [CTR Track Editor: notas técnicas](https://github.com/Boooing/CTR-Nitro-Fueled-Track-Editor/blob/453537704f3ba7652c3a8190db4b3c9cfc0b2a44/ctr_tools/README.md) | Investigación de CUSA13795 1.21; distingue resultados probados de candidatos | Modifica pistas antes de arrancar |
| [Esquema de clases del editor](https://github.com/Boooing/CTR-Nitro-Fueled-Track-Editor/blob/453537704f3ba7652c3a8190db4b3c9cfc0b2a44/ctr_tools/schema/ctr_meta.json) | Nombres, tipos y offsets relativos de clases extraídas del ejecutable | No localiza instancias vivas, funciones ni ABI de llamadas |
| [powerup.py del editor](https://github.com/Boooing/CTR-Nitro-Fueled-Track-Editor/blob/453537704f3ba7652c3a8190db4b3c9cfc0b2a44/ctr_tools/powerup.py) | Referencias a cajas, fruta, TNT/Nitro y zonas turbo | La colocación en archivos no demuestra creación dinámica |

El editor declara como objetivo la misma edición **CUSA13795, v1.21**. Es una coincidencia útil, pero las estructuras y referencias deben cotejarse con el ejecutable exacto del usuario. Su documentación reporta un turbo de pista probado en juego; marca la importación de TNT/Nitro/fruta como pendiente de validación en juego. No equiparar ambos resultados. [Fuente: notas técnicas del autor](https://github.com/Boooing/CTR-Nitro-Fueled-Track-Editor/blob/453537704f3ba7652c3a8190db4b3c9cfc0b2a44/ctr_tools/README.md).

### Mecánicas que guían el diseño

Con 10 frutas Wumpa los objetos reciben mejoras. El turbo de inventario y las reservas son conceptos distintos. El arsenal documentado incluye bombas, misiles, TNT/Nitro, frascos, escudos, máscaras, reloj y orbe. Super Engine e invisibilidad están descritos para batalla. Estas reglas sirven para diseñar efectos; no ofrecen por sí mismas un método de inyección. [Fuente oficial](https://blog.activision.com/crash-bandicoot/2019-06/Crash-Team-Racing-Nitro-Fueled-Power-Up-Planning).

En este plan, **Wumpa significa fruta de la carrera**. Las Wumpa Coins de la tienda y las recompensas del perfil son otro sistema y no forman parte del efecto. Buscar solamente “Wumpa modifier” también devuelve código de monedas; no reutilizarlo como prueba del contador del kart.

### Pistas técnicas concretas encontradas

Datos leídos del [esquema publicado](https://github.com/Boooing/CTR-Nitro-Fueled-Track-Editor/blob/453537704f3ba7652c3a8190db4b3c9cfc0b2a44/ctr_tools/schema/ctr_meta.json). Offsets en hexadecimal, convertidos de los valores decimales del JSON. **Son relativos al objeto del esquema, no direcciones absolutas ni permisos para escribir RAM.**

| Clase | Campos relevantes | Investigación que permite iniciar |
|---|---|---|
| `CRacerAdjustWumpaFruitsMessage` | `_wumpaFruitCount` +0x40; `_previousWumpaFruitCount` +0x44 | Seguir ajuste de fruta |
| `CWumpaCrateCollectedMessage` | `_amount` +0x38 | Seguir recolección |
| `CVehiclePowerUpControllerComponent` | `_storedPowerUp` +0x30; `_activePowerUp` +0x38; `_isShuffling` +0x40 | Identificar inventario y ruleta |
| Mismo controlador | `_crateCollectedPowerUpType` +0x78 | Rastrear tipo obtenido |
| `CMultiPowerUpComponent` | `_charges` +0x50; `_subType` +0x54; `_stayedJuiced` +0x58 | Examinar cargas múltiples |
| `CPowerUpFactory` / `CPowerUpData` | `_powerUpData` / `_turboBoostData` | Localizar fábrica y recursos |
| `CKartBoostComponent` | `_kartBoosts` +0x28; `_pendingBoosts` +0x30; `_currentBoostSpeed` +0x38 | Seguir boosts activos |
| `CKartBoost` | `_elapsedTime` +0x18; `_boostSustainDuration` +0x28; `_isMaintained` +0x30 | Examinar temporización |

Interpretación propuesta: observar la asignación normal de un objeto y seguir su fábrica/controlador es mejor punto de partida que copiar un handle. Confirmar esto en el ejecutable; el esquema no expone la función de entrega ni los valores numéricos de las armas. Para Wumpa, el mensaje puede ayudar a rastrear la actualización, pero no prueba dónde está el contador autoritativo.

Para turbo, el [código del editor](https://github.com/Boooing/CTR-Nitro-Fueled-Track-Editor/blob/453537704f3ba7652c3a8190db4b3c9cfc0b2a44/ctr_tools/powerup.py) referencia `GPI_SpeedBoostZone006`, `Scripts.Graph.common_TurboPadData` y `vsc/common/race/common_turbopad.igz`. El [manual](https://github.com/Ryu7w7/OCTR-NitroFueled-PC/blob/30611310865f0d3e98251d38914a43177df56f47/docs/native-handbook/Nitro-Creator-Handbook.html) distingue recursos de turbo normal y superturbo. Son puntos para rastrear el boost nativo; no constituyen una función de fuego azul lista para llamar.

**Resultado de la búsqueda:** se encontraron documentación de mecánicas, herramientas y estructuras con las que comenzar. No se encontró en las fuentes revisadas una API de regalos, una tabla completa de IDs de armas ni una tabla de RAM validada para este portable. Los proyectos CTR de PS1 y sus direcciones no son intercambiables con Nitro-Fueled.

## Fase 0B — Pruebas prioritarias de Wumpa, turbo y arsenal

Esta fase precede a dar por terminada la primera entrega. Las cifras siguientes son límites de producto propuestos, no constantes descubiertas en el juego.

1. **Identificar al corredor local P1.** Desde sus componentes, localizar controlador de objetos y boost; validar identidad, vida útil y estado de carrera en cada solicitud. Empezar por observación sin escrituras: registrar recolección de fruta, caja, uso de objeto y paso por turbo. Guardar evidencia por hash del ejecutable.
2. **Wumpa:** seguir `CRacerAdjustWumpaFruitsMessage`/recolección hasta el estado autoritativo. Probar +1, +3, llevar a 10, quitar 3 y vaciar, con límites 0–10 para Cloud90. Verificar HUD y cambio real de potencia al cruzar 9↔10, daño y respawn. Confirmar unidades y semántica del mensaje antes de emitirlo; no suponer que `_wumpaFruitCount` es un delta.
3. **Dar objetos/armas:** observar cajas normales, recuperar el mapa de tipos desde el juego y seguir `CPowerUpFactory` y el controlador. Encontrar la ruta que actualiza handle, icono, ruleta y cargas correctamente. Probar un misil, una bomba y un turbo. No copiar handles entre corredores ni carreras.
4. **Uso y reemplazo:** primero entregar al inventario y dejar que el jugador dispare. Si ya tiene objeto o la ruleta gira, rechazar con motivo concreto en el MVP. Añadir reemplazo, quitar objeto y uso automático solo al demostrar limpieza y consumo correctos. Una pulsación sintética sin inventario verificado no cuenta como “dar arma”.
5. **Turbo inmediato:** seguir la aplicación normal del turbo y probar un pulso nativo para P1. Separar `give_item(turbo)` de `boost_now`; el primero entrega, el segundo activa. Medir efecto en velocidad y duración; no usar solo el color del escape como evidencia.
6. **Reservas:** observar boosts de derrape, salto y pista; identificar cómo se mantienen y consumen. El esquema muestra listas/temporizadores, no un contador confirmado de reservas. Probar sumar una cantidad equivalente a ~2 s y quitar reservas solo después de definir unidades y límites. No traducir segundos a frames suponiendo 60 FPS.
7. **Fuego azul:** comparar zonas normales y superturbo, datos y transición de intensidad. Probar activación temporal únicamente cuando se conozca la ruta nativa y cómo convive con boosts ya ganados. No elevar arbitrariamente `_currentBoostSpeed` ni confundir una llama visual con el boost físico.
8. **Arsenal ampliado:** validar cada opción de la tabla siguiente, HUD, carga, uso y limpieza al cambiar carrera. Separar los objetos de batalla del catálogo de carrera.

### Catálogo de regalos prioritario

IDs públicos propuestos. El frontend usa opciones de texto permitidas y el módulo las traduce a IDs internos verificados. No exponer números arbitrarios, handles o direcciones.

| Efecto | Acción propuesta | Prioridad / requisito |
|---|---|---|
| `wumpa_add` | +1, +3 o +5 frutas; tope 10 | Primera entrega |
| `wumpa_full` | Llevar a 10 frutas | Primera entrega |
| `wumpa_remove` | Quitar 3 o vaciar | Primera entrega; troll |
| `give_item` | Entregar objeto elegido por la regla | Primera entrega: misil, bomba y turbo; ampliar con pruebas |
| `remove_item` | Retirar el objeto guardado | Primera entrega si se verifica liberación; troll |
| `boost_now` | Un pulso del turbo nativo | Primera entrega |
| `reserve_add` | Aumentar reservas con tope | Investigación prioritaria; requiere unidades verificadas |
| `reserve_clear` | Quitar reservas | Investigación prioritaria; troll |
| `blue_fire` | Superturbo temporal | Investigación prioritaria; requiere restauración y convivencia |
| `random_item` | Elegir entre objetos compatibles con el modo | Tras validar `give_item`; sin opciones no probadas |

### Opciones de objetos y armas

Nombres basados en el [arsenal documentado por Activision](https://blog.activision.com/crash-bandicoot/2019-06/Crash-Team-Racing-Nitro-Fueled-Power-Up-Planning); IDs de la primera columna propuestos por Cloud90. La disponibilidad técnica debe comprobarse por modo.

| Opción | Objeto/arma | Prueba específica |
|---|---|---|
| `missile` | Misil perseguidor | Entrega y disparo; después triple carga |
| `bomb` | Bomba de bolos | Entrega, dirección y detonación; después triple carga |
| `tnt` | Caja explosiva | Validar transformación con Wumpa |
| `beaker` | Frasco de N. Brio | Validar variante potenciada |
| `turbo` | Objeto turbo | Inventario y consumo separados del boost inmediato |
| `mask` | Máscara | Resolver variante válida para el personaje |
| `power_shield` | Escudo del juego | Separado de `shield`, el escudo antitroll de Cloud90 |
| `clock` | Reloj de N. Tropy | Validar alcance sobre corredores CPU |
| `warp_orb` | Orbe | Validar trayectoria/objetivo y recursos disponibles |
| `super_engine` | Supermotor | Solo catálogo de batalla hasta demostrar otro soporte |
| `invisibility` | Invisibilidad | Solo catálogo de batalla |

No asumir un ID interno distinto para cada variante potenciada: comprobar si el juego lo decide a partir de la fruta al entregar o usar el objeto. Probar con 0, 9 y 10 Wumpa. No forzar fruta a 10 en secreto para entregar un arma mejorada; esa combinación debe ser una regla explícita con resultado verificable.

### Límites, confirmación y restauración

- Empezar con una entrega de objeto por comando, un pulso de turbo y cooldown de 15 s; reservar el ajuste comercial para después de la prueba jugable.
- Si el inventario está ocupado, responder “ya tienes un objeto”; no acumular armas para la siguiente carrera. Si ya tiene 10 frutas y se pide llenar, responder “Wumpa ya al máximo” sin anunciar un cambio inexistente.
- `ack` de entrega confirma inventario actualizado; no afirma que el misil haya sido disparado ni que haya golpeado. `boost_now` confirma activación, no una victoria ni una duración supuesta.
- Fruta y consumibles son cambios instantáneos: no restaurar una foto vieja del inventario al terminar un temporizador, porque podría recuperar armas gastadas o borrar fruta recogida jugando. Pánico cancela la actividad de Cloud90 y modificaciones temporales; no deshace automáticamente consumos ya ocurridos.
- Para boost temporal, restaurar solo la contribución de Cloud90; respetar los turbos obtenidos durante el efecto. Si no se puede separar la contribución, publicar primero el pulso nativo sin prometer fuego azul “durante X segundos”.
- Todas las acciones siguen limitadas a carreras locales; revalidar modo y corredor justo antes de aplicar.

**Criterio de salida 0B para la primera entrega:** demostrar Wumpa real, entrega/uso de misil y bomba, entrega del objeto turbo y boost inmediato en al menos dos pistas, con P1, 60 FPS, pausa, daño/respawn y transición a otra carrera. Registrar por efecto si pasa, falla o queda bloqueado. Reservas/fuego azul no se anuncian como disponibles hasta pasar sus pruebas específicas.

## Fase 1 — Módulo de efectos del runtime

Archivos propuestos en el fork: módulo de comunicación, cola/temporizadores y transformaciones de entrada; los nombres y puntos de integración se fijan después de leer el código correspondiente.

- Comandos con `id`, efecto y duración acotada, asociados a una sesión. Rechazar mensajes malformados, desconocidos, repetidos, vencidos o de una sesión anterior; establecer límite de tamaño y de cola.
- Usar reloj monotónico para duración. Al pausar la carrera se cancelan los efectos temporales en el MVP; no se dejan pendientes para reanudarse por sorpresa.
- Combinar entradas sobre una copia del estado de cada actualización, sin acumular inversiones ni modificar permanentemente el mapeo del usuario.
- Rechazar efectos en menús, carga, resultados, durante pausa o en modo en línea. En MVP no se conservan regalos para la próxima carrera.
- Pánico accesible desde Game Link y desde una tecla configurable del runtime, evitando conflictos con el juego. Limpia cola y efectos, y mantiene la recepción pausada hasta reactivarla.
- Si Game Link desaparece o se pierde el canal, limpiar efectos con una supervisión de conexión local. Un cierre abrupto no debe dejar una modificación persistente.
- Responder `ack` solo después de aplicar el efecto en el punto correcto; informar el motivo cuando se rechaza. La entrega al canal local no cuenta como aplicación.
- Reportar `inPlay` y pausa al puente; exponer diagnóstico local de versión y estado.

### Catálogo candidato

Los identificadores siguientes son propuestas. Ningún efecto de CTR está verificado todavía.

| Efecto | Método a investigar | Alcance inicial |
|---|---|---|
| `invert_steering` | Invertir eje horizontal de P1, conservando aceleración/freno | MVP; 5 s por defecto, máximo 10 s |
| `no_accelerate` | Bloquear aceleración en teclado y mando de P1 | MVP si la lectura de entrada lo permite; 2 s, máximo 3 s |
| `no_items` | Bloquear uso del objeto en P1 | MVP si se valida; 5 s, máximo 10 s |
| `shield` | Rechazar efectos troll desde el módulo | MVP; alineado con la semántica existente de Cloud90 |
| `roulette` | Elección en backend entre efectos disponibles | Después del MVP; virtual, no se envía literalmente al juego |
| Dar/quitar Wumpa | Seguir mensajes y estado del corredor documentados en fase 0B | Primera entrega, tras prueba real |
| Dar objetos/armas | Fábrica y controlador nativos; opciones validadas de fase 0B | Primera entrega: misil, bomba y turbo |
| Turbo inmediato | Ruta nativa de activación; independiente del inventario | Primera entrega, tras prueba real |
| Reservas/fuego azul | Listas, datos y temporización de boost; unidades por verificar | Investigación prioritaria; publicación tras validación propia |
| Giro/trompo o velocidad temporal | Encontrar mecanismo válido y límites | Investigación posterior |

No prometer objetos, turbo ni Wumpa a partir de direcciones supuestas. Registrar región, versión, firmas, tipo de dato, estado permitido y evidencia de cada candidato. Si no puede verificarse, no entra al catálogo público.

## Fase 2 — Pestaña en Cloud90 Game Link

Cambios previstos en `cloud90-game-link`:

- `src/Ctr.cs` nuevo: detección, compatibilidad, preparación, arranque, seguimiento del proceso real y transporte local.
- `src/MainForm.cs`: `CtrHost : GameHost` y pestaña “CTR Nitro-Fueled”.
- `src/Settings.cs`: ruta configurable `CtrDir`; aprovechar `Tokens["ctrnf"]` y su cifrado actual. `D:\CTR` es la ubicación de esta PC, no una ruta fija del producto.
- `src/Link.cs`: separar la salida local de efectos de su implementación concreta Lua, conservando validaciones, confirmaciones y tiempos límite. Mantener el transporte existente para Hades/MAME y agregar el de CTR. Acordar la interfaz antes de modificarlo.
- `build.ps1` y empaquetado: incluir solo los componentes nuevos y su manifiesto de compatibilidad.

La carpeta elegida debe contener el portable compatible, no únicamente los datos PS4. El arranque debe conservar el flujo necesario de `run.py`/launcher —preparación, mods, ajustes y directorio de trabajo— y seguir el runtime incluso si un proceso intermedio termina o reinicia el juego.

Instalación reversible: validar versión, respaldar el runtime original y registrar qué archivos se agregaron. Desinstalar restaura la copia compatible sin borrar partidas ni sobrescribir una actualización posterior del usuario. Mostrar un error concreto ante una versión no probada.

**Criterio de salida:** desde la pestaña se abre CTR, un servidor de prueba envía un efecto, el juego lo aplica y confirma; pausa, cierre y reconexión funcionan.

## Fase 3 — Backend Cloud90

Rutas principales de trabajo en `backcloud90devstreamkit`:

- `src/game-link/game-link.catalog.ts`: registrar `ctrnf`, efectos realmente probados, duración máxima, flags troll y cooldowns.
- `src/game-link/game-link-config.ts`: reglas iniciales conservadoras, inicialmente desactivadas hasta que el usuario las configure.
- Revisar validación de IDs y versiones, permisos, sesiones y pruebas en `src/game-link/`; incorporar la versión de Game Link y el identificador de compatibilidad del módulo CTR.
- Reutilizar token por juego, autorización, límites, confirmaciones, historial, overlay y voz. Definir el nivel de acceso comercial siguiendo la política vigente de los demás juegos, sin asumir aquí un cambio de plan.
- Mantener como autoridad el resultado del juego: no anunciar como aplicado un efecto rechazado por menú, pausa, versión o desconexión.

Configuración de ejemplo para la prueba: un regalo elegido por el usuario -> `invert_steering`, 5 s, cooldown de 15 s. Activar primero un único efecto para comprobar toda la cadena.

## Fase 4 — Frontend y experiencia de uso

En `frontcloud90devstreamkit`:

- `src/lib/game-link.ts`: juego `ctrnf`, requisitos, pestaña y descarga compatibles.
- Tarjeta de juego y ruta `/juegos/ctrnf`, siguiendo `GameLinkGame` y los componentes existentes.
- Permitir configurar regalos, duración, cooldown, prueba manual, overlay y voz mediante el catálogo del backend.
- Instrucciones: elegir portable compatible, instalar módulo, pegar token CTR, abrir juego y ejecutar una prueba en carrera local.
- Mensajes claros para “conectado”, “esperando carrera”, “efectos pausados” y “versión no compatible”, según el estado que soporte el protocolo.
- Leer la guía local de Next.js requerida por `frontcloud90devstreamkit/AGENTS.md` antes de implementar cambios de frontend.

## Fase 5 — Validación y entrega

1. Probar lógica de cola, límites, IDs repetidos, expiración y pérdida de sesión sin lanzar CTR.
2. Probar Game Link contra un servidor falso y el módulo local; no depender de regalos reales para verificar el protocolo.
3. En CTR real: teclado y mando, entrada/salida de carrera, pausa, reinicio, pánico, desconexión, regalos simultáneos y 60 FPS. Comprobar que solo cambia P1 y que los controles recuperan el estado esperado.
4. Comprobar rechazo en modo en línea y en versiones desconocidas, y restauración al cerrar el puente.
5. Ejecutar las pruebas existentes relevantes de Game Link, backend y frontend, incluyendo Hades y MAME si se modifica su transporte compartido.
6. Completar al menos una sesión local de 30 minutos con efectos y comparar guardados/configuración con el comportamiento esperado.
7. Publicar código y cambios del runtime conforme a su licencia, instalador reversible, notas de compatibilidad y binarios del puente/módulo. No incluir dumps, PKG, BIOS, claves ni recursos del juego.
8. Preparar artefactos y validar compatibilidad antes del despliegue. Orden de activación: backend compatible, release de Game Link/runtime, frontend y acceso de prueba; habilitar el catálogo al terminar la prueba completa con un regalo real.

**Terminado cuando:** regalos reales modifican Wumpa, entregan las armas iniciales y activan turbo durante carreras locales, con confirmaciones correctas al overlay; los efectos temporales terminan correctamente y se puede desinstalar la integración sin alterar la instalación del usuario. Reservas y fuego azul figuran como pendientes si no se han demostrado, sin darlos por implementados.

## Orden recomendado y decisiones pendientes

**Primero fase 0.** La principal incertidumbre es disponer de código reproducible para el runtime local y de un punto seguro para modificar entrada y detectar estado de carrera. La conexión a Cloud90 ya tiene una base reutilizable.

Tras superar esa prueba: fase 0B (Wumpa, armas y turbo) -> módulo local -> pestaña y servidor falso -> backend/frontend -> regalos reales. Estos tres grupos son el núcleo atractivo de la primera entrega; los controles invertidos sirven como prueba inicial de conexión. Reservas y fuego azul se estudian desde 0B y se publican al demostrar su implementación.

Antes de implementar deben quedar anotados el commit del port, build compatible, transporte local elegido, mecanismo de detección de carrera/modo local y efectos demostrados. Este plan no afirma que esas comprobaciones ya se hayan realizado.
