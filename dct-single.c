#include <stdio.h> 
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include <limits.h>

#define PSUDO_WIDTH  16
#define PSUDO_HEIGHT 16

// TEST IMAGE RESOLUTION
// #define PSUDO_WIDTH 1200
// #define PSUDO_HEIGHT 800

/*
    // RESOLUTION: QVGA
    #define PSUDO_WIDTH 320
    #define PSUDO_HEIGHT 240

    // RESOLUTION: VGA
    #define PSUDO_WIDTH 640
    #define PSUDO_HEIGHT 480

    // RESOLUTION: HD
    #define PSUDO_WIDTH 1280
    #define PSUDO_HEIGHT 720

    // RESOLUTION: WQHD
    #define PSUDO_WIDTH 2560
    #define PSUDO_HEIGHT 1440
*/

// Controller for the pixel value types (e.g. floats, doubles, etc.)
typedef double valueType;

// Pixel structure definition
// --------------------------
//
// r : Value of intensity of 'red' of this pixel
// g : Value of intensity of 'green' of this pixel
// b : Value of intensity of 'blue' of this pixel
// i : Value of grayscale intensity of this pixel
//
typedef struct {
    valueType r, g, b;
    valueType i;
} pixel;


// Image structure definition
// --------------------------
//
// width  : Amount of pixels in the x-direction
// height : Amount of pixels in the y-direction
// m      : The matrix itself containing all pixels
//
typedef struct {
    int width;
    int height;
    int channels;
    pixel** m;
} image;

// Struct to pass to each thread so it knows the 
// information needed for accessing
struct info {
    int threadIndex;
    int start;
    int end;
    int paddingStart;
    double error;
    image* srcIMG;
    image* dctIMG;
    image* idctIMG;
};

// ALLOCATION & INITIALIZATION
image* allocateImage(int width, int height, int channels);
image* generateImage(int width, int height, int channels, int fitAmnt);
int imGetPadSize(int totalThreads, int startingSize);
void imUnfit(image* srcIMG, image* dctIMG, image* idctIMG, int fitAmnt);
void imread(image* im, FILE *inFile);
void imwrite(image* im, char* fileName);
void imDelete(image* srcIMG, image* dctIMG, image* idctIMG);

// VALIDATION
int imValidate(image* imA, image* imB, double threshold);
double imERR(image* imA, image* imB);
double imERR2(image* imA, image* imB);
long double imMSE(image* imA, image* imB);

// OPERATIONS
void imPrint(image* im);
void imDCT(image* inIMG, image* outIMG);
void imIDCT(image* inIMG, image* outIMG);
void* imProcess(void* arg);
double runTest(int height, int width, int bits, int channels, int totalThreads);
void imBlockIDCT(image* inIMG, image* outIMG, int i, int j);
void imBlockDCT(image* inIMG, image* outIMG, int i, int j);




