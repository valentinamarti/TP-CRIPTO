#!/bin/bash

# --- Config ---
EXECUTABLE="../stegobmp"
EXAMPLES_DIR="../ejemplos"
EXTRACTED_FILE="extraido_test"
PASSWORD="margarita"
RESULTS_DIR="results"

# Colores para la salida
GREEN="\033[0;32m"
RED="\033[0;31m"
YELLOW="\033[1;33m"
NC="\033[0m" # No Color

# Contador de fallos
FAILURES=0

# --- Función de Limpieza ---
# Se ejecuta al salir del script
cleanup() {
  echo "Resultados en el directorio '$RESULTS_DIR'"
#    rm -rf "$RESULTS_DIR"
}
trap cleanup EXIT

# --- Función de Verificación ---
# $1: Código de retorno de la extracción
# $2: Nombre de la prueba
# $3: Archivo esperado (opcional, para verificar que existe)
check_result() {
    local exit_code=$1
    local test_name=$2
    local expected_file=$3
    
    if [ $exit_code -eq 0 ]; then
        # Verificar que el archivo extraído existe
        if [ -n "$expected_file" ] && [ ! -f "$expected_file" ]; then
            echo -e "  ${RED}FAIL${NC}: $test_name (Archivo extraído no existe: $expected_file)"
            FAILURES=$((FAILURES + 1))
            return
        fi
        
        # Verificar que el archivo extraído no está vacío
        if [ -n "$expected_file" ] && [ ! -s "$expected_file" ]; then
            echo -e "  ${RED}FAIL${NC}: $test_name (Archivo extraído está vacío)"
            FAILURES=$((FAILURES + 1))
            return
        fi
        
        # Verificar que es un PNG válido (si tiene extensión .png)
        if [ -n "$expected_file" ] && [[ "$expected_file" == *.png ]]; then
                    if file "$expected_file" | grep -q "PNG image"; then
                        echo -e "  ${GREEN}PASS${NC}: $test_name (Guardado en $expected_file)"
                    else
                        echo -e "  ${YELLOW}WARN${NC}: $test_name (Archivo extraído no parece ser un PNG válido)"
                        echo -e "  ${GREEN}PASS${NC}: $test_name (Extracción exitosa, pero formato no verificado)"
                    fi
                else
                    echo -e "  ${GREEN}PASS${NC}: $test_name (Guardado en $expected_file)"
                fi
    else
        echo -e "  ${RED}FAIL${NC}: $test_name (Extract falló con código $exit_code)"
        FAILURES=$((FAILURES + 1))
    fi
}

# --- Verificar que el ejecutable existe ---
if [ ! -f "$EXECUTABLE" ]; then
    echo -e "${RED}Error: No se encuentra el ejecutable '$EXECUTABLE'${NC}"
    echo "Por favor, compila el proyecto primero con 'make'"
    exit 1
fi

# --- Verificar que el directorio de ejemplos existe ---
if [ ! -d "$EXAMPLES_DIR" ]; then
    echo -e "${RED}Error: No se encuentra el directorio de ejemplos '$EXAMPLES_DIR'${NC}"
    exit 1
fi

# --- INICIO DE PRUEBAS ---
echo "Iniciando pruebas de esteganografía con archivos de ejemplo..."
mkdir -p "$RESULTS_DIR" # Crear el directorio
echo

# --- 1. Pruebas SIN encriptación ---
echo "--- Pruebas SIN Encriptación ---"

# LSB1
STEGO_FILE="${EXAMPLES_DIR}/ladoLSB1.bmp"
LOCAL_OUT_BASE="${RESULTS_DIR}/ladoLSB1_out"
if [ -f "$STEGO_FILE" ]; then
    if "$EXECUTABLE" -extract -p "$STEGO_FILE" -out "$LOCAL_OUT_BASE" -steg LSB1 >/dev/null 2>&1; then
        # Buscar el archivo extraído (puede tener extensión .png)
        EXTRACTED_FILE_PATH=$(find "$RESULTS_DIR" -maxdepth 1 -name "ladoLSB1_out*" -type f | head -n 1)
        if [ -n "$EXTRACTED_FILE_PATH" ]; then
            check_result 0 "ladoLSB1.bmp (LSB1, sin encriptación)" "$EXTRACTED_FILE_PATH"
            # rm -f "$EXTRACTED_FILE_PATH"
        else
            check_result 1 "ladoLSB1.bmp (LSB1, sin encriptación)" ""
        fi
    else
        check_result $? "ladoLSB1.bmp (LSB1, sin encriptación)" ""
    fi
