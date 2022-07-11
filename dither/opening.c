#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include "timer.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"


#ifndef CONCURRENCY
#define CONCURRENCY 0
#endif

typedef struct
{
    void *memory;
    long long int width;
    long long int height;
    int bytesPerPixel;
} ScreenBuffer;

typedef struct
{
    Window *window;
    Display *display;
    int delay;
} SendRedrawEventArgs;

Atom wmDeleteMessage;
int isRunning = 0;
double inicio = 0, fim = 0, delta = 0;
#define NTHREADS 4
int nthreads = 1;
int setDisplay;
long long int *fila;
//variaveis para sincronizacao
pthread_mutex_t mutex;
pthread_mutex_t mutex2;
pthread_mutex_t locker;
int gidx;

void* threadedSendRedrawEvent (void* params);
XImage *create_ximage(Display *display, Visual *visual, ScreenBuffer* scbuffer);
void reflect(uint8_t *p, ScreenBuffer scbuffer);
void drawGradient(ScreenBuffer scbuffer);
void processEvent(Display *display, Window window, XImage *xImage, ScreenBuffer *scbuffer, int width, int height);

uint8_t find_closest_palette_color(uint8_t pixel);
void addPixelError(ScreenBuffer buffer, long long int x, long long int y, int value);
void dithering(ScreenBuffer scbuffer);
void *task (void *arg);
void dithering_conc(ScreenBuffer scbuffer);
void dither_line(ScreenBuffer scbuffer, long long int y);

void processEvent(Display *display, Window window, XImage *xImage, ScreenBuffer *scbuffer, int width, int height)
{
    XEvent ev = {};
    GC gc = DefaultGC(display, 0);
    XNextEvent(display, &ev);

    switch(ev.type)
    {
        case Expose:
                XPutImage(display, window, gc, xImage, 0, 0, 0, 0, scbuffer->width, scbuffer->height);
            break;
        case KeyPress:
            isRunning = 0;
            break;
        case ClientMessage:
            if (ev.xclient.data.l[0] == wmDeleteMessage)
            {
                isRunning = 0;
            }
   }
}

 void* threadedSendRedrawEvent (void* params)
{
    XEvent expose;
    SendRedrawEventArgs* args = (SendRedrawEventArgs*) params;
    while (1)
    {
        usleep(args->delay*1000);

        memset(&expose, 0, sizeof(expose));
        expose.type = Expose;
        expose.xexpose.window = *(args->window);
        XSendEvent(args->display , *(args->window), False, ExposureMask, &expose);
        XFlush((args->display));
        if(!isRunning){break;}
    }
    pthread_exit(NULL);
}

void reflect(uint8_t *p, ScreenBuffer scbuffer)
{
    int i = 0;
    for (i = 0; i < scbuffer.width * scbuffer.height * scbuffer.bytesPerPixel; i +=4)
    {
        // BGR
        uint8_t *bgrx = (uint8_t*) scbuffer.memory;
        bgrx[i + 0] = p[i + 0];
        bgrx[i + 1] = p[i + 1];
        bgrx[i + 2] = p[i + 2];
        bgrx[i + 3] = p[i + 3];
   }
}

void drawGradient(ScreenBuffer scbuffer)
{
    int i = 0;
    uint8_t c = 0;
    for (i = 0; i < scbuffer.width * scbuffer.height * scbuffer.bytesPerPixel; i += 4)
    {
        // BGRA
        uint8_t *bgrx = (uint8_t*) scbuffer.memory;
        c = (((256*i)/(scbuffer.width*4)));
        bgrx[i + 0] = c;
        bgrx[i + 1] = c;
        bgrx[i + 2] = c;
        bgrx[i + 3] = c;
   }
}

XImage *create_ximage(Display *display, Visual *visual, ScreenBuffer *scbuffer)
{
    scbuffer->memory = malloc(scbuffer->width * scbuffer->height * 4);
    if (!scbuffer->memory){fprintf(stderr, "Cannot alocate memory\n");exit(1);}

    return XCreateImage(display, visual, 24,
                        ZPixmap, 0, scbuffer->memory,
                        scbuffer->width, scbuffer->height, 32, 0);
}

