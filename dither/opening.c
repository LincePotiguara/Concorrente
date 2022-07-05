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

#define DEBUG_PRINT 0

typedef struct
{
    void *memory;
    long long int width;
    long long int height;
    int bytesPerPixel;
    int idx;
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
// FIXME: faça com que isso realmente funcione
pthread_mutex_t mutex;
pthread_mutex_t locker;
pthread_cond_t cond_leit, cond_escr;
int leader, gidx;
/* pthread_t tid[NTHREADS]; */
/* pthread_t *tid; */
/* pthread_mutex_t lock_list[NTHREADS]; */
pthread_mutex_t *lock_list = 0;
void* threadedSendRedrawEvent (void* params);
XImage *create_ximage(Display *display, Visual *visual, ScreenBuffer* scbuffer);
//void draw(ScreenBuffer scbuffer);
void reflect(uint8_t *p, ScreenBuffer scbuffer);
//void drawGradient(ScreenBuffer scbuffer);
void processEvent(Display *display, Window window, XImage *ximage, ScreenBuffer *scbuffer, int width, int height);

uint8_t find_closest_palette_color(uint8_t pixel);
void addPixelError(ScreenBuffer buffer, long long int x, long long int y, int value);
void dithering(ScreenBuffer scbuffer);
void *task (void *arg);
void dithering_conc(ScreenBuffer scbuffer);
void dither_line(ScreenBuffer scbuffer, long long int y);



void processEvent(Display *display, Window window, XImage *ximage, ScreenBuffer *scbuffer, int width, int height)
{
    XEvent ev = {};
    GC gc = DefaultGC(display, 0);
    XNextEvent(display, &ev);

    switch(ev.type)
    {
        case Expose:
         //XFillRectangle(display, window, DefaultGC(display, screen), 20, 20, 10, 10);
         //XDrawString(display, window, DefaultGC(display, screen), 10, 50, msg, strlen(msg));
         // apenas chame draw se quiser computar uma mudança todo xImage
         //draw(*scbuffer);
         //draw(scbuffer->memory, scbuffer->width, scbuffer->height);
            /* reflect(*scbuffer); */
            XPutImage(display, window, gc, ximage, 0, 0, 0, 0, scbuffer->width, scbuffer->height);
            // printf("x %d, y %d \n", scbuffer->width, scbuffer->height);
            // printf("handling expose\n");
            // fflush(stdout);
            break;
        case KeyPress:
            isRunning = 0;
            break;
        case ClientMessage:
            // you can use atom to get properties information, WM_DELETE_WINDOW is a property
            if (ev.xclient.data.l[0] == wmDeleteMessage)
            {
                isRunning = 0;
            }
            //RenderWeirdGradient(buffer, XOffset, YOffset);
   }
}

 void* threadedSendRedrawEvent (void* params)
{
    XEvent expose;
    SendRedrawEventArgs* args = (SendRedrawEventArgs*) params;
    while (1)
    {
        // sleep (3);
        usleep(args->delay*1000);
        // printf ("Sending event of type Expose\n");
        memset(&expose, 0, sizeof(expose));
        expose.type = Expose;
        expose.xexpose.window = *(args->window);
        XSendEvent(args->display , *(args->window), False, ExposureMask, &expose);
        XFlush((args->display));
        if(!isRunning){break;}
    }
    pthread_exit(NULL);
    // return NULL;
}
/*
void drawGradient(ScreenBuffer scbuffer)
{
    int i = 0;
    for (i = 0; i < scbuffer.width * scbuffer.height * scbuffer.bytesPerPixel; i += 4)
    {
        // BGR
        uint8_t *bgrx = (uint8_t*) scbuffer.memory;
        bgrx[i + 0] = (i%256);
        bgrx[i + 1] = (i%256);
        bgrx[i + 2] = (i%256);
        bgrx[i + 3] = (i%256);
    }
}
*/
void reflect(uint8_t *p, ScreenBuffer scbuffer)
{
    int i = 0;
#if DEBUG_PRINT
    printf("==========\n");
#endif
    for (i = 0; i < scbuffer.width * scbuffer.height * scbuffer.bytesPerPixel; i +=4)
    {
        // BGR
        uint8_t *bgrx = (uint8_t*) scbuffer.memory;
#if DEBUG_PRINT
        /* printf("(%d, %d) = %hhu %hhu %hhu %hhu \n",((i/4)%scbuffer.width), ((i/4)/scbuffer.height), bgrx[i], bgrx[i+1],  bgrx[i+2], bgrx[i+3]); */
        printf("(%d, %d) = %hhu \n", ((i/4)%scbuffer.width), ((i/4)/scbuffer.height), bgrx[i]);
#endif
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
        /* reflect(scbuffer); */
        /* c++; */
   }
}

//XImage *create_ximage(Display *display, Visual *visual, char **image32, int width, int height)
XImage *create_ximage(Display *display, Visual *visual, ScreenBuffer *scbuffer)
{
    scbuffer->memory = malloc(scbuffer->width * scbuffer->height * 4);
    if (!scbuffer->memory){fprintf(stderr, "Cannot alocate memory\n");exit(1);}
    //scbuffer->size = scbuffer->width * scbuffer->height * 4;
    //draw(*image32, width, height);
    /* reflect(*scbuffer); */
#if DEBUG_PRINT
    uint32_t *data = scbuffer->memory;
    int s = 0x7e7e7e7e;
    data[0] = s;
    data[1] = s;
    data[2] = s;
    data[3] = s;
#else
    /* draw(*scbuffer); */
#endif
   //
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
    //buffer.size = gray_img_size;
    buffer.memory = gray_img;
    /* stbi_write_jpg("sky_gray.png", img_width, img_height, gray_img_channels, gray_img, 95); */
    printf("Loaded image with a width of %dpx, a height of %dpx and %d channels\n", img_width, img_height, img_channels);
    stbi_image_free(img);
    return buffer;
}

void dithering(ScreenBuffer scbuffer)
{
    int oldpixel, newpixel, error;
    unsigned char *data = scbuffer.memory;
    /* int offsetX, offsetY; */
    /* int color, xindex, yindex; */
    long long int  x,y;
    /* GET_TIME(inicio); */

    for (long long int i = 0; i < scbuffer.width * scbuffer.height * scbuffer.bytesPerPixel; i += 4)
    {
        y = (i / scbuffer.bytesPerPixel) / (scbuffer.width);
        x = (i / scbuffer.bytesPerPixel) % (scbuffer.width);
        /* xindex = x * scbuffer.bytesPerPixel; */
        /* yindex = y * scbuffer.width * scbuffer.bytesPerPixel; */

        oldpixel = data[y*scbuffer.width * scbuffer.bytesPerPixel + x * scbuffer.bytesPerPixel];
        newpixel = find_closest_palette_color(oldpixel);

        data[y*scbuffer.width * scbuffer.bytesPerPixel + x * scbuffer.bytesPerPixel+0] = newpixel;
        data[y*scbuffer.width * scbuffer.bytesPerPixel + x * scbuffer.bytesPerPixel+1] = newpixel;
        data[y*scbuffer.width * scbuffer.bytesPerPixel + x * scbuffer.bytesPerPixel+2] = newpixel;
        data[y*scbuffer.width * scbuffer.bytesPerPixel + x * scbuffer.bytesPerPixel+3] = newpixel;
        error = oldpixel - newpixel;
#if DEBUG_PRINT
        printf(" error:%d ", (error*5)/16);
        printf("(%d, %d) = %hhu %hhu %hhu %hhu \n", x, y,
               data[yindex+xindex], data[yindex+xindex+1],  data[yindex+xindex+2], data[yindex+xindex+3]);
#endif
        addPixelError(scbuffer, x  , y+1, (error*5)/16);
        addPixelError(scbuffer, x-1, y+1, (error*3)/16);
        addPixelError(scbuffer, x+1, y  , (error*7)/16);
        addPixelError(scbuffer, x+1, y+1, (error*1)/16);
   }
    /* GET_TIME(fim); */
    /* delta = fim - inicio; */
    /* printf("Timeit: %lf\n", delta); */
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
    /* char* buffer = 0; */
    long long int width = 5;
    long long int height = 5;
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
    /* printf("%lld %lld\n", width, height); */
    // connect to server
    if(setDisplay)
    {
        display = XOpenDisplay(NULL);
        if (display == NULL){fprintf(stderr, "Cannot open display\n"); exit(1);}
        screen = DefaultScreen(display);

        wmDeleteMessage = XInternAtom(display, "WM_DELETE_WINDOW", False);

/*  window = XCreateSimpleWindow(display, RootWindow(display, screen),
                               10, 10, width, height, 1,
                               BlackPixel(display, screen), WhitePixel(display, screen));
*/
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
    pthread_mutex_init(&locker, NULL);
    pthread_cond_init(&done, NULL);
    // Inicializa a lista de locks

    lock_list = malloc(sizeof(pthread_mutex_t) * nthreads);
    for(int i = 0; i < nthreads; i++)
    {
        pthread_mutex_init(&lock_list[i], NULL);
    }

    fila = malloc(sizeof(long long int)*height);
    memset(fila, 0,sizeof(long long int)*height);
    //XAddPixel(xGradient, (long)0x00ff00);

    if(argc == 2)
    {
        reflect(img, imgbuffer);
        dithering(imgbuffer);
        /* stbi_write_jpg("dither256.jpg", width, height, 4, imgbuffer.memory, 95); */
        /* dithering_conc(imgbuffer); */
        /* XPutImage(display, window, gc, xImage, 0, 0, 0, 0, img_width, img_height); */
    }
    else if(argc == 3)
    {

        drawGradient(scbuffer);
        GET_TIME(inicio);
        printf("Dithering sequencial\n");
        for(int times = 0; times < 5; times++)
        {
            /* drawGradient(scbuffer); */
            dithering(scbuffer);
        }
        GET_TIME(fim);
        delta = fim - inicio;
        printf("Dithering sequencial: %lf\n", delta);
        fflush(stdout);

        drawGradient(scbuffer);
        printf("Dithering concorrente\n");
        GET_TIME(inicio);
        for(int times = 0; times < 5; times++)
        {
            /* drawGradient(scbuffer); */
            dithering_conc(scbuffer);
        }
        GET_TIME(fim);
        delta = fim - inicio;
        printf("Dithering concorrente: %lf\n", delta);
    }
    else
    {
        drawGradient(scbuffer);
#if 0
        dithering(scbuffer);
#else
        dithering_conc(scbuffer);
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
    printf("Encerrando\n");
    if(xImage) {XDestroyImage(xImage);}
    if(xGradient) {XDestroyImage(xGradient);}
    if(bgRedrawThread) {free(bgRedrawThread);}
    if(setDisplay)
    {
        if(pthread_join(thread, NULL)){fprintf(stderr, "error");}
        XFreeGC(display, gc);
        XCloseDisplay(display);
    }
    /* lock_list = malloc(sizeof(pthread_mutex_t) * nthreads); */
    for(int i = 0; i < nthreads; i++)
    {
        pthread_mutex_destroy(&lock_list[i]);
    }
    free(lock_list);
    if(img) stbi_image_free(img);
    free(fila);
    if(!setDisplay){free(scbuffer.memory);}
    //if(gray_img)free(gray_img);
    /* if(bgRedrawThread) free(bgRedrawThread); */
    return 0;
}

void dithering_conc(ScreenBuffer scbuffer)
{
    //FIXME Esse procedimento entra em deadlocks
    //FIXME Esse procedimento acessa memoria invalidamente
    /* printf("Dithering Concorrente inicio\n"); */
    //int oldpixel, newpixel, error;
    //unsigned char *data = scbuffer.memory;

    //int x, y;
    pthread_t *tid = malloc(sizeof(pthread_t) * NTHREADS);
    ScreenBuffer *args = malloc(sizeof(ScreenBuffer) * NTHREADS);

    /* printf("tasks\n"); */
    for(int i = 0; i < NTHREADS; i++)
    {
        args[i].width = scbuffer.width;
        args[i].height = scbuffer.height;
        args[i].bytesPerPixel = scbuffer.bytesPerPixel;
        args[i].memory = scbuffer.memory;
        args[i].idx = i;
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
    /* printf("Dithering Concorrente Fim\n"); */

    /*
    for(int y = 0; y < scbuffer.height; y++)
    {
        dither_line(scbuffer, y);
    }*/
    /*
   for (int i = 0; i < scbuffer.width * scbuffer.height * scbuffer.bytesPerPixel; i += scbuffer.bytesPerPixel)
   {
        y = (i / scbuffer.bytesPerPixel) / (scbuffer.width);
        x = (i / scbuffer.bytesPerPixel) % (scbuffer.width);
   }*/
}

void *task (void *arg)
{
    ScreenBuffer *buffer = arg;
    long long int i = 0;
    // o indice global diz qual linha será a próxima discretizada
    while(1)
    {
        if(gidx < buffer->height)
        {
            pthread_mutex_lock(&mutex);
            i = gidx++;
            /* printf("i = %d de %d\n", i, buffer->height); */
            /* fflush(stdout); */
            pthread_mutex_unlock(&mutex);
            dither_line(*buffer, i);
        }
        else
        {
            //pthread_mutex_unlock(&mutex);
            break;
        }
    }
    return NULL;
}

/* pthread_mutex_t lock_list[NTHREADS]; */
#define conc 0
void dither_line(ScreenBuffer scbuffer, long long int y)
{
    int oldpixel, newpixel, error;
    uint8_t *data = scbuffer.memory;
    /* int index = scbuffer.idx; */
    long long int x = 0;
    /* int enter = 0; */
#if conc
    printf("First %d\n", y);
#endif
    for (x = 0; x < scbuffer.width ;x++)
    {
        while(1)
        {
            /* pthread_mutex_lock(&lock_list[index]); */
            pthread_mutex_lock(&locker);
#if conc
            fflush(stdout);
            printf("lock(i%d, y%d)[%d] x%d \n", index, y, (y == 0) || (fila[y-1] > x + 2) || (fila[y-1] == scbuffer.width), x);
            fflush(stdout);
            /* printf("<"); */
#endif
            /* enter = (y == 0) || (fila[y-1] > x + 2) || (fila[y-1] == scbuffer.width); */
            /* if(enter) */
            if((y == 0) || (fila[y-1] > x + 500) || (fila[y-1] == scbuffer.width))
            {
                pthread_mutex_unlock(&locker);
#if conc
                /* printf("if>"); */
#endif
                /* pthread_mutex_unlock(&lock_list[index]); */
                /* if(fila[y-1] > ){print();} */
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
                if((scbuffer.width*3/2 == x) | (scbuffer.width/2 == x))
                {
                    pthread_cond_broadcast(&done);
                }
                break;
            }
            else
            {
                /* fflush(stdout); */
                pthread_cond_broadcast(&done);
                /* pthread_cond_signal(&done); */
                /* pthread_cond_wait(&done, &lock_list[index]); */
                /* printf("wait<%lld %lld>\n", x, y); */
                /* fflush(stdout); */
                pthread_cond_wait(&done, &locker);
                /* printf("<%lld %lld>wait\n", x, y); */
                /* fflush(stdout); */
                /* pthread_mutex_unlock(&lock_list[index]); */
                pthread_mutex_unlock(&locker);
#if conc
                /* printf("el>"); */
#endif
            }
        }
        fila[y] = scbuffer.width;
    }
#if conc
    printf("fim line %d\n", index);
#endif
    /* pthread_mutex_unlock(&lock_list[index]); */
    /* pthread_mutex_unlock(&locker); */
}

void addPixelError(ScreenBuffer buffer, long long int x, long long int y, int value)
{
    unsigned char *data = buffer.memory;
    int color;
    if((x >= 0) & (x < buffer.width) & (y >= 0) & (y < buffer.height))
    {
#if DEBUG_PRINT
        printf("[%lld->%lld]", data[y*buffer.width*buffer.bytesPerPixel + x*buffer.bytesPerPixel + 0],  value);
#endif
        color = data[y*buffer.width*buffer.bytesPerPixel + x*buffer.bytesPerPixel];
        color += value;
        color &= 0xFF;
        for(int i = 0; i < 4; i++)
        {
            data[y*buffer.width*buffer.bytesPerPixel + x*buffer.bytesPerPixel + i] = (unsigned char) color;
        }
        /*
        data[y*buffer.width*buffer.bytesPerPixel + x*buffer.bytesPerPixel + 0] = (unsigned char) color;
        data[y*buffer.width*buffer.bytesPerPixel + x*buffer.bytesPerPixel + 1] = (unsigned char) color;
        data[y*buffer.width*buffer.bytesPerPixel + x*buffer.bytesPerPixel + 2] = (unsigned char) color;
        data[y*buffer.width*buffer.bytesPerPixel + x*buffer.bytesPerPixel + 3] = (unsigned char) color;
        */
    }
}

uint8_t find_closest_palette_color(uint8_t pixel)
{
    return (pixel)>127?255:0;
    unsigned char div = 256/3;
    unsigned char color = pixel/div;
    color = color * div;
    return color;
}