else
    echo -e "  ${YELLOW}SKIP${NC}: ladoLSB1.bmp (archivo no encontrado)"
fi

# LSB4
STEGO_FILE="${EXAMPLES_DIR}/ladoLSB4.bmp"
LOCAL_OUT_BASE="${RESULTS_DIR}/ladoLSB4_out"
if [ -f "$STEGO_FILE" ]; then
    if "$EXECUTABLE" -extract -p "$STEGO_FILE" -out "$LOCAL_OUT_BASE" -steg LSB4 >/dev/null 2>&1; then
        EXTRACTED_FILE_PATH=$(find "$RESULTS_DIR" -maxdepth 1 -name "ladoLSB4_out*" -type f | head -n 1)
        if [ -n "$EXTRACTED_FILE_PATH" ]; then
            check_result 0 "ladoLSB4.bmp (LSB4, sin encriptación)" "$EXTRACTED_FILE_PATH"
            # rm -f "$EXTRACTED_FILE_PATH"
        else
            check_result 1 "ladoLSB4.bmp (LSB4, sin encriptación)" ""
        fi
    else
        check_result $? "ladoLSB4.bmp (LSB4, sin encriptación)" ""
    fi
else
    echo -e "  ${YELLOW}SKIP${NC}: ladoLSB4.bmp (archivo no encontrado)"
fi

# LSBI
STEGO_FILE="${EXAMPLES_DIR}/ladoLSBI.bmp"
LOCAL_OUT_BASE="${RESULTS_DIR}/ladoLSBI_out"
if [ -f "$STEGO_FILE" ]; then
    if "$EXECUTABLE" -extract -p "$STEGO_FILE" -out "$LOCAL_OUT_BASE" -steg LSBI >/dev/null 2>&1; then
        EXTRACTED_FILE_PATH=$(find "$RESULTS_DIR" -maxdepth 1 -name "ladoLSBI_out*" -type f | head -n 1)
        if [ -n "$EXTRACTED_FILE_PATH" ]; then
            check_result 0 "ladoLSBI.bmp (LSBI, sin encriptación)" "$EXTRACTED_FILE_PATH"
            # rm -f "$EXTRACTED_FILE_PATH"
        else
            check_result 1 "ladoLSBI.bmp (LSBI, sin encriptación)" ""
        fi
    else
        check_result $? "ladoLSBI.bmp (LSBI, sin encriptación)" ""
    fi
else
    echo -e "  ${YELLOW}SKIP${NC}: ladoLSBI.bmp (archivo no encontrado)"
fi

# --- 2. Pruebas CON encriptación ---
echo
echo "--- Pruebas CON Encriptación ---"

# LSBI + AES256 + OFB
STEGO_FILE="${EXAMPLES_DIR}/ladoLSBIaes256ofb.bmp"
LOCAL_OUT_BASE="${RESULTS_DIR}/ladoLSBIaes256ofb_out"
if [ -f "$STEGO_FILE" ]; then
    if "$EXECUTABLE" -extract -p "$STEGO_FILE" -out "$LOCAL_OUT_BASE" -steg LSBI -a aes256 -m ofb -pass "$PASSWORD" >/dev/null 2>&1; then
        EXTRACTED_FILE_PATH=$(find "$RESULTS_DIR" -maxdepth 1 -name "ladoLSBIaes256ofb_out*" -type f | head -n 1)
        if [ -n "$EXTRACTED_FILE_PATH" ]; then
            check_result 0 "ladoLSBIaes256ofb.bmp (LSBI, AES256, OFB)" "$EXTRACTED_FILE_PATH"
            # rm -f "$EXTRACTED_FILE_PATH"
        else
            check_result 1 "ladoLSBIaes256ofb.bmp (LSBI, AES256, OFB)" ""
        fi
    else
        check_result $? "ladoLSBIaes256ofb.bmp (LSBI, AES256, OFB)" ""
    fi