ScreenBuffer load_img(char *image_name)
{
    int img_width = 0, img_height = 0, img_channels = 0, gray_img_channels = 0;
    size_t gray_img_size = 0, img_size = 0;
    uint8_t *gray_img = 0, *img = 0;
    ScreenBuffer buffer = {};

    img = stbi_load(image_name, &img_width, &img_height, &img_channels, 0);
    if(!img) {fprintf(stderr, "Error in loading the image[%s]\n", image_name); exit(1);}
    img_size = img_width * img_height * img_channels;
    /* gray_img_channels = img_channels == 4 ? 2 : 1; */
    gray_img_channels = 4;
    gray_img_size = img_width * img_height * gray_img_channels;
    gray_img = malloc(gray_img_size);
    if(!gray_img) {fprintf(stderr, "Unable to allocate memory for the gray image\n"); exit(1);}
    for(unsigned char *p = img, *pg = gray_img; p != img + img_size; p += img_channels, pg += gray_img_channels)
    {
        pg[0] = (uint8_t)((p[0] + p[1] + p[2])/3.0);
        pg[1] = (uint8_t)((p[0] + p[1] + p[2])/3.0);
        pg[2] = (uint8_t)((p[0] + p[1] + p[2])/3.0);
        if(img_channels == 4) {
            pg[3] = p[3];
        } else {
            pg[3] = 255;
        }
    }
    buffer.height = img_height;
    buffer.width = img_width;
    buffer.bytesPerPixel = gray_img_channels;
    buffer.memory = gray_img;
    printf("Loaded image with a width of %dpx, a height of %dpx and %d channels\n", img_width, img_height, img_channels);
    stbi_image_free(img);
    return buffer;
}

void dithering(ScreenBuffer scbuffer)
{
    int oldpixel, newpixel, error;
    unsigned char *data = scbuffer.memory;
    long long int  x,y;

    for (long long int i = 0; i < scbuffer.width * scbuffer.height * scbuffer.bytesPerPixel; i += 4)
    {
        y = (i / scbuffer.bytesPerPixel) / (scbuffer.width);
        x = (i / scbuffer.bytesPerPixel) % (scbuffer.width);

        oldpixel = data[y*scbuffer.width * scbuffer.bytesPerPixel + x * scbuffer.bytesPerPixel];
        newpixel = find_closest_palette_color(oldpixel);

        data[y*scbuffer.width * scbuffer.bytesPerPixel + x * scbuffer.bytesPerPixel+0] = newpixel;
        data[y*scbuffer.width * scbuffer.bytesPerPixel + x * scbuffer.bytesPerPixel+1] = newpixel;
        data[y*scbuffer.width * scbuffer.bytesPerPixel + x * scbuffer.bytesPerPixel+2] = newpixel;
        data[y*scbuffer.width * scbuffer.bytesPerPixel + x * scbuffer.bytesPerPixel+3] = newpixel;
        error = oldpixel - newpixel;
        addPixelError(scbuffer, x  , y+1, (error*5)/16);
        addPixelError(scbuffer, x-1, y+1, (error*3)/16);
        addPixelError(scbuffer, x+1, y  , (error*7)/16);
        addPixelError(scbuffer, x+1, y+1, (error*1)/16);
   }
}


