#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <vector>
#include <cmath>

// Configuración de la ventana y la grilla
const unsigned int ANCHO_VENTANA = 800;
const unsigned int ALTO_VENTANA = 800;
const int GRILLA_TAMANO = 20; // Grilla de 20x20 celdas

// Estructura para almacenar puntos enteros
struct Punto {
    int x, y;
};

// Shaders GLSL integrados
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec3 aColor;
    out vec3 FragColor;
    void main() {
        gl_Position = vec4(aPos, 0.0, 1.0);
        FragColor = aColor;
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    in vec3 FragColor;
    out vec4 FragColorOut;
    void main() {
        FragColorOut = vec4(FragColor, 1.0);
    }
)";

// Algoritmo de Bresenham exacto según las diapositivas
std::vector<Punto> calcularBresenham(int x0, int y0, int x1, int y1) {
    std::vector<Punto> puntos;
    int dx = std::abs(x1 - x0);
    int dy = std::abs(y1 - y0);

    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;

    int x = x0;
    int y = y0;

    std::cout << "=== TRAZA DEL ALGORITMO DE BRESENHAM ===" << std::endl;
    std::cout << "Punto Inicial: (" << x0 << ", " << y0 << ") -> Final: (" << x1 << ", " << y1 << ")" << std::endl;
    std::cout << "dx: " << dx << ", dy: " << dy << "\n----------------------------------" << std::endl;

    // CASO 1: Eje dominante es X (|dx| >= |dy|)
    if (dx >= dy) {
        int P = 2 * dy - dx; // Parámetro inicial P0 = 2dy - dx
        std::cout << "P0 = " << P << std::endl;

        for (int k = 0; k <= dx; k++) {
            puntos.push_back({ x, y });
            std::cout << "k=" << k << "\tP=" << P << "\tPíxel: (" << x << ", " << y << ")" << std::endl;

            if (P >= 0) {
                y += sy;
                P += 2 * dy - 2 * dx; // P_k+1 = P_k + 2dy - 2dx
            }
            else {
                P += 2 * dy;          // P_k+1 = P_k + 2dy
            }
            x += sx;
        }
    }
    // CASO 2: Eje dominante es Y (|dy| > |dx|)
    else {
        int P = 2 * dx - dy; // Parámetro inicial P0 = 2dx - dy
        std::cout << "P0 = " << P << std::endl;

        for (int k = 0; k <= dy; k++) {
            puntos.push_back({ x, y });
            std::cout << "k=" << k << "\tP=" << P << "\tPíxel: (" << x << ", " << y << ")" << std::endl;

            if (P >= 0) {
                x += sx;
                P += 2 * dx - 2 * dy; // P_k+1 = P_k + 2dx - 2dy
            }
            else {
                P += 2 * dx;          // P_k+1 = P_k + 2dx
            }
            y += sy;
        }
    }

    return puntos;
}