int main(int argc, char* argv[]) {



    ///////////////////////////////////////////
    //            INTITIAL SETUP             //
    ///////////////////////////////////////////

    int height, width, bits, channels, 
        totalThreads, paddedSize, fitFlag, padding;

    channels = 1, width = PSUDO_WIDTH, height = PSUDO_HEIGHT, 
                        fitFlag = 0, padding = 0, bits = 255;

    // Print the object information for verification
    printf("Height: %i\nWidth: %i\nBits: %.0f\nChannels: %i\n", 
                          height, width, log2(bits+1), channels);

    // Get the amount of threads to use and 
    // then check for padding requirements
    totalThreads = (argc < 2? 1 : atoi(argv[1]));
    paddedSize = imGetPadSize(totalThreads, height);
    if (paddedSize != height && totalThreads != 1)
        padding = paddedSize - height;

    printf("Total Threads: %i\n\n", totalThreads);

    // Read in or randomly generate the source 
    // image given the image  characteristics
    image* srcIMG  = generateImage(width, height, channels, padding);
    /*
        image* srcIMG = allocateImage(width, (height + padding), channels);
        FILE* inputFile = fopen("campus.pgm", "r+");
        imread(srcIMG, inputFile);
    */

    // Allocate for the DCT & IDCT images    
    image* dctIMG = allocateImage(width, (height + padding), channels);
    image* idctIMG = allocateImage(width, (height + padding), channels);





    ///////////////////////////////////////////
    //           THREADING SETUP             //
    ///////////////////////////////////////////

    // Create threading structure
    pthread_t tid[totalThreads];

    // Initialize structure to hold information for each thread
    struct info* s = (struct info*)malloc(totalThreads*sizeof(struct info));
    for (int i = 0; i < totalThreads; i++) {
        s[i].threadIndex  = i;
        s[i].start        = i*(height + padding)/totalThreads;
        s[i].end          = (i + 1)*(height + padding)/totalThreads;
        s[i].paddingStart = height;
        s[i].srcIMG       = srcIMG;
        s[i].dctIMG       = dctIMG;
        s[i].idctIMG      = idctIMG;
        s[i].error        = 0.0;
    }
    





    ///////////////////////////////////////////
    //          THREADING RUNTIME            //
    ///////////////////////////////////////////

    // Start timing the total time elapsed for process
    struct timespec start, end; 
    clock_gettime(CLOCK_REALTIME, &start);

    // Launch all threads
    for (int i = 0; i < totalThreads; i++)
        pthread_create(&tid[i], NULL, (void*)imProcess, &s[i]);

    // Collect all threads launched
    for (int i = 0; i < totalThreads; i++)
        pthread_join(tid[i], NULL);

    // Stop timer and set time elapsed value for process
    clock_gettime(CLOCK_REALTIME, &end);
    double time_spent = (end.tv_sec - start.tv_sec) + 
                        (end.tv_nsec - start.tv_nsec) / 1e9;

    // Unpad the image for cases of thread amount & height mismatch
    if (paddedSize != -1)
        imUnfit(srcIMG, dctIMG, idctIMG, (paddedSize - height));






    ///////////////////////////////////////////
    //          VERIFYING RESULTS            //
    ///////////////////////////////////////////

    // Manual image verification
    
        printf("Original Image:\n");
        imPrint(srcIMG);
        printf("\n\n");

        printf("DCT Image:\n");
        imPrint(dctIMG);
        printf("\n\n");

        printf("IDCT Image:\n");
        imPrint(idctIMG);
        printf("\n\n");
    


    // Get accuracy/percision of the process
    int DOP          = 12;
    double precision = (double)(1.0/(pow(10.0, (double)DOP)));
    printf("imValidate() return value: %i\n",     imValidate(srcIMG, idctIMG, precision));
    printf("    imERR1() return value: %.25f\n",  imERR(srcIMG, idctIMG));
    printf("    imERR2() return value: %.25f\n",  imERR2(srcIMG, idctIMG));
    printf("     imMSE() return value: %.15Le\n", imMSE(srcIMG, idctIMG));

    // Write the output images for verificiation
    imwrite(srcIMG, "srcIMG.pgm");
    imwrite(dctIMG, "dctIMG.pgm");
    imwrite(idctIMG, "idctIMG.pgm");
    return 0;
}

void* imProcess(void* arg) {

    // Parse the argument pointer into a local (struct info*) structure
    struct info* input = (struct info*)arg;

    // Iterate through the rows corrosponding to this thread
    for (int y = input->start; (y < input->end) && (y < input->paddingStart); y += 8) {

        // Iterate through the columns corrosponding to this thread
        for (int x = 0, width = input->srcIMG->width; x < width; x += 8) {
            imBlockDCT(input->srcIMG, input->dctIMG, x, y);
            imBlockIDCT(input->dctIMG, input->idctIMG, x, y);
        }
    }
}

void imBlockDCT(image* inIMG, image* outIMG, int i, int j) {
    double OOSQT = 1.0/sqrt(2.0);
    double sum   = 0.0;
    double HPW   = (double)16.0;

    for (int u = 0; u < 8; u++) {
        for (int v = 0; v < 8; v++) {

            sum = 0.0;
            for (int x = 0; x < 8; x++) {
                for (int y = 0; y < 8; y++) {
                    sum += (double)inIMG->m[i + x][j + y].i *
                           cos(((2.0*(double)(x)+1.0) * (double)u * M_PI)/HPW) *
                           cos(((2.0*(double)(y)+1.0) * (double)v * M_PI)/HPW);
                }
            }
            outIMG->m[i + u][j + v].i = 0.25 
                                        * (u == 0? OOSQT:1.0)
                                        * (v == 0? OOSQT:1.0)
                                        * sum;
        }
    }
}

void imBlockIDCT(image* inIMG, image* outIMG, int i, int j) {
    double OOSQT = 1.0/sqrt(2.0);
    double sum   = 0.0;
    double MPOL  = M_PI/(double)16.0;

    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {

            sum = 0.0;
            for (int u = 0; u < 8; u++) {
                for (int v = 0; v < 8; v++) {
                    sum += inIMG->m[i + u][j + v].i * 
                           (u == 0? OOSQT:1.0) * 
                           (v == 0? OOSQT:1.0) *
                           cos((2.0*(double)(x)+1.0) * (double)u * MPOL) *
                           cos((2.0*(double)(y)+1.0) * (double)v * MPOL);
                }
            }
            outIMG->m[i + x][j + y].i = sum * 0.25;
        }
    }
}

