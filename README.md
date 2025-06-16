# HULK Compiler 🚀

Un compilador moderno y robusto para el lenguaje de programación HULK con soporte completo para herencia, operaciones matemáticas, manipulación de strings y generación de código LLVM IR.

## ✨ Características Principales

### 🎯 Funcionalidades del Lenguaje
- **Herencia Múltiple**: Soporte completo para herencia de tipos con resolución dinámica de métodos
- **Operaciones Matemáticas**: Funciones built-in (`pow`, `sqrt`, `sin`, `cos`, `PI`, `E`)
- **Manipulación de Strings**: Concatenación (`@`), conversiones automáticas de tipos
- **Tipos Dinámicos**: Inferencia automática de tipos de campos y parámetros
- **Métodos y Propiedades**: Sistema completo de métodos con `self` y `base`
- **Constructores**: Métodos `init` con inicialización automática de campos

### 🏗️ Arquitectura Técnica
- **Modo Interpretación**: Ejecución directa del código HULK (modo por defecto)
- **Generación LLVM IR**: Compilación a código intermedio LLVM para optimización
- **Análisis Semántico**: Verificación de tipos, funciones y herencia
- **Patrón Visitor**: Arquitectura limpia y extensible
- **Fallback Automático**: Funciona con o sin LLVM instalado

### 🔧 Soporte Multiplataforma
- **Windows**: MSYS2/MinGW y Windows nativo
- **Linux**: Distribuciones con GCC/Clang
- **macOS**: Soporte completo con Homebrew
- **Detección Automática**: El makefile detecta automáticamente el entorno

## 🚀 Uso Rápido

### Instalación y Ejecución
```bash
# 1. Clonar el repositorio
git clone <repositorio-hulk>
cd Hulk-Compiler

# 2. Copiar tu código HULK en script.hulk
# (Ya incluye un ejemplo completo de herencia y operaciones)

# 3. Compilar el proyecto
make compile

# 4. Ejecutar tu programa HULK
make execute
```

### Comandos Disponibles
```bash
make help      # Mostrar ayuda de uso
make info      # Información del sistema y configuración
make compile   # Compilar y generar artifacts en hulk/
make execute   # Ejecutar el programa HULK (incluye compilación automática)
make clean     # Limpiar archivos generados
```

## 🔧 Opciones Avanzadas del Compilador

El compilador HULK incluye opciones avanzadas para análisis detallado, depuración y optimización de código:

### 💡 Modo LLVM (Generación de Código Optimizado)
```bash
# Ejecución directa con LLVM IR
./hulk/hulk_compiler.exe script.hulk --llvm

# A través del makefile
make execute ARGS="--llvm"

# Objetivo específico del makefile
make execute-llvm
```
**Características:**
- Genera código intermedio LLVM optimizado
- Muestra el IR generado en pantalla para análisis
- Mejor rendimiento para programas complejos
- Optimizaciones automáticas de bucles y expresiones

### 🐛 Modo Debug (Información de Depuración)
```bash
# Ejecución con información detallada
./hulk/hulk_compiler.exe script.hulk --debug

# A través del makefile
make execute ARGS="--debug"

# Objetivo específico del makefile
make execute-debug
```
**Información mostrada:**
- Pasos detallados del análisis sintáctico y semántico
- Resolución de tipos en tiempo real
- Información de scope y vinculación de variables
- Detalles de herencia y resolución de métodos
- Útil para depurar errores complejos

### 📊 Modo Show-IR (Solo Generación de Código)
```bash
# Mostrar únicamente el código LLVM IR
./hulk/hulk_compiler.exe script.hulk --show-ir

# A través del makefile
make execute ARGS="--show-ir"

# Objetivo específico del makefile
make show-ir
```
**Uso principal:**
- Genera y muestra únicamente el código LLVM IR
- No ejecuta el programa
- Ideal para análisis de optimizaciones
- Útil para entender la representación interna

### 🔗 Combinación de Opciones
```bash
# Combinar múltiples opciones para análisis completo
./hulk/hulk_compiler.exe script.hulk --llvm --debug

# Con makefile
make execute ARGS="--llvm --debug"
```

