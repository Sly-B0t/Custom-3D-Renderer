// main.cpp
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <cstdint>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <cmath>
#include <windowsx.h>
static const int WIDTH  = 800;
static const int HEIGHT = 600;
static const int pixelsize = 10;
#include <string>
int MOUSE_X = 0;
int MOUSE_Y= 0;
float sensitivity = 0.01;
float mdx;
float mdy;
float yaw;
float pitch;
// 32-bit pixels: 0x00RRGGBB (we'll write as RRGGBB and let GDI interpret it)
static uint32_t gPixels[WIDTH * HEIGHT];
static BITMAPINFO gBmi;

struct Vertex{
    float x,y,z;
};

struct Face{
    Vertex v0,v1,v2;
};
struct screenPoint{
    float x,y;
};
struct camPos{
    float x = 0;
    float y = 0;
    float z = 0;
};
camPos cameraPosition;

std::vector<Face> Faces;

screenPoint toScreen(screenPoint src){
    screenPoint sp;
    sp.x = (src.x + 1) / (2 * WIDTH);
    sp.y = (src.y + 1) / (2 * HEIGHT);
    return src;
}
screenPoint projectToPixels(const Vertex& v)
{
    // camera looks down +z, so z must be positive
    if (v.z <= 0.001f) return {-1e9f, -1e9f};

    float focal = 300.0f; // tweak this
    float nx = v.x / v.z;
    float ny = v.y / v.z;

    screenPoint sp;
    sp.x = nx * focal + WIDTH  * 0.5f;
    sp.y = -ny * focal + HEIGHT * 0.5f;
    return sp;
}

float dist(screenPoint sp){
    return sqrt(sp.x * sp.x + sp.y * sp.y);
}

float maxf(float a, float b){
    return (a > b) ? a: b;
}

void drawEdge(screenPoint a, screenPoint b)
{
    float dx = b.x - a.x;
    float dy = b.y - a.y;

    int steps = (int)maxf(std::abs(dx), std::abs(dy));

    if (steps == 0) return;

    float xInc = dx / steps;
    float yInc = dy / steps;

    float x = a.x;
    float y = a.y;

    for (int i = 0; i <= steps; i++)
    {
        int xi = (int)x;
        int yi = (int)y;

        if ((unsigned)xi < WIDTH && (unsigned)yi < HEIGHT)
            gPixels[yi * WIDTH + xi] = 0x0000FF00;

        x += xInc;
        y += yInc;
    }
}

void fillPixel(screenPoint sp,float z)
{
    float size = 35 / z;
    float half = size / 2;

    int cx = (int)sp.x;
    int cy = (int)sp.y;

    for (int j = -half; j <= half; ++j)
    {
        int y = cy + j;
        if ((unsigned)y >= HEIGHT) continue;

        for (int i = -half; i <= half; ++i)
        {
            int x = cx + i;
            if ((unsigned)x >= WIDTH) continue;

            gPixels[y * WIDTH + x] = (255 << 8);
        }
    }
}

void clear(){
    for (int y = 0; y < HEIGHT; ++y)
    {
        for (int x = 0; x < WIDTH; ++x)
        {
            //turn it all black
            uint8_t r = 0;
            uint8_t g = 0;
            uint8_t b = 0;

            gPixels[y * WIDTH + x] = (r << 16) | (g << 8) | (b);
        }
    }
}

// yaw = rotate around Y axis (left/right)
// pitch = rotate around X axis (up/down)
Vertex rotateVertex(const Vertex& v)
{
    float cy = cosf(yaw),  sy = sinf(yaw);
    float cp = cosf(pitch), sp = sinf(pitch);

    // 1) Yaw (Y-axis)
    float x1 = v.x * cy + v.z * sy;
    float y1 = v.y;
    float z1 = -v.x * sy + v.z * cy;

    // 2) Pitch (X-axis)
    Vertex out;
    out.x = x1;
    out.y = y1 * cp - z1 * sp;
    out.z = y1 * sp + z1 * cp;

    return out;
}