pthread_cond_t done;
int main(int argc, char **argv)
{
    Display *display;
    XImage *xGradient = 0, *xImage = 0;
    Visual *visual;
    GC gc;
    Window window;
    int screen;
    long long int width = 512;
    long long int height = 512;
    pthread_t thread;
    ScreenBuffer imgbuffer = {};
    ScreenBuffer scbuffer = {};
    scbuffer.width = width;
    scbuffer.height = height;
    scbuffer.bytesPerPixel = 4;
    uint8_t *img = 0;
    setDisplay = 1;
    nthreads = NTHREADS;

    if (argc == 2)
    {
        imgbuffer = load_img(argv[1]);
        img = imgbuffer.memory;
        width = imgbuffer.width;
        height = imgbuffer.height;
    }

    if (argc == 3)
    {
        width = height = strtol(argv[1], NULL, 10);
        scbuffer.width = scbuffer.height = width;
        nthreads = strtol(argv[2], NULL, 10);
        setDisplay = 0;
    }
    if(setDisplay)
    {
        display = XOpenDisplay(NULL);
        if (display == NULL){fprintf(stderr, "Cannot open display\n"); exit(1);}
        screen = DefaultScreen(display);

        wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);
        window = XCreateSimpleWindow(display, RootWindow(display, screen), 0, 0, width, height, 1, 0, 0);
        XSelectInput(display, window, ExposureMask | KeyPressMask);
        XSetWMProtocols(display, window, &wmDeleteMessage, 1);
        gc = XCreateGC(display, window, 0, NULL);
        visual = DefaultVisual(display, 0);


        XMapWindow(display, window);
        xGradient = create_ximage(display, visual, &scbuffer);
        xImage = create_ximage(display, visual, &imgbuffer);
    }
    else
    {
        scbuffer.memory = malloc(scbuffer.width * scbuffer.height * 4);
        if (!scbuffer.memory){fprintf(stderr, "Cannot alocate memory\n");exit(1);}
    }
    SendRedrawEventArgs *bgRedrawThread =  0;
    if(setDisplay)
    {
        bgRedrawThread = malloc(sizeof(SendRedrawEventArgs));
        bgRedrawThread->window = &window;
        bgRedrawThread->display = display;
        bgRedrawThread->delay = 500;
        if (!bgRedrawThread){fprintf(stderr, "Cannot alocate memory\n"); exit(1);}
        pthread_create(&thread, NULL, threadedSendRedrawEvent, (void*) bgRedrawThread);
    }

    // update loop
    isRunning = 1;

    pthread_mutex_init(&mutex, NULL);
    pthread_mutex_init(&mutex2, NULL);
    pthread_mutex_init(&locker, NULL);
    pthread_cond_init(&done, NULL);

    fila = malloc(sizeof(long long int)*height);
    memset(fila, 0, sizeof(long long int)*height);

    if(argc == 2)
    {
        reflect(img, imgbuffer);
#if CONCURRENCY
        dithering_conc(imgbuffer);
#else
        dithering(imgbuffer);
#endif
    }
    else if(argc == 3)
    {
        drawGradient(scbuffer);
        GET_TIME(inicio);
        printf("Dithering sequencial\n");
        dithering(scbuffer);
        GET_TIME(fim);
        delta = fim - inicio;
        printf("Dithering sequencial: %lf\n", delta);
        fflush(stdout);
        
        drawGradient(scbuffer);
        printf("Dithering concorrente\n");
        GET_TIME(inicio);
        dithering_conc(scbuffer);
        GET_TIME(fim);
        delta = fim - inicio;
        printf("Dithering concorrente: %lf\n", delta);
    }
    else
    {
        drawGradient(scbuffer);
#if CONCURRENCY
        dithering_conc(scbuffer);
#else
        dithering(scbuffer);
#endif
    }


    while(isRunning)
    {
        if(argc == 2) {
            processEvent(display, window, xImage, &imgbuffer, width, height);
        }
        else if(argc == 3)
        {
            break;
        }
        else
        {
            processEvent(display, window, xGradient, &scbuffer, width, height);
        }
    }
    if(xImage) {XDestroyImage(xImage);}
    if(xGradient) {XDestroyImage(xGradient);}
    if(setDisplay)
    {
        if(pthread_join(thread, NULL)){fprintf(stderr, "error");}
        if(bgRedrawThread) {free(bgRedrawThread);}
        XFreeGC(display, gc);
        XCloseDisplay(display);
    }

    if(img) stbi_image_free(img);
    free(fila);
    if(!setDisplay){free(scbuffer.memory);}

    return 0;
}

