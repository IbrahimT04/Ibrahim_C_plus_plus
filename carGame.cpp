
#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include <chrono>
#include <conio.h>

#include <stdio.h>
#include <Windows.h>
#include <cmath>
#include <thread>

using namespace std;

int nScreenWidth = 120;			// Console Screen Size X (columns)
int nScreenHeight = 40;			// Console Screen Size Y (rows)
int nMapWidth = 16;				// World Dimensions
int nMapHeight = 16;

float fPlayerX = 2.00f;			// Player Start Position
float fPlayerY = 8.00f;
float fPlayerA = 0.0f;			// Player Start Rotation
float fFOV = 3.14159f / 4.0f;	// Field of View
float fDepth = 18.0f;			// Maximum rendering distance
float fSpeed = 0.0f;			// Walking Speed
string message = "Forward";
bool goForward = true;





int main()
{
    string printer;
    // Create Screen Buffer
    wchar_t *screen = new wchar_t[nScreenWidth*nScreenHeight];
    HANDLE hConsole = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
    SetConsoleActiveScreenBuffer(hConsole);
    DWORD dwBytesWritten = 0;

    // Create Map of world space # = wall block, . = space
    wstring map;
    map += L"################";
    map += L"#--------------#";
    map += L"#-............-#";
    map += L"#-.----------.-#";
    map += L"#-.-########-.-#";
    map += L"#-.-#......#-.-#";
    map += L"#-.-#......#-.-#";
    map += L"#-.-#......#-.-#";
    map += L"#-.-#......#-.-#";
    map += L"#-.-#......#-.-#";
    map += L"#-.-#......#-.-#";
    map += L"#-.-########-.-#";
    map += L"#-.----------.-#";
    map += L"#-............-#";
    map += L"#--------------#";
    map += L"################";

    auto tp1 = chrono::system_clock::now();
    auto tp2 = chrono::system_clock::now();

    while (1)
    {
        /*
        if (fSpeed < 0.0f && goForward == true){
            goForward = false;
            message = "Reverse";
        }
        else if (fSpeed < 0.0f)
         */
        // We'll need time differential per frame to calculate modification
        // to movement speeds, to ensure consistent movement, as ray-tracing
        // is non-deterministic
        tp2 = chrono::system_clock::now();
        chrono::duration<float> elapsedTime = tp2 - tp1;
        tp1 = tp2;
        float fElapsedTime = elapsedTime.count();


        // Handle CCW Rotation
        if (GetAsyncKeyState((unsigned short)'A') & 0x8000)
            fPlayerA -= (5.0f * 0.25f) * fElapsedTime;

        // Handle CW Rotation
        if (GetAsyncKeyState((unsigned short)'D') & 0x8000)
            fPlayerA += (5.0f * 0.25f) * fElapsedTime;

        // Handle Forwards movement & collision
        if (GetAsyncKeyState((unsigned short)'W') & 0x8000)
        {
            if (fSpeed < 5.0f) fSpeed += 0.005f;
        }

        // Handle backwards movement & collision
        if (GetAsyncKeyState((unsigned short)'S') & 0x8000)
        {
            if (fSpeed > -0.7f)fSpeed -= 0.01f;
        }
        fPlayerX += sinf(fPlayerA) * fSpeed * fElapsedTime;
        fPlayerY += cosf(fPlayerA) * fSpeed * fElapsedTime;
        if (map.c_str()[(int)fPlayerX * nMapWidth + (int)fPlayerY] == '#')
        {
            fPlayerX -= sinf(fPlayerA) * fSpeed * fElapsedTime;
            fPlayerY -= cosf(fPlayerA) * fSpeed * fElapsedTime;
        }
        for (int x = 0; x < nScreenWidth; x++)
        {
            // For each column, calculate the projected ray angle into world space
            float fRayAngle = (fPlayerA - fFOV/2.0f) + ((float)x / (float)nScreenWidth) * fFOV;

            // Find distance to wall
            float fStepSize = 0.1f;		  // Increment size for ray casting, decrease to increase
            float fDistanceToWall = 0.0f; //                                      resolution
            bool bHitSideFloor = false;
            bool bHitWall = false;		// Set when ray hits wall block
            bool bBoundary = false;		// Set when ray hits boundary between two wall blocks

            float fEyeX = sinf(fRayAngle); // Unit vector for ray in player space
            float fEyeY = cosf(fRayAngle);

            // Incrementally cast ray from player, along ray angle, testing for
            // intersection with a block
            while (!bHitWall && fDistanceToWall < fDepth)
            {
                fDistanceToWall += fStepSize;
                int nTestX = (int)(fPlayerX + fEyeX * fDistanceToWall);
                int nTestY = (int)(fPlayerY + fEyeY * fDistanceToWall);

                // Test if ray is out of bounds
                if (nTestX < 0 || nTestX >= nMapWidth || nTestY < 0 || nTestY >= nMapHeight)
                {
                    bHitWall = true;			// Just set distance to maximum depth
                    fDistanceToWall = fDepth;
                }
                else
                {
                    if (map.c_str()[nTestX * nMapWidth + nTestY] == '-' && !bHitSideFloor){
                        bHitSideFloor = true;
                        float fDistanceToSFloor = fDistanceToWall;
                    }
                    // Ray is inbounds so test to see if the ray cell is a wall block
                    if (map.c_str()[nTestX * nMapWidth + nTestY] == '#')
                    {
                        // Ray has hit wall
                        bHitWall = true;

                        // To highlight tile boundaries, cast a ray from each corner
                        // of the tile, to the player. The more coincident this ray
                        // is to the rendering ray, the closer we are to a tile
                        // boundary, which we'll shade to add detail to the walls
                        vector<pair<float, float>> p;

                        // Test each corner of hit tile, storing the distance from
                        // the player, and the calculated dot product of the two rays
                        for (int tx = 0; tx < 2; tx++)
                            for (int ty = 0; ty < 2; ty++)
                            {
                                // Angle of corner to eye
                                float vy = (float)nTestY + ty - fPlayerY;
                                float vx = (float)nTestX + tx - fPlayerX;
                                float d = sqrt(vx*vx + vy*vy);
                                float dot = (fEyeX * vx / d) + (fEyeY * vy / d);
                                p.push_back(make_pair(d, dot));
                            }

                        // Sort Pairs from closest to farthest
                        sort(p.begin(), p.end(), [](const pair<float, float> &left, const pair<float, float> &right) {return left.first < right.first; });

                        // First two/three are closest (we will never see all four)
                        float fBound = 0.01;
                        if (acos(p.at(0).second) < fBound) bBoundary = true;
                        if (acos(p.at(1).second) < fBound) bBoundary = true;
                        if (acos(p.at(2).second) < fBound) bBoundary = true;
                    }
                }
            }

            // Calculate distance to ceiling and floor
            int nCeiling = (float)(nScreenHeight/2.0);
            int nFloor = nScreenHeight - ((float)(nScreenHeight/2.0) - nScreenHeight / ((float)fDistanceToWall));

            // Shader walls based on distance
            short nShade = ' ';


            if (fDistanceToWall <= fDepth / 4.0f)			nShade = 0x2588;	// Very close
            else if (fDistanceToWall < fDepth / 3.0f)		nShade = 0x2593;
            else if (fDistanceToWall < fDepth / 2.0f)		nShade = 0x2592;
            else if (fDistanceToWall < fDepth)				nShade = 0x2591;
            else											nShade = ' ';		// Too far away

            if (bBoundary)		nShade = '|'; // Black it out

            for (int y = 0; y < nScreenHeight; y++)
            {
                //float b = 1.0f - (((float)y - float(nScreenHeight)/2.0f) / ((float)nScreenHeight / 2.0f));
                //float formula = fDepth*b/fDistanceToWall;


                // Each Row
                if(y <= nCeiling)
                    screen[y*nScreenWidth + x] = ' ';

                else if(y > nCeiling && y <= nFloor){
                    screen[y*nScreenWidth + x] = nShade;
                }
                else // Floor
                {
                    // Shade floor based on distance
                    float b = 1.0f - (((float)y - float(nScreenHeight)/2.0f) / ((float)nScreenHeight / 2.0f));
                    if (b < 0.25)		nShade = '#';
                    else if (b < 0.4)	nShade = 'X';
                    else if (b < 0.55)	nShade = 'x';
                    else if (b < 0.75)	nShade = '-';
                    else if (b < 0.9)	nShade = '.';
                    else				nShade = ' ';

                    screen[y*nScreenWidth + x] = nShade;
                }
            }
        }

        // Display Stats
        swprintf_s(screen, 40, L"X=%3.2f, Y=%3.2f, A=%3.2f FPS=%3.2f ", fPlayerX, fPlayerY, fPlayerA, 1.0f/fElapsedTime);

        // Display Map
        for (int nx = 0; nx < nMapWidth; nx++)
            for (int ny = 0; ny < nMapWidth; ny++)
            {
                screen[(ny+1)*nScreenWidth + nx] = map[ny * nMapWidth + nx];
            }
        screen[((int)fPlayerX+1) * nScreenWidth + (int)fPlayerY] = 'P';

        // Display Framed
        screen[nScreenWidth * nScreenHeight - 1] = '\0';
        WriteConsoleOutputCharacterW(hConsole, screen, nScreenWidth * nScreenHeight, { 2,0 }, &dwBytesWritten);
        fSpeed *= 0.999;
    }
}