void imDCT(image* inIMG, image* outIMG) {
    double OOSQT = 1.0/sqrt(2.0);
    double sum   = 0.0;
    double HPW   = (double)(inIMG->height + inIMG->width);

    for (int u = 0; u < inIMG->width; u++) {
        for (int v = 0; v < inIMG->height; v++) {

            sum = 0.0;
            for (int x = 0; x < inIMG->width; x++) {
                for (int y = 0; y < inIMG->height; y++) {
                    sum += (double)inIMG->m[x][y].i *
                           cos(((2.0*(double)x+1.0) * (double)u*M_PI)/HPW) *
                           cos(((2.0*(double)y+1.0) * (double)v*M_PI)/HPW);
                }
            }
            outIMG->m[u][v].i = 1.0/((double)inIMG->height/2.0) *
                                (!u? OOSQT:1.0)*(!v? OOSQT:1.0) *
                                sum;
        }
    }
}

void imIDCT(image* inIMG, image* outIMG) {
    double OOSQT = 1.0/sqrt(2.0);
    double sum   = 0.0;
    double HPW  = (double)(inIMG->height + inIMG->width);
    double MPOL  = M_PI/(double)(HPW);

    for (int x = 0; x < inIMG->width; x++) {
        for (int y = 0; y < inIMG->height; y++) {

            sum = 0.0;
            for (int u = 0; u < inIMG->width; u++) {
                for (int v = 0; v < inIMG->height; v++) {
                    sum += inIMG->m[u][v].i * 
                           (!u? OOSQT:1.0) * 
                           (!v? OOSQT:1.0) *
                           cos((2.0*(double)x+1.0) * (double)u*MPOL) *
                           cos((2.0*(double)y+1.0) * (double)v*MPOL);
                }
            }
            outIMG->m[x][y].i = sum/((double)inIMG->height/2.0);
        }
    }
}

void imPrint(image* im) {

    // If input image is a grayscale image
    if (im->channels == 1) {
        for (int y = 0; y < im->height; y++) {
            for (int x = 0; x < im->width; x++) {

                // NOTE: This should be a %X.Yf, where
                //       X = Y+5 for spacing values and 
                //       the sign of the values neatly
                //
                printf("[%8.3f] ", im->m[x][y].i); // %18.13f for demo
            }
            printf("\n");
        }
    }

    // Else the input is a RGB image
    else {
        for (int y = 0; y < im->height; y++) {
            for (int x = 0; x < im->width; x++) {
                printf("[%3f,%3f,%3f]\t", 
                             im->m[x][y].r,
                             im->m[x][y].g,
                             im->m[x][y].b);
            }
            printf("\n");
        }
    }
}

// This should give the average error per ELEMENT
double imERR(image* imA, image* imB) {
    
    double avg_err = 0.0;
    for (int y = 0; y < imA->height; y++) {
        for (int x = 0; x < imA->width; x++) {

            // In the case of either being zero you get really bad
            // values, and I'm not sure what else to do but skip it
            if (imA->m[x][y].i == 0.0 || imB->m[x][y].i == 0.0)
                continue;

            avg_err += fabs(imB->m[x][y].i - imA->m[x][y].i)/imA->m[x][y].i;
        }
    }
    return avg_err;
}

// This should give the average error overall, 
// as in for the entire image
double imERR2(image* imA, image* imB) {
    double total_A = 0.0;
    double total_B = 0.0;
    for (int y = 0; y < imA->height; y++) {
        for (int x = 0; x < imA->width; x++) {
            total_A += (double)fabs(imA->m[x][y].i);
            total_B += (double)fabs(imB->m[x][y].i);
        }
    }
    return (100.0 * fabs(total_B - total_A)/total_A);
}

// This should give the average error overall, 
// as in for the entire image
long double imMSE(image* imA, image* imB) {
    image* imC = allocateImage(imA->width, 
                               imA->height, 
                               imA->channels);

    long double MSE = (long double)0.0;
    for (int y = 0; y < imA->height; y++) {
        for (int x = 0; x < imA->width; x++) {
            MSE += pow((long double)(imB->m[x][y].i - imA->m[x][y].i), 
                                                    (long double)2.0);
        }
    }
    return (MSE / (long double)(imA->height * imA->width));
}


int imValidate(image* imA, image* imB, double threshold) {
    if (imA->height != imB->height)
        return -7;

    if (imA->width != imB->width)
        return -6;

    if (imA->channels != imB->channels)
        return -5;
    
    // If input image is a grayscale image
    if (imA->channels == 1) {
        for (int y = 0; y < imA->height; y++) {
            for (int x = 0; x < imA->width; x++) {
                if (fabs(imA->m[x][y].i - imB->m[x][y].i) >= threshold) {
                        printf("INVALID POINT: (%i, %i)\n", x, y);
                        printf("A(%i, %i): [%.20f]\n", x, y, imA->m[x][y].i);
                        printf("B(%i, %i): [%.20f]\n", x, y, imB->m[x][y].i);
                        printf("A(x,y) - B(x,y): %.38f\n\n", 
                            imA->m[x][y].i - imB->m[x][y].i);
                    
                    return -4;
                }
            }
        }
    }

    // Else the input is a RGB image
    else {
        for (int y = 0; y < imA->height; y++) {
            for (int x = 0; x < imA->width; x++) {
                if (fabs(imA->m[x][y].r - imB->m[x][y].r) >= threshold)
                    return -1;

                if (fabs(imA->m[x][y].g - imB->m[x][y].g) >= threshold)
                    return -2;

                if (fabs(imA->m[x][y].b - imB->m[x][y].b) >= threshold)
                    return -3;
            }
        }
    }
    return 0;
}