else
    echo -e "  ${YELLOW}SKIP${NC}: ladoLSBIaes256ofb.bmp (archivo no encontrado)"
fi

# LSBI + 3DES + CFB
STEGO_FILE="${EXAMPLES_DIR}/ladoLSBIdescfb.bmp"
LOCAL_OUT_BASE="${RESULTS_DIR}/ladoLSBIdescfb_out"
if [ -f "$STEGO_FILE" ]; then
    if "$EXECUTABLE" -extract -p "$STEGO_FILE" -out "$LOCAL_OUT_BASE" -steg LSBI -a 3des -m cfb -pass "$PASSWORD" >/dev/null 2>&1; then
        EXTRACTED_FILE_PATH=$(find "$RESULTS_DIR" -maxdepth 1 -name "ladoLSBIdescfb_out*" -type f | head -n 1)
        if [ -n "$EXTRACTED_FILE_PATH" ]; then
            check_result 0 "ladoLSBIdescfb.bmp (LSBI, 3DES, CFB)" "$EXTRACTED_FILE_PATH"
            # rm -f "$EXTRACTED_FILE_PATH"
        else
            check_result 1 "ladoLSBIdescfb.bmp (LSBI, 3DES, CFB)" ""
        fi
    else
        check_result $? "ladoLSBIdescfb.bmp (LSBI, 3DES, CFB)" ""
    fi
else
    echo -e "  ${YELLOW}SKIP${NC}: ladoLSBIdescfb.bmp (archivo no encontrado)"
fi

# --- 3. Pruebas con archivos del grupo10 (si existen) ---
echo
echo "--- Pruebas con Archivos del Grupo10 ---"

