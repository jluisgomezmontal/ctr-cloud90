# Evidencia de desarrollo Cloud90 — 2026-10-25

Windows, OCTR v0.6.4, imagen `9952a20911931dcd0a54fe19135117d96934d17c43b7ced103a4aa7fda783c31`.
Datos y guardados de prueba en una carpeta aislada. No se incluyen dumps ni capturas de memoria.

| Prueba | Resultado observado |
|---|---|
| Compilar runtime completo | Correcto con MSYS2 CLANG64 |
| Sumar Wumpa | +3 confirmado por CRacer y HUD |
| Llenar/vaciar | Aplicación nativa confirmada |
| Dar nueve objetos | Confirmación nativa en los nueve casos |
| Quitar objeto | Inventario vacío y confirmación nativa |
| Inventario ocupado | Rechazo correcto; no sustituye el misil |
| Turbo directo | Boost activo, valor hasta 540, terminación natural y misil conservado |
| Inicio de carrera | Turbo rechazado por el juego; no se informa éxito falso |
| Conector real + servidor WS local | Hello 1.2.0/ctrnf, Wumpa, turbo y misil con respuestas del juego |
| Desconexión/reconexión WS | Reconecta; no repite las órdenes anteriores |
| Escudo 10 s | Bloquea Wumpa y dirección invertida, incluido un intento unos 8 s después |
| Pausa y reanudación | Nuevas órdenes rechazadas en pausa; temporizadores limpiados |
| Tres efectos de controles | Aceptados en carrera; transformación de botones y gatillo validada por tests C |
| Parser y acceso a memoria | Tests C pasan, incluido campo en otra región comprometida |
| Heartbeat y ruta Unicode | Lectura válida, fallo transitorio tolerado y caducidad comprobados por test C |
| Python embebido/aislado | Wrapper probado en el portable real y en un test aislado |
| Instalador | Arranque bloqueado con juego abierto; reinstalación, guardados y ediciones del usuario protegidos |

Las pruebas del servidor usan eventos sintéticos: no se envió un regalo real
desde una transmisión. La aceptación de un efecto de controles no sustituye
una prueba humana completa de todas las distribuciones de botones. Antes de
una publicación estable falta completar dos pistas, daño/respawn, transición
de carrera y rechazo de pantalla dividida. El parche 60 FPS de OCTR no encontró
sus firmas en esta imagen. Reservas y fuego azul no forman parte del catálogo.
