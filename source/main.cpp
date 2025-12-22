#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <iostream>
#include <string>

// --- CONFIGURATION ---
const int COLS = 20;
const int ROWS = 20;
const float WIDTH = 2.0f / COLS;
const float HEIGHT = 2.0f / ROWS;

struct Point {
    int x;
    int y;

    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
};

// --- GAME STATE ---
std::vector<Point> snakeBody = {{10,10}, {10,9}, {10,8}};
Point snakeDir = {0, 1};
Point apple {2, 2};
bool gameOver = false;
bool isPaused = false;

// --- TIMERS ---
double lastTime = 0.0;
double moveDelay = 0.4;

void generateFood() {
    bool onSnake;

    do {
        apple.x = rand() % COLS;
        apple.y = rand() % ROWS;
        onSnake = false;

        for(const Point& point : snakeBody)
            if(point == apple) {
                onSnake = true;
                break;
            }

    } while(onSnake);
}

void restartGame() {
    snakeBody = {{10,10}, {10,9}, {10,8}};
    snakeDir = {0, 1};
    apple = {2, 2};
    gameOver = false;
    isPaused = true;
}

bool isCollideBody(Point newHead) {
    for(const Point& point : snakeBody) {
        if(newHead == point)
            return true;
    }
    return false;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) glfwSetWindowShouldClose(window, true);
        if (key == GLFW_KEY_UP && snakeDir.y != -1)    snakeDir = {0, 1};
        if (key == GLFW_KEY_DOWN && snakeDir.y != 1)   snakeDir = {0, -1};
        if (key == GLFW_KEY_LEFT && snakeDir.x != 1)   snakeDir = {-1, 0};
        if (key == GLFW_KEY_RIGHT && snakeDir.x != -1) snakeDir = {1, 0};
        if (key == GLFW_KEY_P) isPaused = !isPaused;
        if (key == GLFW_KEY_R) restartGame();
    }
}

