#include "pch.h"
#include <vector>
#include <cmath>
#include <random>

float cameraAngleX = 30.0f; // gora dol
float cameraAngleY = -45.0f; // prawo lewo
float cameraZoom = 40.0f;
float gap = 0.01f;

// wczytanie tekstury na tlo
sf::Texture backgroundTexture;
bool textureLoaded = false;

// obsluga myszki
bool isDragging = false;
sf::Vector2i lastMousePos;

// pojedyncza kostka
struct Cubie {
    int x, y, z; // pozycja w siatce
    int ogX, ogY, ogZ; // do sprawdzenia czy wygrana
    float transform[16]; // macierz obrotu

    Cubie(int _x, int _y, int _z) : x(_x), y(_y), z(_z), ogX(_x), ogY(_y), ogZ(_z) {
        for (int i = 0; i < 16; i++) {
            transform[i] = 0.0f;
        }
        transform[0] = transform[5] = transform[10] = transform[15] = 1.0f; // macierz jednostkowa
    }
};

std::vector<Cubie> cubeGrid;
sf::Shader shader;
bool shaderLoaded = false;

// shadery
const std::string vertexShaderSource = R"(
    #version 120
    varying vec3 vNormal;
    varying vec3 vPosition;
    varying vec4 vColor;
    void main() {
        vPosition = vec3(gl_ModelViewMatrix * gl_Vertex);
        vNormal = normalize(gl_NormalMatrix * gl_Normal);
        vColor = gl_Color;
        gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
    }
)";

const std::string fragmentShaderSource = R"(
    #version 120
    varying vec3 vNormal;
    varying vec3 vPosition;
    varying vec4 vColor;
    void main() {
        vec3 lightPos = vec3(5.0, 10.0, 10.0);
        vec3 lightColor = vec3(1.0, 1.0, 1.0);
        float ambientStrength = 0.4;
        vec3 ambient = ambientStrength * lightColor;
        vec3 norm = normalize(vNormal);
        vec3 lightDir = normalize(lightPos - vPosition);
        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor;
        float specularStrength = 0.9;
        vec3 viewDir = normalize(-vPosition);
        vec3 reflectDir = reflect(-lightDir, norm);
        float spec = pow(max(dot(viewDir, reflectDir), 0.0), 64.0);
        vec3 specular = specularStrength * spec * lightColor;
        vec3 result = (ambient + diffuse + specular) * vColor.rgb;
        gl_FragColor = vec4(result, 1.0);
    }
)";


bool isAnimating = false;
bool gameWon = true;
float currentAnimAngle = 0.0f;
float targetAnimAngle = 90.0f;
float animSpeed = 5.0f;

int spinningAxis = 0;
int spinningSlice = 0;
int spinningDir = 1;


void initCubeGrid() {
    cubeGrid.clear();
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            for (int z = -1; z <= 1; z++) {
                cubeGrid.push_back(Cubie(x, y, z));
            }
        }
    }
    gameWon = true;
}

void multiplyMatrix(float* output, const float* m1, const float* m2) {
    float temp[16];
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++) {
                sum += m1[k * 4 + r] * m2[c * 4 + k];
            }
            temp[c * 4 + r] = sum;
        }
    }
    for (int i = 0; i < 16; i++) output[i] = temp[i];
}

void checkIfSolved() {
    for (const auto& c : cubeGrid) {
        if (c.x != c.ogX || c.y != c.ogY || c.z != c.ogZ) {
            gameWon = false; 
            return;
        }
        if (c.transform[0] < 0.9f || c.transform[5] < 0.9f || c.transform[10] < 0.9f) {
            gameWon = false; 
            return;
        }
    }
    gameWon = true;
}

