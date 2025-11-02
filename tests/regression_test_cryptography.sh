#!/bin/bash

# --- Config ---
EXECUTABLE="../stegobmp"
CARRIER="blue-bmp-24-bit.bmp"
SECRET_FILE="secreto.txt"
STEGO_FILE="stego_test.bmp"
EXTRACTED_FILE="extraido_test"
PASSWORD="mi_password_seguro_123!"

# Algoritmos y Modos a probar
ALGOS="aes128 aes192 aes256 3des"
MODES="ecb cbc cfb ofb"

# Colores para la salida
GREEN="\033[0;32m"
RED="\033[0;31m"
NC="\033[0m" # No Color

# Contador de fallos
FAILURES=0

# --- Función de Limpieza ---
# Se ejecuta al salir del script
cleanup() {
    rm -f "$SECRET_FILE" "$STEGO_FILE" "${EXTRACTED_FILE}.txt"
}
trap cleanup EXIT

# --- Función de Verificación ---
# $1: Código de retorno de 'diff'
# $2: Nombre de la prueba
check_result() {
    if [ $1 -eq 0 ]; then
        echo -e "  ${GREEN}PASS${NC}: $2"
    else
        echo -e "  ${RED}FAIL${NC}: $2 (Los archivos no coinciden)"
        FAILURES=$((FAILURES + 1))
    fi
    rm -f "$STEGO_FILE" "${EXTRACTED_FILE}.txt"
}

# --- Función de Verificación de Fallo Esperado ---
# $1: Código de retorno del comando
# $2: Nombre de la prueba
check_fail() {
    if [ $1 -ne 0 ]; then
        echo -e "  ${GREEN}PASS${NC}: $2 (Falló como se esperaba)"
    else
        echo -e "  ${RED}FAIL${NC}: $2 (Debía fallar, pero funcionó)"
        FAILURES=$((FAILURES + 1))
    fi
    rm -f "$STEGO_FILE" "${EXTRACTED_FILE}.txt"
}


# --- INICIO DE PRUEBAS ---
echo "Iniciando pruebas de criptografía..."

if [ ! -f "$CARRIER" ]; then
    echo -e "${RED}Error: No se encuentra el archivo '$CARRIER'${NC}"
    exit 1
fi

# Crear archivo secreto
echo "Este es un mensaje secreto para probar la encriptación." > "$SECRET_FILE"


# --- 1. Pruebas de Combinaciones (16 pruebas) ---
echo
echo "--- Pruebas de Combinaciones (4x4) ---"
for algo in $ALGOS; do
    for mode in $MODES; do
        TEST_NAME="$algo / $mode"

        # Embed
        "$EXECUTABLE" -embed -in "$SECRET_FILE" -p "$CARRIER" -out "$STEGO_FILE" -steg LSB1 -a "$algo" -m "$mode" -pass "$PASSWORD" &>/dev/null

        # Extract
        "$EXECUTABLE" -extract -p "$STEGO_FILE" -out "$EXTRACTED_FILE" -steg LSB1 -a "$algo" -m "$mode" -pass "$PASSWORD" &>/dev/null

        # Verify
        diff "$SECRET_FILE" "${EXTRACTED_FILE}.txt" &>/dev/null
        check_result $? "$TEST_NAME"
    done
done


# --- 2. Pruebas de Defaults [cite: 105, 106, 107] ---
echo
echo "--- Pruebas de Defaults ---"

# Prueba A: Solo -pass (Default: aes128-cbc)
"$EXECUTABLE" -embed -in "$SECRET_FILE" -p "$CARRIER" -out "$STEGO_FILE" -steg LSB1 -pass "$PASSWORD" &>/dev/null
"$EXECUTABLE" -extract -p "$STEGO_FILE" -out "$EXTRACTED_FILE" -steg LSB1 -pass "$PASSWORD" &>/dev/null
diff "$SECRET_FILE" "${EXTRACTED_FILE}.txt" &>/dev/null
check_result $? "Default (solo -pass)"

# Prueba B: -pass y -a (Default: cbc)
"$EXECUTABLE" -embed -in "$SECRET_FILE" -p "$CARRIER" -out "$STEGO_FILE" -steg LSB1 -a 3des -pass "$PASSWORD" &>/dev/null
"$EXECUTABLE" -extract -p "$STEGO_FILE" -out "$EXTRACTED_FILE" -steg LSB1 -a 3des -pass "$PASSWORD" &>/dev/null
diff "$SECRET_FILE" "${EXTRACTED_FILE}.txt" &>/dev/null
check_result $? "Default (solo -a 3des, modo cbc)"

# Prueba C: -pass y -m (Default: aes128)
"$EXECUTABLE" -embed -in "$SECRET_FILE" -p "$CARRIER" -out "$STEGO_FILE" -steg LSB1 -m ecb -pass "$PASSWORD" &>/dev/null
"$EXECUTABLE" -extract -p "$STEGO_FILE" -out "$EXTRACTED_FILE" -steg LSB1 -m ecb -pass "$PASSWORD" &>/dev/null
diff "$SECRET_FILE" "${EXTRACTED_FILE}.txt" &>/dev/null
check_result $? "Default (solo -m ecb, algo aes128)"


# --- 3. Prueba de Error (Password incorrecto) ---
echo
echo "--- Pruebas de Fallo ---"
# Embed con la contraseña correcta
"$EXECUTABLE" -embed -in "$SECRET_FILE" -p "$CARRIER" -out "$STEGO_FILE" -steg LSB1 -pass "$PASSWORD" &>/dev/null

# Extract con la contraseña INCORRECTA. Debe fallar.
"$EXECUTABLE" -extract -p "$STEGO_FILE" -out "$EXTRACTED_FILE" -steg LSB1 -pass "PASSWORD_INCORRECTO" &>/dev/null
# Verificamos que el comando falló (código de retorno != 0)
check_fail $? "Password incorrecto"


# --- Resumen Final ---
echo
if [ $FAILURES -eq 0 ]; then
    echo -e "${GREEN}¡ÉXITO! Todas las pruebas de criptografía pasaron.${NC}"
else
    echo -e "${RED}FALLARON $FAILURES PRUEBAS.${NC}"
fi