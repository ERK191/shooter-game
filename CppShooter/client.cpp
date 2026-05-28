#ifndef UNICODE
#define UNICODE
#endif
#include <winsock2.h>
#include <windows.h>
#include <gl/gl.h> 
#include <iostream>
#include <string>
#include <vector>
#include <cmath>

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "opengl32.lib") 

enum EngineState { M_LOGIN, M_LOBBY, M_GAMEPLAY, M_ESCMENU };
EngineState gameState = M_LOGIN;
enum WeaponType { W_RIFLE, W_SNIPER, W_BOT_GUN };
WeaponType activeWeapon = W_RIFLE;

std::string loggedInUser = "ErickShooter"; 
float camX = 0.0f, camY = 0.0f, camZ = 5.0f, rotX = 0.0f, rotY = 0.0f;
float mouseSensitivity = 0.12f; float playerSpeed = 0.08f; 

float velY = 0.0f; bool isGrounded = true; float gravity = -0.008f, jumpForce = 0.22f;
bool isAiming = false; float fovMultiplier = 1.0f;
int myHealth = 150; int mySpawnImmunity = 180; 

struct GLBullet { float x, y, z, dx, dy, dz; int life; WeaponType type; bool firedByBot; };
std::vector<GLBullet> activeBullets;

struct EnemyBot { float x, z; float r, g, b; float angle; int health; bool isAlive; int respawnTimer; std::string name; int shootCooldown; int immunity; };
std::vector<EnemyBot> bots;

int selectedCamo = 0; int selectedAttachment = 0; int myFireCooldown = 0;

std::string SendServerPacket(std::string packetData) {
    WSADATA wsa; WSAStartup(MAKEWORD(2,2), &wsa);
    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serv; serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr("127.0.0.1"); serv.sin_port = htons(50001);
    std::string response = "ERROR";
    if (connect(s, (SOCKADDR*)&serv, sizeof(serv)) != SOCKET_ERROR) {
        send(s, packetData.c_str(), (int)packetData.size() + 1, 0);
        char buf[1024] = {0}; recv(s, buf, 1024, 0); response = std::string(buf);
    }
    closesocket(s); WSACleanup();
    return response;
}
void Draw3DBlock(float x, float y, float z, float w, float h, float d, float r, float g, float b) {
    glBegin(GL_QUADS); glColor3f(r, g, b);
    glVertex3f(x-w, y-h, z+d); glVertex3f(x+w, y-h, z+d); glVertex3f(x+w, y+h, z+d); glVertex3f(x-w, y+h, z+d);
    glColor3f(r*0.7f, g*0.7f, b*0.7f);
    glVertex3f(x-w, y-h, z-d); glVertex3f(x-w, y+h, z-d); glVertex3f(x+w, y+h, z-d); glVertex3f(x+w, y-h, z-d);
    glColor3f(r*0.9f, g*0.9f, b*0.9f);
    glVertex3f(x-w, y+h, z-d); glVertex3f(x-w, y+h, z+d); glVertex3f(x+w, y+h, z+d); glVertex3f(x+w, y+h, z-d);
    glVertex3f(x-w, y-h, z-d); glVertex3f(x+w, y-h, z-d); glVertex3f(x+w, y-h, z+d); glVertex3f(x-w, y-h, z-d);
    glEnd();
}