void applyRotationLogic() {
    glPushMatrix();
    glLoadIdentity();
    if (spinningAxis == 0) {
        glRotatef(90.0f * spinningDir, 1, 0, 0);
    }
    else if (spinningAxis == 1) {
        glRotatef(90.0f * spinningDir, 0, 1, 0);
    }
    else glRotatef(90.0f * spinningDir, 0, 0, 1);

    float rotMatrix[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, rotMatrix);
    glPopMatrix();

    for (auto& c : cubeGrid) {
        bool inSlice = false;
        if (spinningAxis == 0 && c.x == spinningSlice) inSlice = true;
        if (spinningAxis == 1 && c.y == spinningSlice) inSlice = true;
        if (spinningAxis == 2 && c.z == spinningSlice) inSlice = true;

        if (inSlice) {
            multiplyMatrix(c.transform, rotMatrix, c.transform);
            for (int i = 0; i < 16; i++) {
                c.transform[i] = round(c.transform[i]);
            }

            int oldX = c.x, oldY = c.y, oldZ = c.z;
            if (spinningAxis == 0) {
                if (spinningDir == 1) { 
                    c.y = -oldZ; 
                    c.z = oldY;
                }
                else { 
                    c.y = oldZ;
                    c.z = -oldY;
                }
            }
            else if (spinningAxis == 1) {
                if (spinningDir == 1) { 
                    c.z = -oldX;
                    c.x = oldZ; 
                }
                else { 
                    c.z = oldX; 
                    c.x = -oldZ; 
                }
            }
            else if (spinningAxis == 2) {
                if (spinningDir == 1) { 
                    c.x = -oldY; 
                    c.y = oldX; 
                }
                else { 
                    c.x = oldY; 
                    c.y = -oldX; 
                }
            }
        }
    }
    checkIfSolved();
}

void startMove(int axis, int slice, int dir) {
    if (isAnimating) return;
    spinningAxis = axis;
    spinningSlice = slice;
    spinningDir = dir;
    currentAnimAngle = 0.0f;
    isAnimating = true;
    gameWon = false;
}

void shuffleCube() {
    if (isAnimating) return;
    for (int i = 0; i < 20; i++) {
        spinningAxis = rand() % 3;
        spinningSlice = (rand() % 3) - 1;
        spinningDir = (rand() % 2 == 0) ? 1 : -1;
        applyRotationLogic();
    }
    gameWon = false;
}

// rysowanie
void drawSingleCube(float s) {
    glBegin(GL_QUADS);
    // front
    glNormal3f(0, 0, 1);
    glColor3f(0.9f, 0, 0);
    glVertex3f(-s, -s, s); 
    glVertex3f(s, -s, s);
    glVertex3f(s, s, s);
    glVertex3f(-s, s, s);
    // back 
    glNormal3f(0, 0, -1);
    glColor3f(1.0f, 0.5f, 0);
    glVertex3f(-s, s, -s);
    glVertex3f(s, s, -s);
    glVertex3f(s, -s, -s);
    glVertex3f(-s, -s, -s);
    // up
    glNormal3f(0, 1, 0); 
    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex3f(-s, s, -s);
    glVertex3f(-s, s, s);
    glVertex3f(s, s, s);
    glVertex3f(s, s, -s);
    // down
    glNormal3f(0, -1, 0);
    glColor3f(1.0f, 1.0f, 0);
    glVertex3f(-s, -s, s);
    glVertex3f(-s, -s, -s); 
    glVertex3f(s, -s, -s);
    glVertex3f(s, -s, s);
    // right
    glNormal3f(1, 0, 0);
    glColor3f(0, 0, 1.0f);
    glVertex3f(s, -s, s);
    glVertex3f(s, -s, -s);
    glVertex3f(s, s, -s);
    glVertex3f(s, s, s);
    // left
    glNormal3f(-1, 0, 0);
    glColor3f(0, 1.0f, 0);
    glVertex3f(-s, -s, -s);
    glVertex3f(-s, -s, s);
    glVertex3f(-s, s, s);
    glVertex3f(-s, s, -s);

    // srodek
    glColor3f(0.1f, 0.1f, 0.1f);
    glEnd();
}