screenPoint RenderVertex(Vertex vert)
{
    screenPoint sp = projectToPixels(vert);
    sp.x *= 1;
    sp.y *= 1;
    fillPixel(sp, vert.z); 
    return sp;
}
static void RenderFace()
{
    for(int i =0; i < Faces.size(); i++){
        screenPoint sp0;
        screenPoint sp1;
        screenPoint sp2;
        Vertex rdx0 = rotateVertex(Faces[i].v0);
        Vertex rdx1 = rotateVertex(Faces[i].v1);
        Vertex rdx2 = rotateVertex(Faces[i].v2);
        rdx0.x -= cameraPosition.x;
        rdx0.y -= cameraPosition.y;
        rdx0.z -= cameraPosition.z;
        rdx1.x -= cameraPosition.x;
        rdx1.y -= cameraPosition.y;
        rdx1.z -= cameraPosition.z;
        rdx2.x -= cameraPosition.x;
        rdx2.y -= cameraPosition.y;
        rdx2.z -= cameraPosition.z;
        if(rdx0.z > 0.5){
            sp0 = RenderVertex(rdx0);
        }
        if(rdx1.z > 0.5){
            sp1 = RenderVertex(rdx1);
        }
        if(rdx2.z > 0.5){
            sp2 = RenderVertex(rdx2);
        }
        
        if(rdx0.z > 0.5 && rdx1.z > 0.5 && rdx2.z > 0.5){
            drawEdge(sp0,sp1);
            drawEdge(sp1,sp2);
            drawEdge(sp2,sp0);
        }


    }
    
        
    
}

static void RenderPattern()
{
    
    RenderFace();
    gPixels[ 300 * WIDTH + 400] = (255 << 8);

}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CREATE:
        {
            // Setup the bitmap description for our pixel buffer
            ZeroMemory(&gBmi, sizeof(gBmi));
            gBmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
            gBmi.bmiHeader.biWidth       = WIDTH;
            gBmi.bmiHeader.biHeight      = -HEIGHT; // negative = top-down image
            gBmi.bmiHeader.biPlanes      = 1;
            gBmi.bmiHeader.biBitCount    = 32;
            gBmi.bmiHeader.biCompression = BI_RGB;
            
            Face temp={
                {-1,1,1},
                {1,1,1},
                {1,-1,1}
            };
            Face temp2={
                {-1,-1,1},
                {-1,1,1},
                {1,-1,1}
            };
            Face temp3={
                {-1,1,2},
                {1,1,2},
                {1,-1,2}
            };
            Face temp4={
                {-1,-1,2},
                {-1,1,2},
                {1,-1,2}
            };
            Faces.push_back(temp);
            Faces.push_back(temp2);
            Faces.push_back(temp3);
            Faces.push_back(temp4);
            RenderPattern();
            // Ask Windows to paint once
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            clear();
            RenderPattern();
            HDC hdc = BeginPaint(hwnd, &ps);
            StretchDIBits(
                hdc,
                0, 0, WIDTH, HEIGHT,      // dest rect
                0, 0, WIDTH, HEIGHT,      // src rect
                gPixels,
                &gBmi,
                DIB_RGB_COLORS,
                SRCCOPY
            );

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_KEYDOWN:
        {
            // Press R to re-randomize the pattern
            if (wParam == 'R')
            {
                for(int i = 0; i < Faces.size();i++){
                Faces[i].v0.z += 2;
                Faces[i].v1.z += 2;
                Faces[i].v2.z += 2;
                }
                RenderPattern();
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            if (wParam == 'E')
            {
                for(int i = 0; i < Faces.size();i++){
                Faces[i].v0.z -= 2;
                Faces[i].v1.z -= 2;
                Faces[i].v2.z -= 2;
                }
                RenderPattern();
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            if (wParam == 'W')
            {
                cameraPosition.x +=0.1;
                RenderPattern();
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            if (wParam == 'S')
            {
                cameraPosition.x -=0.1;
                RenderPattern();
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            if (wParam == 'D')
            {
                cameraPosition.y +=0.1;
                RenderPattern();
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            if (wParam == 'A')
            {
                cameraPosition.y -=0.1;
                RenderPattern();
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            if (wParam == VK_SPACE)
            {
                cameraPosition.z +=0.1;
                RenderPattern();
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            if (wParam == VK_SHIFT)
            {
                cameraPosition.z -=0.1;
                RenderPattern();
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;
        }
        case WM_MOUSEMOVE:
        {
            mdx = GET_X_LPARAM(lParam) - MOUSE_X;
            mdy = GET_Y_LPARAM(lParam) - MOUSE_Y;
            yaw   += mdx * sensitivity;
            pitch += mdy * sensitivity;
            MOUSE_X = GET_X_LPARAM(lParam);
            MOUSE_Y = GET_Y_LPARAM(lParam);
            // x and y are in client-area pixel coordinates
            // top-left = (0,0)

            InvalidateRect(hwnd, nullptr, FALSE);

            return 0;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    const wchar_t CLASS_NAME[] = L"PixelWindowClass";

    WNDCLASSW wc = {};
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);

    if (!RegisterClassW(&wc))
        return 0;

    // Create a window sized roughly to fit the buffer (note: client area != window size exactly)
    HWND hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"Pixel Buffer Window (software rendering)",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WIDTH + 16, HEIGHT + 39,   // quick-and-dirty sizing for borders/titlebar
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!hwnd)
        return 0;

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}