int main()
{
    if(!glfwInit()) return -1;
    srand(time(0));
    generateFood();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(800, 800, "Snake OpenGL", nullptr, nullptr);
    if(window == nullptr) {
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if(glewInit() != GLEW_OK) {
        glfwTerminate();
        return -1;
    }

    // --- SHADERS ---
    std::string vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 color;

        out vec3 vColor;
        uniform vec3 movePos;

        void main()
        {
            vColor = color;
            gl_Position = vec4(aPos.x + movePos.x, aPos.y + movePos.y, aPos.z + movePos.z, 1.0);
        }
    )";

    std::string fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        in vec3 vColor;
        uniform vec4 uColor;

        void main()
        {
            FragColor = vec4(vColor, 1.0f) * uColor;
        }
    )";

    // Compile Vertex Shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    const char* vShaderCode = vertexShaderSource.c_str();
    glShaderSource(vertexShader, 1, &vShaderCode, nullptr);
    glCompileShader(vertexShader);

    // Compile Fragment Shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fShaderCode = fragmentShaderSource.c_str();
    glShaderSource(fragmentShader, 1, &fShaderCode, nullptr);
    glCompileShader(fragmentShader);

    // Link Program
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // --- MAP DATA (GRID) ---
    std::vector<float> mapVertices;
    std::vector<unsigned int> mapIndices;
    int indexCounter = 0;

    for(int y = 0; y < ROWS; y++) {
        for(int x = 0; x < COLS; x++) {
            float startX = -1.f + (x * WIDTH);
            float startY = -1.f + (y * HEIGHT);
            float c = ((x+y) % 2 == 0) ? 0.1f : 0.05f; // Darker checkerboard

            mapVertices.insert(mapVertices.end(), {
                startX + WIDTH, startY + HEIGHT, 0.0f, c, c, c,
                startX + WIDTH, startY,          0.0f, c, c, c,
                startX,         startY,          0.0f, c, c, c,
                startX,         startY + HEIGHT, 0.0f, c, c, c
            });

            mapIndices.insert(mapIndices.end(), {
                (unsigned int)(indexCounter + 0), (unsigned int)(indexCounter + 1), (unsigned int)(indexCounter + 3),
                (unsigned int)(indexCounter + 1), (unsigned int)(indexCounter + 2), (unsigned int)(indexCounter + 3)
            });
            indexCounter += 4;
        }
    }

    GLuint mapVAO, mapVBO, mapEBO;
    glGenVertexArrays(1, &mapVAO);
    glGenBuffers(1, &mapVBO);
    glGenBuffers(1, &mapEBO);

    glBindVertexArray(mapVAO);
    glBindBuffer(GL_ARRAY_BUFFER, mapVBO);
    glBufferData(GL_ARRAY_BUFFER, mapVertices.size() * sizeof(float), mapVertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mapEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mapIndices.size() * sizeof(unsigned int), mapIndices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // --- SNAKE DATA (SINGLE BLOCK) ---
    float blockVertices[] = {
        WIDTH, HEIGHT, 0.0f,  1.0f, 1.0f, 1.0f,
        WIDTH, 0.0f,   0.0f,  1.0f, 1.0f, 1.0f,
        0.0f,  0.0f,   0.0f,  1.0f, 1.0f, 1.0f,
        0.0f,  HEIGHT, 0.0f,  1.0f, 1.0f, 1.0f
    };
    unsigned int blockIndices[] = { 0, 1, 3, 1, 2, 3 };

    GLuint snakeVAO, snakeVBO, snakeEBO;
    glGenVertexArrays(1, &snakeVAO);
    glGenBuffers(1, &snakeVBO);
    glGenBuffers(1, &snakeEBO);

    glBindVertexArray(snakeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, snakeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(blockVertices), blockVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, snakeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(blockIndices), blockIndices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // --- UNIFORMS ---
    GLint uColorLoc = glGetUniformLocation(shaderProgram, "uColor");
    GLint movePosLoc = glGetUniformLocation(shaderProgram, "movePos");

    glfwSetKeyCallback(window, key_callback);

    // --- MAIN LOOP ---
    while(!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();

        // Clear screen
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if(!isPaused) {
            // --- LOGIC ---
            if (currentTime - lastTime > moveDelay && !gameOver) {
                lastTime = currentTime;

                Point newHead = { snakeBody[0].x + snakeDir.x, snakeBody[0].y + snakeDir.y };

                if (newHead.x < 0 || newHead.x >= COLS || newHead.y < 0 || newHead.y >= ROWS && isCollideBody(newHead)) {
                    gameOver = true;
                    std::cout << "Game Over!" << std::endl;
                } else {
                    if(newHead == apple) {
                        snakeBody.push_back(apple);
                        generateFood();
                    }

                    snakeBody.insert(snakeBody.begin(), newHead);
                    snakeBody.pop_back();
                }
            }
        }

        // --- DRAWING ---
        glUseProgram(shaderProgram);

        // 1. Map
        glUniform3f(movePosLoc, 0.0f, 0.0f, 0.0f);
        glUniform4f(uColorLoc, 1.0f, 1.0f, 1.0f, 1.0f); // White multiplier (preserves vertex colors)
        glBindVertexArray(mapVAO);
        glDrawElements(GL_TRIANGLES, mapIndices.size(), GL_UNSIGNED_INT, 0);

        // 2. Snake
        glUniform4f(uColorLoc, 0.0f, 1.0f, 0.0f, 1.0f); // Green
        glBindVertexArray(snakeVAO);

        for (const auto& segment : snakeBody) {
            float xPos = -1.0f + (segment.x * WIDTH);
            float yPos = -1.0f + (segment.y * HEIGHT);

            glUniform3f(movePosLoc, xPos, yPos, 0.0f);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

        // 3. Draw Apple
        glUniform4f(uColorLoc, 1.0f, 0.0f, 0.0f, 1.0f); // Red color
        glBindVertexArray(snakeVAO); // Reuse the same shape (square)

        float ax = -1.0f + (apple.x * WIDTH);
        float ay = -1.0f + (apple.y * HEIGHT);

        glUniform3f(movePosLoc, ax, ay, 0.0f);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        // Important: Swap buffers and handle events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