void drawBackground() {
    if (!textureLoaded) return;
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    glDisable(GL_DEPTH_TEST); // zeby tlo bylo z tylu
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, 1, 0, 1, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    sf::Texture::bind(&backgroundTexture);
    glColor3f(1.0f, 1.0f, 1.0f);

    glBegin(GL_QUADS);
    glTexCoord2f(0, 1); glVertex2f(0, 0);
    glTexCoord2f(1, 1); glVertex2f(1, 0);
    glTexCoord2f(1, 0); glVertex2f(1, 1);
    glTexCoord2f(0, 0); glVertex2f(0, 1);
    glEnd();

    sf::Texture::bind(NULL);

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glPopAttrib();
}

void drawScene() {
    glLoadIdentity();
    float physicalDistance = 55.0f - cameraZoom;
    glTranslatef(0.0f, 0.0f, -physicalDistance);
    glRotatef(cameraAngleX, 1.0f, 0.0f, 0.0f);
    glRotatef(cameraAngleY, 0.0f, 1.0f, 0.0f);

    if (shaderLoaded) sf::Shader::bind(&shader);

    for (auto& cubie : cubeGrid) {
        glPushMatrix();
        bool isMoving = false;
        if (isAnimating) {
            if (spinningAxis == 0 && cubie.x == spinningSlice) isMoving = true;
            if (spinningAxis == 1 && cubie.y == spinningSlice) isMoving = true;
            if (spinningAxis == 2 && cubie.z == spinningSlice) isMoving = true;
        }

        if (isMoving) {
            if (spinningAxis == 0) glRotatef(currentAnimAngle * spinningDir, 1, 0, 0);
            if (spinningAxis == 1) glRotatef(currentAnimAngle * spinningDir, 0, 1, 0);
            if (spinningAxis == 2) glRotatef(currentAnimAngle * spinningDir, 0, 0, 1);
        }

        glMultMatrixf(cubie.transform);
        drawSingleCube(0.48f);
        glPopMatrix();
    }
    if (shaderLoaded) sf::Shader::bind(NULL);
}