### 📝 Ejemplos Prácticos

Para probar las opciones avanzadas, puedes usar el script de demostración incluido:

```bash
# Copiar el script de demostración avanzada
cp examples/advanced_demo.hulk script.hulk

# Probar con diferentes opciones
make execute                        # Modo interpretación normal
make execute-llvm                   # Con generación LLVM IR
make execute-debug                  # Con información de depuración
make show-ir                        # Solo mostrar IR generado
make execute ARGS="--llvm --debug"  # Combinación completa
```

## 📋 Requisitos del Sistema

### Dependencias Mínimas (Fallback)
- **Compilador**: GCC 7.0+ o Clang 6.0+
- **Herramientas**: Flex, Bison, Make
- **Estándar**: C++17

### Dependencias Completas (Recomendado)
- **LLVM**: 10.0+ (para generación de IR optimizado)
- **Clang**: Compilador principal cuando LLVM está disponible

### Instalación de Dependencias

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install build-essential flex bison make
# Para soporte LLVM completo:
sudo apt-get install llvm-dev clang
```

#### macOS
```bash
brew install llvm flex bison make
# Agregar LLVM al PATH:
export PATH="/opt/homebrew/opt/llvm/bin:$PATH"
```

#### Windows (MSYS2)
```bash
pacman -S base-devel mingw-w64-x86_64-toolchain
pacman -S mingw-w64-x86_64-flex mingw-w64-x86_64-bison
# Para LLVM:
pacman -S mingw-w64-x86_64-llvm mingw-w64-x86_64-clang
```

## 🎨 Ejemplo de Código HULK

El proyecto incluye un ejemplo completo en `script.hulk` que demuestra:

```hulk
// Herencia múltiple (3 niveles)
type Vehicle {
    wheels = 4;
    fuel = 100.0;
    
    init(w, f) {
        self.wheels := w;
        self.fuel := f;
    }
    
    drive() => "Vehicle moving with " @ str(self.wheels) @ " wheels";
}

type Car inherits Vehicle {
    brand = "Generic";
    
    init(w, f, b) {
        base.init(w, f);  // Llamada al constructor padre
        self.brand := b;
    }
    
    honk() => self.brand @ " goes BEEP BEEP!";
}

type SportsCar inherits Car {
    maxSpeed = 200.0;
    
    init(w, f, b, speed) {
        base.init(w, f, b);  // Herencia de múltiples niveles
        self.maxSpeed := speed;
    }
    
    turbo() => "TURBO MODE: " @ str(self.maxSpeed * 1.5) @ " km/h!";
}

// Uso del sistema de herencia
let ferrari = new SportsCar(4, 80.0, "Ferrari", 350.0) in {
    print(ferrari.drive());      // Método heredado de Vehicle
    print(ferrari.honk());       // Método heredado de Car  
    print(ferrari.turbo());      // Método propio de SportsCar
};
```

### Características Demostradas
- ✅ **Herencia múltiple**: Vehicle → Car → SportsCar
- ✅ **Constructores con parámetros**: `init(w, f, b, speed)`
- ✅ **Llamadas a constructores padre**: `base.init()`
- ✅ **Métodos heredados**: Acceso a métodos de clases padre
- ✅ **Concatenación de strings**: Operador `@`
- ✅ **Conversiones automáticas**: `str()` para números
- ✅ **Operaciones matemáticas**: `pow()`, `sqrt()`, etc.

## 🏗️ Arquitectura del Proyecto

```
Hulk-Compiler/
├── src/
│   ├── AST/              # Árbol de Sintaxis Abstracta
│   ├── Lexer/            # Análisis léxico (Flex)
│   ├── Parser/           # Análisis sintáctico (Bison)
│   ├── Semantic/         # Análisis semántico y tipos
│   ├── Evaluator/        # Intérprete principal
│   ├── CodeGen/          # Generador de código LLVM
│   ├── Runtime/          # Biblioteca runtime en C
│   ├── Value/            # Sistema de valores y objetos
│   └── main.cpp          # Punto de entrada
├── hulk/                 # Directorio de artifacts (generado)
│   └── hulk_compiler.exe # Ejecutable compilado
├── script.hulk           # Script de ejemplo
├── makefile              # Sistema de build multiplataforma
└── README.md             # Este archivo
```

## 🔍 Información Técnica

### Modos de Operación

1. **Modo Interpretación** (Por defecto)
   - Ejecución directa del código HULK
   - Soporte completo para todas las características
   - No requiere LLVM

2. **Modo LLVM** (Cuando está disponible)
   - Generación de código intermedio LLVM IR
   - Optimizaciones automáticas
   - Mejor rendimiento para programas complejos

### Sistema de Tipos
- **Inferencia automática**: Los tipos se deducen del contexto
- **Tipos soportados**: `Number`, `String`, `Boolean`, tipos definidos por usuario
- **Conversiones**: Automáticas entre números y strings donde sea apropiado

### Funciones Built-in
```hulk
// Matemáticas
pow(x, y)     // x elevado a y
sqrt(x)       // Raíz cuadrada
sin(x)        // Seno
cos(x)        // Coseno
PI            // Constante π
E             // Constante e

