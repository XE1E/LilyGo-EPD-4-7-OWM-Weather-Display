# Manual de Funciones para Radioaficionado

Este documento describe las pantallas y funciones de referencia para radioaficionados implementadas en el display e-paper LilyGo T5 4.7".

## Acceso

Desde la pantalla principal:
1. Toca zona superior derecha → **Info**
2. Toca botón **RADIO** (esquina inferior derecha)
3. Se muestra el menú con 6 opciones

## Pantallas Disponibles

### 1. Codigo Fonetico

Alfabeto fonético NATO/ICAO para deletreo en comunicaciones de radio.

| A - Alfa | B - Bravo | C - Charlie | D - Delta |
|----------|-----------|-------------|-----------|
| E - Echo | F - Foxtrot | G - Golf | H - Hotel |
| I - India | J - Juliet | K - Kilo | L - Lima |
| M - Mike | N - November | O - Oscar | P - Papa |
| Q - Quebec | R - Romeo | S - Sierra | T - Tango |
| U - Uniform | V - Victor | W - Whiskey | X - X-ray |
| Y - Yankee | Z - Zulu | | |

**Numeros:**
0-Zero, 1-One, 2-Two, 3-Three, 4-Four, 5-Five, 6-Six, 7-Seven, 8-Eight, 9-Niner

### 2. Codigo Q

Codigos Q mas utilizados en comunicaciones de radioaficionado.

| Codigo | Significado |
|--------|-------------|
| QRA | Nombre de estacion |
| QRG | Frecuencia exacta |
| QRL | Frecuencia ocupada? |
| QRM | Interferencia artificial |
| QRN | Interferencia atmosferica |
| QRO | Aumentar potencia |
| QRP | Reducir potencia |
| QRS | Transmitir mas lento |
| QRT | Dejar de transmitir |
| QRV | Estoy listo |
| QRZ | Quien me llama? |
| QSB | Desvanecimiento (fading) |
| QSL | Confirmo recepcion |
| QSO | Comunicado |
| QSY | Cambiar frecuencia |
| QTH | Ubicacion |

### 3. Clave Morse

Representacion grafica del codigo Morse con puntos y rayas.

**Letras:** A-Z con representacion visual de cada caracter.

**Numeros:** 0-9 con representacion visual.

**SOS:** Senal de emergencia en linea separada.

**Prosigns:**
- AR (fin de mensaje)
- SK (fin de QSO)
- BT (pausa)
- KN (adelante solo tu)

### 4. Prefijos DXCC

Tres paginas con prefijos de paises organizados por region geografica.

**Pagina 1 - Americas:**
CE, CM/CO, CP, CX, HC, HI, HK, HP, HR, K/W/N, KH6, KL7, KP4, LU, OA, PY, TG, TI, VE, XE, YN, YS, YV, ZP, 8P...

**Pagina 2 - Europa:**
CT, DL, EA, EA6, EA8, EI, F, G, GD, GI, GM, GW, HA, HB, I, LA, LX, LZ, OE, OH, OK, ON, OZ, PA, S5, SM, SP, SV, UA, UR, YO, YU, 9A...

**Pagina 3 - Asia/Africa/Oceania:**
A4, A6, A7, BV, BY, DU, HL, HS, JA, VK, VR, YB, ZL, 4S, 4X, 5B, 9K, 9M2, 9V, UA0, CN, EA9, SU, ZS, 5H, 5Z, 7X, 9J, V5...

**Informacion adicional (Pagina 3):**
- Regiones ITU: 1=Europa/Africa, 2=Americas, 3=Asia/Pacifico
- Zonas CQ: 1-40 (Mexico=6, USA=3-5, Europa=14-16, Japon=25)

### 5. Propagacion HF (Tiempo Real)

Obtiene datos en tiempo real de HamQSL (www.hamqsl.com).

**Indices Solares:**
- **SFI (Solar Flux):** 70-300 | Bajo <100 | Moderado 100-150 | Bueno >150
- **Indice A:** 0-400 | Tranquilo <10 | Inestable 10-50 | Tormentoso >50
- **Indice K:** 0-9 | Tranquilo 0-2 | Activo 3-4 | Tormentoso >5
- **Manchas Solares:** Numero actual
- **X-Ray:** Clasificacion de actividad
- **Campo Geomagnetico:** Estado traducido (TRANQUILO, ACTIVO, TORMENTA)

**Condiciones de Banda (Dia/Noche):**
- 80m-40m, 30m-20m, 17m-15m, 12m-10m
- Estados: BUENA, REGULAR, MALA

**VHF:** Aurora y E-Skip cuando disponible.

**Sin Conexion:** Si no hay WiFi, muestra informacion de referencia estatica.

### 6. Concursos Principales

| Concurso | Fecha | Modo | Duracion |
|----------|-------|------|----------|
| CQ WW DX CW | Nov (ultimo fin) | CW | 48h |
| CQ WW DX SSB | Oct (ultimo fin) | SSB | 48h |
| CQ WPX CW | Mayo (ultimo fin) | CW | 48h |
| CQ WPX SSB | Marzo (ultimo fin) | SSB | 48h |
| ARRL DX CW | Feb (3er fin) | CW | 48h |
| ARRL DX SSB | Mar (1er fin) | SSB | 48h |
| ARRL Field Day | Jun (4to fin) | Todos | 24h |
| IARU HF World | Jul (2do fin) | CW/SSB | 24h |
| WAE DX CW | Ago (2do fin) | CW | 48h |
| WAE DX SSB | Sep (2do fin) | SSB | 48h |
| JIDX CW | Abr (2do fin) | CW | 48h |
| All Asian DX | Jun/Sep | CW/SSB | 48h |

**Nota:** Horarios en UTC.

## Navegacion

- **Menu:** 6 botones con bordes redondeados (2 filas x 3 columnas)
- **Toca cualquier boton** para ver detalles
- **Toca la pantalla** para regresar al menu de radio
- **DXCC:** Toca para avanzar entre las 3 paginas
- **Desde el menu:** Toca fuera de los botones para regresar a Info

## Notas Tecnicas

Este modulo esta disenado para ser reutilizable en otros proyectos:
- Archivo principal: `radio_screens.h`
- Independiente del resto del codigo
- Solo requiere funciones de dibujo del EPD47
- Propagacion usa HTTPS con WiFiClientSecure

## Futuras Mejoras

- [x] Propagacion en tiempo real (API HamQSL)
- [ ] Navegacion con boton fisico BOOT (sin touch)
- [ ] Mas prefijos DXCC
- [ ] Calendario de concursos actualizado