int main() {
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.antialiasingLevel = 4;
    sf::RenderWindow window(sf::VideoMode(1024, 768), "Kostka Rubika - Projekt Grafika 3D", sf::Style::Titlebar | sf::Style::Close, settings);
     window.setVerticalSyncEnabled(true);
    ImGui::SFML::Init(window);

    if (backgroundTexture.loadFromFile("background.jpg")) {
        textureLoaded = true;
    }

    initCubeGrid();
    for (auto& c : cubeGrid) {
        glPushMatrix();
        glLoadIdentity();
        glTranslatef(c.x * (1.0f + gap), c.y * (1.0f + gap), c.z * (1.0f + gap));
        glGetFloatv(GL_MODELVIEW_MATRIX, c.transform);
        glPopMatrix();
    }

    glEnable(GL_DEPTH_TEST);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    if (shader.loadFromMemory(vertexShaderSource, fragmentShaderSource)) shaderLoaded = true;

    glViewport(0, 0, 1024, 768);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0, 1024.0 / 768.0, 0.1, 100.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    sf::Clock deltaClock;
    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            ImGui::SFML::ProcessEvent(event);

            if (event.type == sf::Event::Closed) window.close();

            if (event.type == sf::Event::MouseWheelScrolled) {
                if (!ImGui::GetIO().WantCaptureMouse) {
                    cameraZoom += event.mouseWheelScroll.delta * 1.0f;
                    if (cameraZoom < 15.0f) cameraZoom = 15.0f;
                    if (cameraZoom > 50.0f) cameraZoom = 50.0f;
                }
            }

            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                if (!ImGui::GetIO().WantCaptureMouse) {
                    isDragging = true;
                    lastMousePos = sf::Mouse::getPosition(window);
                }
            }
            if (event.type == sf::Event::MouseButtonReleased && event.mouseButton.button == sf::Mouse::Left) {
                isDragging = false;
            }
            if (event.type == sf::Event::MouseMoved && isDragging) {
                sf::Vector2i currentMousePos = sf::Mouse::getPosition(window);
                int dx = currentMousePos.x - lastMousePos.x;
                int dy = currentMousePos.y - lastMousePos.y;

                float angleRad = cameraAngleX * 3.14159f / 180.0f;

                if (cos(angleRad) < 0) {
                    dx = -dx;
                }

                cameraAngleY += dx * 0.5f;
                cameraAngleX += dy * 0.5f;

                lastMousePos = currentMousePos;
            }

            if (event.type == sf::Event::KeyPressed && !isAnimating) {
                int dir = (event.key.shift) ? -1 : 1;

                if (event.key.code == sf::Keyboard::L) startMove(0, -1, -dir);// left
                if (event.key.code == sf::Keyboard::R) startMove(0, 1, dir);// right

                if (event.key.code == sf::Keyboard::U) startMove(1, 1, dir);// up
                if (event.key.code == sf::Keyboard::D) startMove(1, -1, -dir);// down

                if (event.key.code == sf::Keyboard::F) startMove(2, 1, dir);// front
                if (event.key.code == sf::Keyboard::B) startMove(2, -1, -dir);// back

                if (event.key.code == sf::Keyboard::Space) shuffleCube(); // spacja
            }

        }

        if (isAnimating) {
            currentAnimAngle += animSpeed;
            if (currentAnimAngle >= targetAnimAngle) {
                currentAnimAngle = 90.0f;
                applyRotationLogic();
                isAnimating = false;
                currentAnimAngle = 0.0f;
            }
        }

        ImGui::SFML::Update(window, deltaClock.restart());

        ImGui::Begin("Kostka Rubika");

        if (gameWon) {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Gratulacje!");
            if (ImGui::Button("Zagraj ponownie")) shuffleCube();
        }
        else {
            if (ImGui::Button("Mieszaj")) shuffleCube();
        }

        ImGui::Separator();
        ImGui::Text("Sterowanie myszka:");
        ImGui::BulletText("LPM + Ruch: Obrot");
        ImGui::BulletText("Scroll: Zoom");

        ImGui::Separator();
        ImGui::Text("Sterowanie klawiatura:");
        ImGui::BulletText("L, R, U, D, F, B - Obrot scianki");
        ImGui::BulletText("Shift + Klawisz - Obrot przeciwny");
        ImGui::BulletText("Spacja - Mieszaj");

        ImGui::Separator();
        ImGui::Text("Sterowanie gra:");

        if (ImGui::Button("L-")) startMove(0, -1, -1);
        ImGui::SameLine();
        if (ImGui::Button("L+")) startMove(0, -1, 1);
        ImGui::SameLine();
        if (ImGui::Button("R-")) startMove(0, 1, -1);
        ImGui::SameLine();
        if (ImGui::Button("R+")) startMove(0, 1, 1);

        if (ImGui::Button("D-")) startMove(1, -1, -1);
        ImGui::SameLine();
        if (ImGui::Button("D+")) startMove(1, -1, 1);
        ImGui::SameLine();
        if (ImGui::Button("U-")) startMove(1, 1, -1);
        ImGui::SameLine();
        if (ImGui::Button("U+")) startMove(1, 1, 1);

        if (ImGui::Button("B-")) startMove(2, -1, -1);
        ImGui::SameLine();
        if (ImGui::Button("B+")) startMove(2, -1, 1);
        ImGui::SameLine();
        if (ImGui::Button("F-")) startMove(2, 1, -1);
        ImGui::SameLine();
        if (ImGui::Button("F+")) startMove(2, 1, 1);

        ImGui::Separator();
        ImGui::Text("Ustawienia kamery:");
        ImGui::SliderFloat("Zoom", &cameraZoom, 15, 50);
        ImGui::SliderFloat("Angle X", &cameraAngleX, -180, 180);
        ImGui::SliderFloat("Angle Y", &cameraAngleY, -180, 180);

        ImGui::End();

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		drawBackground();
        drawScene();

        window.pushGLStates();
        ImGui::SFML::Render(window);
        window.popGLStates();
        window.display();
    }
    ImGui::SFML::Shutdown();
    return 0;
}