void dithering_conc(ScreenBuffer scbuffer)
{
    pthread_t *tid = malloc(sizeof(pthread_t) * NTHREADS);
    ScreenBuffer *args = malloc(sizeof(ScreenBuffer) * NTHREADS);
    for(int i = 0; i < NTHREADS; i++)
    {
        args[i].width = scbuffer.width;
        args[i].height = scbuffer.height;
        args[i].bytesPerPixel = scbuffer.bytesPerPixel;
        args[i].memory = scbuffer.memory;
        pthread_create(&tid[i], NULL, task, (void*) &args[i]);
    }
    for(int i = 0; i < NTHREADS; i++)
    {
        if(pthread_join(tid[i], NULL))
        {
            fprintf(stderr, "error\n");
        }
    }
    free(tid);
    free(args);
}

void *task (void *arg)
{
    ScreenBuffer *buffer = arg;
    long long int i = 0;
    // o indice global diz qual linha será a próxima discretizada
    while(1)
    {
        pthread_mutex_lock(&mutex2);
        if(gidx < buffer->height)
        {
            i = gidx++;
            pthread_mutex_unlock(&mutex2);
            dither_line(*buffer, i);
        }
        else
        {
            pthread_mutex_unlock(&mutex2);
            break;
        }
    }
    return NULL;
}

void dither_line(ScreenBuffer scbuffer, long long int y)
{
    int oldpixel, newpixel, error;
    uint8_t *data = scbuffer.memory;
    long long int x = 0;

    for (x = 0; x < scbuffer.width ;x++)
    {
        while(1)
        {
            pthread_mutex_lock(&locker);
            if((y == 0) || (fila[y-1] > x + 3) || (fila[y-1] == scbuffer.width))
            {
                pthread_mutex_unlock(&locker);
                oldpixel = data[y*scbuffer.width * scbuffer.bytesPerPixel + x * scbuffer.bytesPerPixel];
                newpixel = find_closest_palette_color(oldpixel);

                data[y*scbuffer.width * scbuffer.bytesPerPixel + x * scbuffer.bytesPerPixel+0] = newpixel;
                data[y*scbuffer.width * scbuffer.bytesPerPixel + x * scbuffer.bytesPerPixel+1] = newpixel;
                data[y*scbuffer.width * scbuffer.bytesPerPixel + x * scbuffer.bytesPerPixel+2] = newpixel;
                data[y*scbuffer.width * scbuffer.bytesPerPixel + x * scbuffer.bytesPerPixel+3] = newpixel;
                error = oldpixel - newpixel;

                addPixelError(scbuffer, x  , y+1, (error*5)/16);
                addPixelError(scbuffer, x-1, y+1, (error*3)/16);
                addPixelError(scbuffer, x+1, y  , (error*7)/16);
                addPixelError(scbuffer, x+1, y+1, (error*1)/16);
                fila[y] = x;
                pthread_cond_broadcast(&done);
                /* pthread_cond_signal(&done); */
                break;
            }
            else
            {
                pthread_cond_broadcast(&done);
                pthread_cond_wait(&done, &locker);
                pthread_mutex_unlock(&locker);
            }
        }
        if(x == scbuffer.width - 1){fila[y] = scbuffer.width;}
    }
}

void addPixelError(ScreenBuffer buffer, long long int x, long long int y, int value)
{
    unsigned char *data = buffer.memory;
    int color;
    if((x >= 0) & (x < buffer.width) & (y >= 0) & (y < buffer.height))
    {
        color = data[y*buffer.width*buffer.bytesPerPixel + x*buffer.bytesPerPixel];
        color += value;
        color &= 0xFF;
        for(int i = 0; i < 4; i++)
        {
            data[y*buffer.width*buffer.bytesPerPixel + x*buffer.bytesPerPixel + i] = (unsigned char) color;
        }
    }
}

uint8_t find_closest_palette_color(uint8_t pixel)
{
    return (pixel)>127?255:0;
}