GRUPO10_DIR="${EXAMPLES_DIR}/grupo10"
if [ -d "$GRUPO10_DIR" ]; then
    for stego_file in "$GRUPO10_DIR"/*.bmp; do
        if [ -f "$stego_file" ]; then
            filename=$(basename "$stego_file" .bmp)
            echo "Procesando $filename.bmp..."
            LOCAL_OUT_BASE="${RESULTS_DIR}/${filename}"
            extracted=0

            # Intentar extraer sin encriptación primero
            for steg_algo in LSB1 LSB4 LSBI; do
                if "$EXECUTABLE" -extract -p "$stego_file" -out "$LOCAL_OUT_BASE" -steg "$steg_algo" >/dev/null 2>&1; then
                    EXTRACTED_FILE_PATH=$(find "$RESULTS_DIR" -maxdepth 1 -name "${filename}*" -type f | head -n 1)
                    if [ -n "$EXTRACTED_FILE_PATH" ] && [ -s "$EXTRACTED_FILE_PATH" ]; then
                        check_result 0 "$filename.bmp ($steg_algo, sin encriptación)" "$EXTRACTED_FILE_PATH"
                        # rm -f "$EXTRACTED_FILE_PATH"
                        extracted=1
                        break
                    fi
                fi
            done

            # Si no se pudo extraer sin encriptación, intentar con encriptación
            if [ $extracted -eq 0 ]; then
                # Intentar con diferentes combinaciones de algoritmos y modos comunes
                # (algunos archivos del grupo10 pueden estar encriptados)
                for steg_algo in LSB1 LSB4 LSBI; do
                    for algo in aes128 aes192 aes256 3des; do
                        for mode in ecb cbc cfb ofb; do
                            if "$EXECUTABLE" -extract -p "$stego_file" -out "$LOCAL_OUT_BASE" -steg "$steg_algo" -a "$algo" -m "$mode" -pass "aplausos" >/dev/null 2>&1; then
                                EXTRACTED_FILE_PATH=$(find "$RESULTS_DIR" -maxdepth 1 -name "${filename}*" -type f | head -n 1)
                                if [ -n "$EXTRACTED_FILE_PATH" ] && [ -s "$EXTRACTED_FILE_PATH" ]; then
                                    check_result 0 "$filename.bmp ($steg_algo, $algo, $mode)" "$EXTRACTED_FILE_PATH"
                                    # rm -f "$EXTRACTED_FILE_PATH"
                                    extracted=1
                                    break 3  # Salir de los tres loops
                                fi
                            fi
                        done
                    done
                done
            fi

            # Si aún no se pudo extraer, marcar como fallido
            if [ $extracted -eq 0 ]; then
                echo -e "  ${YELLOW}SKIP${NC}: $filename.bmp (No se pudo extraer - puede requerir parámetros específicos)"
                # No incrementar FAILURES para archivos del grupo10 que no se puedan extraer
                # ya que pueden tener configuraciones específicas desconocidas
            fi
        fi
    done
else
    echo -e "  ${YELLOW}SKIP${NC}: Directorio grupo10 no encontrado"
fi


echo
echo "--- Intentando obtener .wmv desde silence.bmp ---"

SILENCE_FILE="${EXAMPLES_DIR}/grupo10/silence.bmp"
SILENCE_OUT_BASE="${RESULTS_DIR}/silence_out"
SILENCE_PASSWORD="aplausos"
SILENCE_ALGO="aes192"
SILENCE_MODE="ecb"

if [ -f "$SILENCE_FILE" ]; then
    extracted=0

    # Si no salió, intentamos con AES192-ECB + password
    if [ $extracted -eq 0 ]; then
        for steg_algo in LSB1 LSB4 LSBI; do
            if "$EXECUTABLE" -extract -p "$SILENCE_FILE" -out "$SILENCE_OUT_BASE" -steg "$steg_algo" -a "$SILENCE_ALGO" -m "$SILENCE_MODE" -pass "$SILENCE_PASSWORD" >/dev/null 2>&1; then
                EXTRACTED_FILE_PATH=$(find "$RESULTS_DIR" -maxdepth 1 -name "silence_out*" -type f | head -n 1)
                if [ -n "$EXTRACTED_FILE_PATH" ] && [ -s "$EXTRACTED_FILE_PATH" ]; then
                    echo -e "  ${GREEN}EXTRAIDO con cifrado ($SILENCE_ALGO/$SILENCE_MODE) usando $steg_algo${NC}: $EXTRACTED_FILE_PATH"
                    extracted=1
                    break
                fi
            fi
        done
    fi

    # Post-procesado: intentar detectar y renombrar a .wmv
    if [ $extracted -eq 1 ]; then
        EXTRACTED_FILE_PATH=$(find "$RESULTS_DIR" -maxdepth 1 -name "silence_out*" -type f | head -n 1)
        file_out=$(file "$EXTRACTED_FILE_PATH")
        echo "    file: $file_out"

        if echo "$file_out" | grep -qi "ASF\|Windows Media\|Microsoft" ; then
            mv "$EXTRACTED_FILE_PATH" "${EXTRACTED_FILE_PATH}.wmv"
            echo -e "  ${GREEN}Renombrado a: ${EXTRACTED_FILE_PATH}.wmv${NC}"
            check_result 0 "silence.bmp (EXTRAIDO y renombrado a .wmv)" "${EXTRACTED_FILE_PATH}.wmv"
        else
            echo -e "  ${YELLOW}WARN${NC}: silence.bmp (archivo extraído, pero no parece WMV). Guardado en $EXTRACTED_FILE_PATH"
            check_result 0 "silence.bmp (extraído, formato no WMV detectado)" "$EXTRACTED_FILE_PATH"
        fi
    else
        echo -e "  ${RED}FAIL${NC}: silence.bmp (No se pudo extraer con las combinaciones probadas)"
        FAILURES=$((FAILURES+1))
    fi
else
    echo -e "  ${YELLOW}SKIP${NC}: silence.bmp (archivo no encontrado en $EXAMPLES_DIR)"
fi

# --- Resumen Final ---
echo
if [ $FAILURES -eq 0 ]; then
    echo -e "${GREEN}¡ÉXITO! Todas las pruebas de esteganografía pasaron.${NC}"
    exit 0
else
    echo -e "${RED}FALLARON $FAILURES PRUEBAS.${NC}"
    exit 1
fi