// Utilidades
str(x)        // Convertir a string
print(x)      // Imprimir valor
debug(x)      // Información de depuración
```

## 🐛 Depuración y Desarrollo

### 🔍 Información del Sistema
```bash
make info
```
**Muestra:**
- Sistema operativo detectado y entorno
- Compilador en uso (GCC/Clang)
- Estado de LLVM (versión o fallback)
- Rutas de instalación y configuración

### 📋 Análisis de Errores

El compilador proporciona mensajes de error detallados:

**Errores semánticos comunes:**
```bash
# Error de tipo incompatible
Type mismatch: cannot assign String to Number

# Error de método no encontrado  
Method 'invalidMethod' not found in type 'MyType'

# Error de herencia circular
Circular inheritance detected: A inherits B inherits A
```

**Consejos de depuración:**
- Usa `--debug` para ver el proceso de resolución de tipos
- Verifica que los métodos `init` tengan los parámetros correctos
- En herencia, asegúrate de llamar `base.init()` con todos los parámetros

### 🧹 Limpieza y Mantenimiento
```bash
make clean           # Limpiar archivos generados
make info            # Verificar configuración actual
```

**En Windows (PowerShell):**
```powershell
.\scripts\cleanup-project.ps1  # Script de limpieza avanzada
```

### 📊 Logs de Compilación
El makefile proporciona información visual durante la compilación:
- 🔧 Generación de parser/lexer
- 🔨 Compilación de módulos individuales  
- 🔗 Enlazado final con bibliotecas
- 📦 Artifacts generados en hulk/

## 🤝 Contribución

El proyecto está diseñado con una arquitectura modular que facilita la extensión:

- **Nuevos operadores**: Agregar en `Lexer` y `Parser`
- **Nuevos tipos**: Extender el sistema de tipos en `Semantic`
- **Nuevas funciones built-in**: Agregar en `Evaluator`
- **Optimizaciones LLVM**: Mejorar `CodeGen`

### � Scripts de Ejemplo

El proyecto incluye varios scripts de prueba en `examples/` y `tests/`:

- `script.hulk` - Script principal con herencia múltiple
- `examples/advanced_demo.hulk` - Demostración de características avanzadas
- `tests/` - Conjunto completo de pruebas unitarias

## � Licencia

Este proyecto es parte del curso de Compilación y está disponible para fines educativos.

---

**¡El compilador HULK está listo para ejecutar programas con herencia, operaciones matemáticas y manipulación de strings!** 🎉

### � Comenzar Ahora

```bash
# Flujo completo de inicio
git clone <repositorio-hulk>
cd Hulk-Compiler
cp examples/advanced_demo.hulk script.hulk
make compile
make execute

# Probar opciones avanzadas
make execute-llvm
make execute-debug  
make show-ir
```
