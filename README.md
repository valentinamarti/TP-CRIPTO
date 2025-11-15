# TP - ESTEGANOGRAFÍA (72.04 Criptografía y Seguridad)

Este proyecto es una implementación en C de un programa de esteganografía (`stegobmp`) capaz de ocultar y extraer archivos dentro de imágenes BMP de 24 bits. Soporta los algoritmos LSB1, LSB4 y LSBI, e incluye una capa de encriptación opcional usando OpenSSL (AES y 3DES).

## 1. Prerrequisitos

Este proyecto está diseñado para compilar y ejecutarse en un entorno Linux (como WSL2 con Ubuntu 22.04).

Necesitarás las siguientes herramientas:
* `gcc` (Compilador de C)
* `make` (Utilidad para automatizar la compilación)
* `openssl` (La librería de criptografía y sus cabeceras de desarrollo)

Para instalar las dependencias en Ubuntu/WSL:
```bash
sudo apt update
sudo apt install build-essential libssl-dev
```
Para instalar las dependencias en MacOs:
```bash
brew install openssl@3
```

## 2. Cómo Compilar el Proyecto

El proyecto incluye un Makefile que automatiza todo el proceso.

En el directorio raíz del proyecto ejecutá:
```bash
make
```

### Opcional: limpiar compilaciones anteriores
```bash
make clean
```

Esto generará un archivo ejecutable llamado stegobmp en el directorio raíz.

## 3. Modo de Uso (Comandos)
El programa se ejecuta desde la línea de comandos y tiene dos modos principales: -embed y -extract.

### Ocultar un archivo (Embed)
```bash
./stegobmp -embed -in <archivo_secreto> -p <portador.bmp> -out <salida.bmp> -steg <ALGORITMO> [OPCIONES_CRYPTO]
```

Ejemplo SIN encriptación:

```bash
./stegobmp -embed -in secreto.txt -p carrier.bmp -out stego.bmp -steg LSB1
```


Ejemplo CON encriptación (AES-256 CBC):
```bash
./stegobmp -embed -in secreto.txt -p carrier.bmp -out stego.bmp -steg LSB1 -a aes256 -m cbc -pass "mi_password_123"
```

### Extraer un archivo (Extract)
```bash
./stegobmp -extract -p <stego.bmp> -out <nombre_base_salida> -steg <ALGORITMO> [OPCIONES_CRYPTO]
```
Importante: Al extraer, el programa automáticamente agregará la extensión original al archivo de salida (ej. .txt, .png) . El parámetro -out solo necesita el nombre base.


Ejemplo SIN encriptación:

```bash
# Esto creará "secreto_extraido"
./stegobmp -extract -p stego.bmp -out secreto_extraido -steg LSB1
```

Ejemplo CON encriptación (AES-256 CBC):

```bash
# Esto creará "secreto_extraido"
./stegobmp -extract -p stego.bmp -out secreto_extraido -steg LSB1 -a aes256 -m cbc -pass "mi_password_123"
```

## Opciones de Criptografía

- -a <aes128|aes192|aes256|3des>: Algoritmo de cifrado.

- -m <ecb|cfb|ofb|cbc>: Modo de operación. 
- -pass <password>: Contraseña para derivar la clave.


Comportamiento por defecto (Defaults) :

- Si solo se provee -pass, se usará aes128 en modo cbc. 
- Si solo se provee -pass y -a, se usará modo cbc. 
- Si solo se provee -pass y -m, se usará aes128.
