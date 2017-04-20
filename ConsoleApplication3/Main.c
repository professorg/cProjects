#include <stdio.h>
#include <stdlib.h>
#include "SDL.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

#define POINT_SIZE 4
#define QUALITY 1000

void bezier(SDL_Renderer*, int, int*, int);
int triangle(int);
double distance(int, int, int, int);

int main(int argc, char* argv[])
{

    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Init(SDL_INIT_VIDEO);

    SDL_CreateWindowAndRenderer
    (
        WINDOW_WIDTH, WINDOW_HEIGHT, 0, &window, &renderer
    );

    if (window == NULL)
    {
        printf("Could not create window: %s\n", SDL_GetError());
        return 1;
    }
    if (renderer == NULL)
    {
        printf("Could not create renderer: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Surface* screen = SDL_GetWindowSurface(window);

    int num = 0;
    int selectedPoint = -1;
    int* points = NULL;

    SDL_bool done = SDL_FALSE;
    SDL_Rect rect;
    rect.w = POINT_SIZE;
    rect.h = POINT_SIZE;
    while (!done)
    {
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);
        SDL_RenderClear(renderer);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, SDL_ALPHA_OPAQUE);
        bezier(renderer, num, points, QUALITY);
        int x;
        int y;
        SDL_SetRenderDrawColor(renderer, 127, 127, 127, SDL_ALPHA_OPAQUE);
        for (size_t i = 0; i < num; i++)
        {
            rect.x = points[2 * i + 0] - POINT_SIZE / 2;
            rect.y = points[2 * i + 1] - POINT_SIZE / 2;
            SDL_RenderFillRect(renderer, &rect);
            if (i > 0)
            {
                SDL_RenderDrawLine(renderer, x, y, points[2 * i + 0], points[2 * i + 1]);
            }
            x = points[2 * i + 0];
            y = points[2 * i + 1];
        }
        SDL_RenderPresent(renderer);
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                done = SDL_TRUE;
            }

            if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                if (event.button.button == SDL_BUTTON_LEFT)
                {
                    selectedPoint = -1;
                    for (size_t i = 0; i < num; i++)
                    {
                        if (distance(points[2 * i + 0], points[2 * i + 1], event.button.x, event.button.y) <= POINT_SIZE)
                        {
                            selectedPoint = i;
                            break;
                        }
                    }
                    if (selectedPoint < 0)
                    {
                        num++;
                        int* newPts = (int*) SDL_malloc(2 * num * sizeof(int));
                        if (num <= 1)
                        {
                            points = newPts;
                            points[0] = event.button.x;
                            points[1] = event.button.y;
                            selectedPoint = num - 1;
                        }
                        else
                        {
                            int loc = num - 1;
                            for (size_t i = 0; i < num - 1; i++)
                            {
                                int x1 = points[2 * i + 0];
                                int y1 = points[2 * i + 1];
                                int x2 = points[2 * i + 2];
                                int y2 = points[2 * i + 3];
                                if  (   // If click is in rectangular bounding box between points
                                        (
                                            (
                                                event.button.x > x1 &&
                                                event.button.x < x2
                                            ) ||
                                            (
                                                event.button.x < x1 &&
                                                event.button.x > x2
                                            )
                                        ) &&
                                        (
                                            (
                                                event.button.y > y1 &&
                                                event.button.y < y2
                                            ) ||
                                            (
                                                event.button.y < y1 &&
                                                event.button.y > y2
                                            )
                                        )
                                    )
                                {   // Project to find perpendicular distance
                                    int dist = (double)SDL_abs((x2 - x1)*(y1 - event.button.y) - (x1 - event.button.x)*(y2 - y1)) / SDL_sqrt((double)(x2 - x1)*(x2 - x1) + (y2 - y1)*(y2 - y1));
                                    if (dist <= POINT_SIZE)
                                    {
                                        loc = i + 1;
                                        break;
                                    }
                                }
                            }
                            SDL_memcpy(newPts, points, sizeof(int) * 2 * loc);
                            newPts[2 * loc + 0] = event.button.x;
                            newPts[2 * loc + 1] = event.button.y;
                            if (loc < num - 1) SDL_memcpy(newPts + 2 * loc + 2, points + 2 * loc, sizeof(int) * 2 * (num - 1 - loc));
                            SDL_free(points);
                            points = newPts;
                            selectedPoint = loc;
                        }
                    }
                }

                if (event.button.button == SDL_BUTTON_RIGHT)
                {
                    selectedPoint = -1;
                    for (size_t i = 0; i < num; i++)
                    {
                        if (distance(points[2 * i + 0], points[2 * i + 1], event.button.x, event.button.y) <= POINT_SIZE)
                        {
                            num--;
                            int* newPts = (int*) SDL_malloc(2 * num * sizeof(int));
                            SDL_memcpy(newPts, points, 2 * i * sizeof(int));
                            if (i < num)
                            {
                                SDL_memcpy(newPts + 2 * i, points + 2 * (i + 1), 2 * (num - i) * sizeof(int));
                            }
                            SDL_free(points);
                            points = newPts;
                            break;
                        }
                    }
                }
            }

            if (event.type == SDL_MOUSEBUTTONUP)
            {
                selectedPoint = -1;
            }

            if (event.type == SDL_MOUSEMOTION)
            {
                if (selectedPoint >= 0)
                {
                    points[selectedPoint * 2 + 0] += event.motion.xrel;
                    points[selectedPoint * 2 + 1] += event.motion.yrel;
                }
            }
        }
    }

    SDL_free(points);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
    return 0;
}

void bezier(SDL_Renderer* renderer, int num, int* points, int quality)
{
    if (num <= 1) return;
    size_t size = triangle(num - 1)*2;
    float* intermediate = (float*) SDL_malloc(size*sizeof(float));
    float x = points[0];
    float y = points[1];
    for (size_t i = 0; i < quality; i++)
    {
        for (size_t j = 0; j < num-1; j++)
        {
            for (size_t k = 0; k < num-j-1; k++)
            {
                float x1;
                float y1;
                float dx;
                float dy;
                int root;
                if (j == 0)
                {
                    x1 = points[2 * k + 0];
                    y1 = points[2 * k + 1];
                    dx = points[2 * k + 2] - points[2 * k + 0];
                    dy = points[2 * k + 3] - points[2 * k + 1];
                }
                else
                {
                    root = 2 * (k + triangle(num - 1) - triangle(num - j));
                    x1 = intermediate[root + 0];
                    y1 = intermediate[root + 1];
                    dx = intermediate[root + 2] - intermediate[root + 0];
                    dy = intermediate[root + 3] - intermediate[root + 1];
                }
                root = 2 * (k + triangle(num - 1) - triangle(num - j - 1));
                intermediate[root + 0] = x1 + dx * (i / (float)quality);
                intermediate[root + 1] = y1 + dy * (i / (float)quality);
            }
        }
        SDL_RenderDrawLine(renderer, x, y, intermediate[size-2], intermediate[size - 1]);
        x = intermediate[size - 2];
        y = intermediate[size - 1];
    }
    SDL_RenderDrawLine(renderer, x, y, points[num * 2 - 2], points[num * 2 - 1]);
    SDL_free(intermediate);
}

int triangle(int n) {
    return n*(n + 1) / 2;
}

double distance(int x1, int y1, int x2, int y2) {
    return SDL_sqrt((double)((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1)));
}