// Función auxiliar para compilar shaders
GLuint crearShaderProgram() {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// Genera los vértices para la grilla de fondo
void generarVerticesGrilla(std::vector<float>& vertices) {
    float paso = 2.0f / GRILLA_TAMANO;

    // Color de las líneas de la grilla (Gris claro)
    float r = 0.25f, g = 0.25f, b = 0.25f;

    for (int i = 0; i <= GRILLA_TAMANO; i++) {
        float pos = -1.0f + i * paso;

        // Líneas verticales
        vertices.insert(vertices.end(), { pos, -1.0f, r, g, b });
        vertices.insert(vertices.end(), { pos,  1.0f, r, g, b });

        // Líneas horizontales
        vertices.insert(vertices.end(), { -1.0f, pos, r, g, b });
        vertices.insert(vertices.end(), { 1.0f, pos, r, g, b });
    }
}

// Genera los cuadrados (píxeles) rellenos calculados por Bresenham
void generarVerticesPixeles(const std::vector<Punto>& puntos, std::vector<float>& vertices) {
    float tamanoCelda = 2.0f / GRILLA_TAMANO;
    float r = 0.2f, g = 0.6f, b = 1.0f; // Color Azul para los píxeles trazados

    for (const auto& p : puntos) {
        float xMin = -1.0f + p.x * tamanoCelda;
        float xMax = xMin + tamanoCelda;
        float yMin = -1.0f + p.y * tamanoCelda;
        float yMax = yMin + tamanoCelda;

        // 2 Triángulos por cada celda de la grilla
        vertices.insert(vertices.end(), {
            xMin, yMin, r, g, b,
            xMax, yMin, r, g, b,
            xMax, yMax, r, g, b,

            xMin, yMin, r, g, b,
            xMax, yMax, r, g, b,
            xMin, yMax, r, g, b
            });
    }
}

// Genera la línea roja continua de referencia
void generarVerticesLineaIdeal(int x0, int y0, int x1, int y1, std::vector<float>& vertices) {
    float tamanoCelda = 2.0f / GRILLA_TAMANO;

    float cx0 = -1.0f + (x0 + 0.5f) * tamanoCelda;
    float cy0 = -1.0f + (y0 + 0.5f) * tamanoCelda;
    float cx1 = -1.0f + (x1 + 0.5f) * tamanoCelda;
    float cy1 = -1.0f + (y1 + 0.5f) * tamanoCelda;

    float r = 1.0f, g = 0.2f, b = 0.2f; // Rojo brillante

    vertices.insert(vertices.end(), { cx0, cy0, r, g, b });
    vertices.insert(vertices.end(), { cx1, cy1, r, g, b });
}

// Callback para redimensionar la ventana
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

int main() {
    // 1. Inicializar GLFW
    if (!glfwInit()) {
        std::cerr << "Error al inicializar GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 2. Crear la ventana GLFW
    GLFWwindow* window = glfwCreateWindow(ANCHO_VENTANA, ALTO_VENTANA, "Algoritmo de Bresenham - OpenGL (GLFW/GLAD)", NULL, NULL);
    if (!window) {
        std::cerr << "Error al crear la ventana GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // 3. Cargar punteros de OpenGL con GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Error al inicializar GLAD" << std::endl;
        return -1;
    }

    // 4. Coordenadas de prueba
    int x0 = 2, y0 = 2;
    int x1 = 15, y1 = 8;

    // Calcular puntos con Bresenham
    std::vector<Punto> puntosBresenham = calcularBresenham(x0, y0, x1, y1);

    // 5. Preparar datos de geometría
    std::vector<float> verticesGrilla;
    generarVerticesGrilla(verticesGrilla);

    std::vector<float> verticesPixeles;
    generarVerticesPixeles(puntosBresenham, verticesPixeles);

    std::vector<float> verticesLineaIdeal;
    generarVerticesLineaIdeal(x0, y0, x1, y1, verticesLineaIdeal);

    // 6. Crear Shader Program
    GLuint shaderProgram = crearShaderProgram();

    // 7. Configurar VAOs y VBOs
    GLuint vaoGrilla, vboGrilla;
    glGenVertexArrays(1, &vaoGrilla);
    glGenBuffers(1, &vboGrilla);
    glBindVertexArray(vaoGrilla);
    glBindBuffer(GL_ARRAY_BUFFER, vboGrilla);
    glBufferData(GL_ARRAY_BUFFER, verticesGrilla.size() * sizeof(float), verticesGrilla.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    GLuint vaoPixeles, vboPixeles;
    glGenVertexArrays(1, &vaoPixeles);
    glGenBuffers(1, &vboPixeles);
    glBindVertexArray(vaoPixeles);
    glBindBuffer(GL_ARRAY_BUFFER, vboPixeles);
    glBufferData(GL_ARRAY_BUFFER, verticesPixeles.size() * sizeof(float), verticesPixeles.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    GLuint vaoLinea, vboLinea;
    glGenVertexArrays(1, &vaoLinea);
    glGenBuffers(1, &vboLinea);
    glBindVertexArray(vaoLinea);
    glBindBuffer(GL_ARRAY_BUFFER, vboLinea);
    glBufferData(GL_ARRAY_BUFFER, verticesLineaIdeal.size() * sizeof(float), verticesLineaIdeal.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glLineWidth(2.5f);

    // 8. Bucle principal de renderizado
    while (!glfwWindowShouldClose(window)) {
        // Entrada de teclado (Cerrar con ESC)
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        // Limpiar pantalla (Fondo negro)
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // Dibuja los píxeles rellenados por Bresenham (Azul)
        glBindVertexArray(vaoPixeles);
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(verticesPixeles.size() / 5));

        // Dibuja la grilla de coordenadas (Gris)
        glBindVertexArray(vaoGrilla);
        glDrawArrays(GL_LINES, 0, (GLsizei)(verticesGrilla.size() / 5));

        // Dibuja la línea continua ideal (Roja)
        glBindVertexArray(vaoLinea);
        glDrawArrays(GL_LINES, 0, (GLsizei)(verticesLineaIdeal.size() / 5));

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Liberar recursos
    glDeleteVertexArrays(1, &vaoGrilla);
    glDeleteBuffers(1, &vboGrilla);
    glDeleteVertexArrays(1, &vaoPixeles);
    glDeleteBuffers(1, &vboPixeles);
    glDeleteVertexArrays(1, &vaoLinea);
    glDeleteBuffers(1, &vboLinea);
    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}