void DrawSoldierWithHP(float x, float y, float z, float r, float g, float b, int hp) {
    Draw3DBlock(x, y + 0.9f, z, 0.15f, 0.15f, 0.15f, 0.88f, 0.71f, 0.56f);
    Draw3DBlock(x, y + 0.4f, z, 0.25f, 0.35f, 0.18f, r, g, b);             
    Draw3DBlock(x, y - 0.3f, z, 0.22f, 0.35f, 0.16f, 0.2f, 0.2f, 0.2f);    
    Draw3DBlock(x + 0.35f, y + 0.4f, z, 0.08f, 0.25f, 0.08f, r, g, b);     
    Draw3DBlock(x - 0.35f, y + 0.4f, z, 0.08f, 0.25f, 0.08f, r, g, b);     
    float hpPercent = (float)hp / 100.0f; if (hpPercent < 0.0f) hpPercent = 0.0f;
    Draw3DBlock(x, y + 1.3f, z, 0.4f, 0.03f, 0.01f, 0.5f, 0.0f, 0.0f);
    Draw3DBlock(x - 0.4f + (0.4f * hpPercent), y + 1.3f, z, 0.4f * hpPercent, 0.03f, 0.011f, 0.0f, 1.0f, 0.0f);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_LBUTTONDOWN: {
            if (gameState == M_LOGIN) { // Click pe ecran ca să treci de Login (fără butoane Win32 defecte)
                SendServerPacket("LOGIN,ErickShooter,pass123");
                gameState = M_GAMEPLAY; mySpawnImmunity = 180; bots.clear();
                for(int bIdx=0; bIdx<25; bIdx++) {
                    bots.push_back({(float)(rand()%240 - 120), (float)(rand()%240 - 120), 0.15f, 0.3f, 0.15f, 0.0f, 100, true, 0, "Bot", 0, 180});
                }
                SetCursorPos(525, 425); ShowCursor(FALSE);
            }
            else if (gameState == M_GAMEPLAY && myFireCooldown <= 0) {
                float rx = rotX * 0.01745f, ry = rotY * 0.01745f;
                GLBullet b = {camX, camY - 0.2f, camZ, sinf(rx) * cosf(ry), -sinf(ry), -cosf(rx) * cosf(ry), 80, activeWeapon, false}; activeBullets.push_back(b);
                myFireCooldown = (activeWeapon == W_RIFLE) ? 12 : 45;
            }
            else if (gameState == M_ESCMENU) { // Click în meniu schimbă skin-ul / atașamentul
                POINT pt; GetCursorPos(&pt); ScreenToClient(hwnd, &pt);
                if (pt.y > 200 && pt.y < 250) selectedCamo = (selectedCamo + 1) % 4;
                if (pt.y > 300 && pt.y < 350) selectedAttachment = (selectedAttachment + 1) % 3;
                if (pt.y > 450) { gameState = M_GAMEPLAY; ShowCursor(FALSE); }
            }
            return 0;
        }
        case WM_RBUTTONDOWN: { if (gameState == M_GAMEPLAY) { isAiming = true; fovMultiplier = 0.4f; } return 0; }
        case WM_RBUTTONUP:   { if (gameState == M_GAMEPLAY) { isAiming = false; fovMultiplier = 1.0f; } return 0; }
        case WM_ERASEBKGND:  return 1;
        case WM_DESTROY:     { ShowCursor(TRUE); PostQuitMessage(0); return 0; }
    } return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow) {
    WNDCLASS wc = {}; wc.lpfnWndProc = WindowProc; wc.hInstance = hInst; wc.lpszClassName = L"PureGLClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW); wc.style = CS_OWNDC;
    RegisterClass(&wc);
    
    HWND hwnd = CreateWindowEx(0, L"PureGLClass", L"Premium Shooter Arena 3D", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 100, 100, 850, 650, NULL, NULL, hInst, NULL);
    HDC hdc = GetDC(hwnd); PIXELFORMATDESCRIPTOR pfd = { sizeof(pfd), 1, PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, PFD_TYPE_RGBA, 32 };
    SetPixelFormat(hdc, ChoosePixelFormat(hdc, &pfd), &pfd); wglMakeCurrent(hdc, wglCreateContext(hdc));
    glEnable(GL_DEPTH_TEST); MSG msg = {};

    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) { TranslateMessage(&msg); DispatchMessage(&msg); }
        else {
            glViewport(0, 0, 850, 650);
            if (gameState == M_LOGIN) { // --- INTERFAȚĂ LOGIN COMPLET OPENGL (Adio ecran alb!) ---
                glClearColor(0.1f, 0.15f, 0.2f, 1.0f); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(-1, 1, -1, 1, -1, 1);
                glMatrixMode(GL_MODELVIEW); glLoadIdentity(); glDisable(GL_DEPTH_TEST);
                Draw3DBlock(0.0f, 0.0f, 0.0f, 0.4f, 0.15f, 0.01f, 0.2f, 0.6f, 0.3f); // Buton mare verde de Login
                glEnable(GL_DEPTH_TEST); SwapBuffers(hdc);
            }
            else if (gameState == M_GAMEPLAY || gameState == M_ESCMENU) {
                glClearColor(0.4f, 0.6f, 0.9f, 1.0f); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                glMatrixMode(GL_PROJECTION); glLoadIdentity();
                float aspect = 850.0f / 650.0f; glFrustum(-aspect * 0.1f * fovMultiplier, aspect * 0.1f * fovMultiplier, -0.1f * fovMultiplier, 0.1f * fovMultiplier, 0.1f, 400.0f);
                glMatrixMode(GL_MODELVIEW); glLoadIdentity();

                if (gameState == M_GAMEPLAY) {
                    POINT mp; GetCursorPos(&mp); rotX += (mp.x - 525) * mouseSensitivity; rotY += (mp.y - 425) * mouseSensitivity;
                    if (rotY > 85.0f) rotY = 85.0f; if (rotY < -85.0f) rotY = -85.0f; SetCursorPos(525, 425);
                    float rx = rotX * 0.01745f;
                    if (GetAsyncKeyState('W') & 0x8000) { camX += sinf(rx) * playerSpeed; camZ -= cosf(rx) * playerSpeed; }
                    if (GetAsyncKeyState('S') & 0x8000) { camX -= sinf(rx) * playerSpeed; camZ += cosf(rx) * playerSpeed; }
                    if (GetAsyncKeyState('A') & 0x8000) { camX -= cosf(rx) * playerSpeed; camZ -= sinf(rx) * playerSpeed; }
                    if (GetAsyncKeyState('D') & 0x8000) { camX += cosf(rx) * playerSpeed; camZ += sinf(rx) * playerSpeed; }
                    if (GetAsyncKeyState('1') & 0x8000) activeWeapon = W_RIFLE;
                    if (GetAsyncKeyState('2') & 0x8000) activeWeapon = W_SNIPER;

                    if (myFireCooldown > 0) myFireCooldown--;
                    if (mySpawnImmunity > 0) mySpawnImmunity--;

                    if (camX > 140.0f) camX = 140.0f; if (camX < -140.0f) camX = -140.0f;
                    if (camZ > 140.0f) camZ = 140.0f; if (camZ < -140.0f) camZ = -140.0f;

                    if (loggedInUser == "ErickShooter") {
                        if (GetAsyncKeyState(VK_SHIFT) & 0x8000) camY += 0.08f; if (GetAsyncKeyState(VK_CONTROL) & 0x8000) camY -= 0.08f;
                        velY = 0.0f; isGrounded = true;
                    } else {
                        if ((GetAsyncKeyState(VK_SPACE) & 0x8000) && isGrounded) { velY = jumpForce; isGrounded = false; }
                        camY += velY; velY += gravity; if (camY <= 0.0f) { camY = 0.0f; velY = 0.0f; isGrounded = true; }
                    }
                    if (camY < -10.0f || myHealth <= 0) { camX = 0.0f; camY = 0.0f; camZ = 5.0f; velY = 0.0f; isGrounded = true; myHealth = 150; mySpawnImmunity = 180; }
                    if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) { gameState = M_ESCMENU; ShowCursor(TRUE); Sleep(200); }
                }
                glRotatef(rotY, 1.0f, 0.0f, 0.0f); glRotatef(rotX, 0.0f, 1.0f, 0.0f); glTranslatef(-camX, -camY - 0.5f, -camZ);
                
                Draw3DBlock(0.0f, 120.0f, 0.0f, 300.0f, 1.0f, 300.0f, 0.6f, 0.8f, 1.0f); 
                Draw3DBlock(40.0f, 80.0f, -100.0f, 8.0f, 8.0f, 1.0f, 1.0f, 0.95f, 0.6f); 

                for(int i = -150; i <= 150; i += 10) {
                    for(int j = -150; j <= 150; j += 10) {
                        float grassPattern = ((i + j) % 20 == 0) ? 0.35f : 0.45f;
                        Draw3DBlock((float)i, -1.0f, (float)j, 4.8f, 0.02f, 4.8f, 0.1f, grassPattern, 0.15f); 
                    }
                }
                Draw3DBlock(-20.0f, 5.0f, -30.0f, 4.0f, 6.0f, 4.0f, 0.5f, 0.5f, 0.5f); Draw3DBlock(30.0f, 3.0f, -20.0f, 6.0f, 4.0f, 6.0f, 0.4f, 0.4f, 0.4f);
                Draw3DBlock(0.0f, 2.0f, -60.0f, 35.0f, 3.0f, 2.0f, 0.3f, 0.3f, 0.35f); Draw3DBlock(-45.0f, 6.0f, 20.0f, 6.0f, 7.0f, 6.0f, 0.45f, 0.45f, 0.5f);

                for (auto& b : bots) {
                    if (!b.isAlive) {
                        b.respawnTimer++; if (b.respawnTimer > 150) { b.x = (float)(rand()%200 - 100); b.z = (float)(rand()%200 - 100); b.health = 100; b.isAlive = true; b.respawnTimer = 0; b.immunity = 180; }
                        continue;
                    }
                    if (b.immunity > 0) b.immunity--; 
                    float vX = camX - b.x; float vZ = camZ - b.z; float dist = sqrtf(vX*vX + vZ*vZ);
                    if (dist < 35.0f && gameState == M_GAMEPLAY) { 
                        b.angle = atan2f(vX, vZ); b.shootCooldown++;
                        if (b.shootCooldown > 65) { 
                            GLBullet botBullet = {b.x, 0.2f, b.z, vX/dist * 0.35f, 0.0f, vZ/dist * 0.35f, 90, W_BOT_GUN, true};
                            activeBullets.push_back(botBullet); b.shootCooldown = 0;
                        }
                    } else { b.angle += 0.012f; b.x += sinf(b.angle) * 0.02f; }
                    DrawSoldierWithHP(b.x, 0.0f, b.z, b.r, b.g, b.b, b.health); 
                }

                for (auto it = activeBullets.begin(); it != activeBullets.end();) {
                    it->x += it->dx; it->y += it->dy; it->z += it->dz; it->life--;
                    if (it->firedByBot) Draw3DBlock(it->x, it->y, it->z, 0.05f, 0.05f, 0.05f, 1.0f, 0.2f, 0.2f);
                    else Draw3DBlock(it->x, it->y, it->z, 0.04f, 0.04f, 0.04f, 1.0f, 0.9f, 0.2f);
                    bool bulletSpent = false;
                    if (it->firedByBot) {
                        if (mySpawnImmunity <= 0 && abs(it->x - camX) < 0.7f && abs(it->z - camZ) < 0.7f) { myHealth -= 8; bulletSpent = true; }
                    } else {
                        for(auto& b : bots) {
                            if (b.isAlive && b.immunity <= 0 && abs(it->x - b.x) < 0.6f && abs(it->z - b.z) < 0.6f) {
                                int dmg = (it->type == W_RIFLE) ? ((it->y > 0.6f) ? 30 : 10) : ((it->y > 0.6f) ? 300 : 100); b.health -= dmg;
                                if (b.health <= 0) { b.isAlive = false; b.respawnTimer = 0; }
                                bulletSpent = true; break;
                            }
                        }
                    }
                    if (it->life <= 0 || bulletSpent) it = activeBullets.erase(it); else ++it;
                }

                glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(-1, 1, -1, 1, -1, 1);
                glMatrixMode(GL_MODELVIEW); glLoadIdentity(); glDisable(GL_DEPTH_TEST);
                
                if (gameState == M_ESCMENU) { // --- MENIU ESC REDESENAT ÎN OPENGL (Nu mai dispare!) ---
                    Draw3DBlock(0.0f, 0.3f, 0.0f, 0.5f, 0.08f, 0.01f, 0.1f, 0.1f, 0.15f); // Click linie Camo
                    Draw3DBlock(0.0f, -0.1f, 0.0f, 0.5f, 0.08f, 0.01f, 0.1f, 0.1f, 0.15f); // Click linie Attachment
                    Draw3DBlock(0.0f, -0.6f, 0.0f, 0.5f, 0.08f, 0.01f, 0.1f, 0.5f, 0.2f); // Click buton Resume verde
                }

                float myHpPercent = (float)myHealth / 150.0f;
                Draw3DBlock(-0.8f, -0.8f, 0.0f, 0.15f, 0.02f, 0.01f, 0.2f, 0.2f, 0.2f);
                Draw3DBlock(-0.95f + (0.15f * myHpPercent), -0.8f, 0.0f, 0.15f * myHpPercent, 0.02f, 0.011f, 0.0f, 1.0f, 0.3f);

                float weaponLength = (activeWeapon == W_RIFLE) ? 0.4f : 0.65f;
                float gunX = isAiming ? 0.0f : 0.35f; float gunY = isAiming ? -0.25f : -0.45f;
                
                float cr = 0.4f, cg = 0.4f, cb = 0.42f; 
                if (selectedCamo == 1) { cr = 1.0f; cg = 0.85f; cb = 0.0f; } 
                if (selectedCamo == 2) { cr = 0.9f; cg = 0.2f;  cb = 0.1f; } 
                if (selectedCamo == 3) { cr = 0.1f; cg = 0.6f;  cb = 1.0f; } 
                
                Draw3DBlock(gunX, gunY, 0.0f, 0.08f, 0.08f, weaponLength, cr, cg, cb); 
                if (selectedAttachment == 1) Draw3DBlock(gunX, gunY, 0.0f, 0.04f, 0.04f, weaponLength + 0.2f, 0.1f, 0.1f, 0.1f); 
                if (selectedAttachment == 2) Draw3DBlock(gunX, gunY + 0.12f, 0.0f, 0.03f, 0.03f, 0.05f, 1.0f, 0.0f, 0.0f); 
                
                glBegin(GL_LINES); glColor3f(1.0f, 0.0f, 0.0f); glVertex2f(-0.02f, 0.0f); glVertex2f(0.02f, 0.0f); glVertex2f(0.0f, -0.02f); glVertex2f(0.0f, 0.02f); glEnd();
                glEnable(GL_DEPTH_TEST);
            }
            SwapBuffers(hdc); Sleep(15);
        }
    } return 0;
}