image* allocateImage(int width, int height, int channels) {

    
    // Allocate an image structure
    //
    // NOTE:
    // 
    //    To be able to index with im[x][y], I need 
    //    to initialize with WIDTH, and then iterate over
    //    WIDTH allocating for each column of a certain HEIGHT
    //
    image* im = (image*)malloc(1*sizeof(image));
    im->m     = (pixel**)malloc((width)*sizeof(pixel));

    // Allocate each column of the image matrix
    for (int x = 0; x < width; x++)
        im->m[x] = (pixel*)malloc((height)*sizeof(pixel));

    im->height    = height;
    im->width     = width;
    im->channels  = channels;
    return im;
}

image* generateImage(int width, int height, int channels, int fitAmnt) {
    image* im = allocateImage(width, height + fitAmnt, channels);

    srand((unsigned)time(NULL));
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
            im->m[x][y].i = (double)(rand() % UCHAR_MAX);

    // Initialize all padded area to 0's
    for (int y = height; y < height+fitAmnt; y++)
        for (int x = 0; x < width; x++)
            im->m[x][y].i = 0;

    return im;
}

// Given the following,
// Ts = idx     * s / totalThreads;
// Te = (idx+1) * s / totalThreads;
//
// For it to work, either (Te-Ts)%8 == 0
//                 or...  (h/totalThreads)%8 == 0
//
// Get the correct size image for the thread amount given
int imGetPadSize(int totalThreads, int startingSize) {
    int found = 0;
    int sizeFound = 0;
    for (int s = startingSize; s < 1600; s++) {  
        for (int idx = 0; idx < totalThreads; idx++) {
            int start = idx     * s / totalThreads;
            int end   = (idx+1) * s / totalThreads;

            if (((end-start) % 8) == 0)
                found = 1;

            else {
                found = 0;
                break;
            }
        }
        if (found == 1)
            return s;
    }
    return -1;
}

void imUnfit(image* srcIMG, image* dctIMG, image* idctIMG, int fitAmnt) {
    for (int y = srcIMG->height, nSize = (srcIMG->height - fitAmnt); y >= nSize; y--) {
        free(srcIMG->m[y]);
        free(dctIMG->m[y]);
        free(idctIMG->m[y]);
    }
    srcIMG->height  = (srcIMG->height  - fitAmnt);
    dctIMG->height  = (dctIMG->height  - fitAmnt);
    idctIMG->height = (idctIMG->height - fitAmnt);
}

void imDelete(image* srcIMG, image* dctIMG, image* idctIMG) {
    for (int y = 0; y < srcIMG->height; y++) {
        free(srcIMG->m[y]);
        free(dctIMG->m[y]);
        free(idctIMG->m[y]);
    }
}

void imread(image* im, FILE *inFile) {

    // If input image is a grayscale image
    if (im->channels == 1) {
        int value;
        fscanf(inFile, "P2\n%i %i\n%i", &value, &value, &value);

        for (int y = 0; y < im->height; y++) {
            for (int x = 0; x < im->width; x++) {
               fscanf(inFile, "\n%i", &value);
               im->m[x][y].i = (valueType)value;
            }
        }
    }

    // Else the input is a RGB image
    else {
        int rv, gv, bv;
        for (int y = 0; y < im->height; y++) {
            for (int x = 0; x < im->width; x++) {
               fscanf(inFile, "\n%i %i %i", 
                             &rv, &gv, &bv);

                im->m[x][y].r = (valueType)rv;
                im->m[x][y].g = (valueType)gv;
                im->m[x][y].b = (valueType)bv;
            }
        }
    }
}

void imwrite(image* im, char* fileName) {
    FILE *inFile = fopen(fileName, "w+");
    fprintf(inFile, "P2\n%i %i\n255\n", im->width, im->height);

    for (int y = 0; y < im->height; y++) {
        for (int x = 0; x < im->width; x++) {

            if (im->m[x][y].i >= (valueType)0.0)
                fprintf(inFile, "%.0f\n", im->m[x][y].i);

            else
                fprintf(inFile, "0\n");
        }
    }
}
