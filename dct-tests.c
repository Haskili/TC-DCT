#include <stdio.h> 
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <pthread.h>

#define PSUDO_WIDTH 2560
#define PSUDO_HEIGHT 1440

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


// Thread Info structure definition
// --------------------------
//
// threadIndex: What 'i-th' index a thread is in all threads used
// start      : Starting height index for thread to iterate over
// end        : Starting height index for thread to iterate over
// padIndex   : Starting index for padding in an image
// srcIMG     : Pointer to the source image
// dctIMG     : Pointer to the DCT image
// idctIMG    : Pointer to the IDCT image
//
typedef struct {
    int threadIndex;
    int start;
    int end;
    int padIndex;
    image* srcIMG;
    image* dctIMG;
    image* idctIMG;
} threadInfo;

typedef struct {
    int validation;
    double time_spent;
    long double err1;
    long double err2;
    long double err3;
} testResults;


// ALLOCATION & INITIALIZATION
image* allocateImage(int width, int height, int channels);
image* generateImage(int width, int height, int channels, int padding);
int imGetPadSize(int totalThreads, int startingSize);
void imRemovePadding(image* srcIMG, image* dctIMG, image* idctIMG, int padAmnt);
void imread(image* im, FILE *inFile);
void imwrite(image* im, char* fileName);
void imDelete(image* srcIMG, image* dctIMG, image* idctIMG);

// VALIDATION
int imValidate(image* imA, image* imB, double threshold);
double imERR1(image* imA, image* imB);
double imERR2(image* imA, image* imB);
long double imMSE(image* imA, image* imB);


// OPERATIONS
void imPrint(image* im);
void imDCT(image* inIMG, image* outIMG);
void imIDCT(image* inIMG, image* outIMG);
void imBlockIDCT(image* inIMG, image* outIMG, int i, int j);
void imBlockDCT(image* inIMG, image* outIMG, int i, int j);

// PRIMARY CALLS
testResults* runTest(int totalThreads, int width, 
                        int height, int channels);

void* imProcess(void* arg);

int main(int argc, char* argv[]) {
 

    ///////////////////////////////////////////
    //            INTITIAL SETUP             //
    ///////////////////////////////////////////

    long double time_AVG, err1_AVG, err2_AVG, err3_AVG;
    int height, width, bits, channels, totalThreads;
    char* format = (char*)malloc(sizeof(char)*5);

    // Initialize file pointer and fopen() the image file
    FILE *inFile = fopen((argc == 1? "imtest2.ppm":argv[1]), "r+");

    // Read in the image file characteristics (e.g. height & width) and
    // generate an image based off the characteristics read in
    fscanf(inFile, "%s\n %i %i\n%i", format, &width, &height, &bits);
    channels = 1, width = PSUDO_WIDTH, height = PSUDO_HEIGHT;

    // Print the object information for verification
    printf("Format: %s\nHeight: %i\nWidth: %i\nBits: %.0f\nChannels: %i\n\n", 
                              format, height, width, log2(bits+1), channels);
    


    ///////////////////////////////////////////
    //           TEST ITERATIONS             //
    ///////////////////////////////////////////

    // Perform the tests and print out each set of results
    for (int thread = 1; thread <= 10; thread++) {

        // Reset the running averages
        time_AVG = (long double)0;
        err1_AVG = (long double)0;
        err2_AVG = (long double)0;
        err3_AVG = (long double)0;

        // Perform a certain amount of iterations with the same
        // exact parameters to average over as the final result
        for (int it = 0; it < 25; it++) {

            // Run the test with current parameters   
            testResults* result = runTest(thread, PSUDO_WIDTH, 
                                          PSUDO_HEIGHT, channels);

            // Print the test results
            printf("         Time Elapsed: %.15f\n",  result->time_spent);
            printf("  imValidate() return: %i\n",     result->validation);
            printf("imERR1() return value: %.25Le\n", result->err1);
            printf("imERR2() return value: %.25Le\n", result->err2);
            printf(" imMSE() return value: %.25Le\n", result->err3);
            printf("\n\n");

            // Increment the running averages
            time_AVG += (long double)result->time_spent;
            err1_AVG += (long double)result->err1;
            err2_AVG += (long double)result->err2;
            err3_AVG += (long double)result->err3;
            free(result);
        }

        // Print the overall averages for tests 
        // with current parameters
        printf("\n------------------------------------------\n");
        printf("\n FINISHED ITERATIONS FOR '%i' THREADS \n\n", thread);
        printf("Average Time Elapsed: %.15Lf\n", (time_AVG/(long double)25.0));
        printf("    Average imERR1(): %.25Le\n", (err1_AVG/(long double)25.0));
        printf("    Average imERR2(): %.25Le\n", (err2_AVG/(long double)25.0));
        printf("     Average imMSE(): %.25Le\n", (err3_AVG/(long double)25.0));
        printf("\n------------------------------------------\n");
        printf("\n\n\n\n\n\n\n\n\n\n\n\n");
    }
    return 0;
}

testResults* runTest(int totalThreads, int width, 
                        int height, int channels) {



    ///////////////////////////////////////////
    //            INTITIAL SETUP             //
    ///////////////////////////////////////////

    // Create structure to hold runtime results
    testResults* results = (testResults*)malloc(1*sizeof(testResults));

    // Check for padding requirements
    int padding = 0, paddedSize = imGetPadSize(totalThreads, height);
    if (paddedSize != -1 && paddedSize != height && totalThreads != 1)
        padding = paddedSize - height;

    // Randomly generate the input image based on characteristics given
    image* srcIMG = generateImage(width, height, channels, padding);

    // Allocate for the DCT & IDCT images    
    image* dctIMG = allocateImage(width, (height + padding), channels);
    image* idctIMG = allocateImage(width, (height + padding), channels);



    ///////////////////////////////////////////
    //           THREADING SETUP             //
    ///////////////////////////////////////////

    // Create threading information
    pthread_t tid[totalThreads];
    threadInfo* th = (threadInfo*)malloc(totalThreads*sizeof(threadInfo));
    for (int i = 0; i < totalThreads; i++) {
        th[i].threadIndex  = i;
        th[i].start        = i*(height + padding)/totalThreads;
        th[i].end          = (i + 1)*(height + padding)/totalThreads;
        th[i].padIndex     = height;
        th[i].srcIMG       = srcIMG;
        th[i].dctIMG       = dctIMG;
        th[i].idctIMG      = idctIMG;
    }



    ///////////////////////////////////////////
    //          THREADING RUNTIME            //
    ///////////////////////////////////////////

    struct timespec start, end; 
    clock_gettime(CLOCK_REALTIME, &start);

    // Launch all threads
    for (int i = 0; i < totalThreads; i++)
        pthread_create(&tid[i], NULL, (void*)imProcess, &th[i]);

    // Collect all threads launched
    for (int i = 0; i < totalThreads; i++)
        pthread_join(tid[i], NULL);

    // Save the amount of time spent on processing the image
    clock_gettime(CLOCK_REALTIME, &end);
    results->time_spent = (end.tv_sec - start.tv_sec) + 
                          (end.tv_nsec - start.tv_nsec) / 1e9;
    


    ///////////////////////////////////////////
    //        COLLECTING RESULTS             //
    ///////////////////////////////////////////

    // Unpad the image for cases of thread amount & height mismatch
    if (paddedSize != -1 && paddedSize != height && totalThreads != 1) 
        imRemovePadding(srcIMG, dctIMG, idctIMG, (paddedSize - height));

    // """Manual verification"""
    // (Yes, this needs sarcastic triple-quotes)
    /*
        printf("Original Image:\n");
        imPrint(srcIMG);
        printf("\n\n");
        
        printf("DCT Image:\n");
        imPrint(dctIMG);
        printf("\n\n");

        printf("IDCT Image:\n");
        imPrint(idctIMG);
        printf("\n\n");
    */

    // Collect information about the error generated in processing
    int DOP             = 12;
    double precision    = (double)(1.0/(pow(10.0, (double)DOP)));
    results->validation = imValidate(srcIMG, idctIMG, precision);
    results->err1       = imERR1(srcIMG, idctIMG);
    results->err2       = imERR2(srcIMG, idctIMG);
    results->err3       = imMSE(srcIMG, idctIMG);

    // NOTE: If you don't delete the picture before returning 
    //       this whole thing leaks memory like a damn seive
    //
    imDelete(srcIMG, dctIMG, idctIMG);
    return results;
}


void* imProcess(void* arg) {

    // Parse the argument into a local (struct info*) structure
    threadInfo* input = (threadInfo*)arg;

    // Iterate through the rows corrosponding to this thread
    for (int y = input->start; (y < input->end) && (y < input->padIndex); y += 8) {

        // Iterate through the columns corrosponding to this thread
        for (int x = 0, width = input->srcIMG->width; x < width; x += 8) {

            // Perform a DCT -> IDCT on a 8x8 macroblock
            // centered at the point (x, y)
            imBlockDCT(input->srcIMG, input->dctIMG, x, y);
            imBlockIDCT(input->dctIMG, input->idctIMG, x, y);
        }
    }
}


/*
    Perform the DCT algorithm over an 8x8 block in the image starting 
    at the point inIMG[i][j] and ending at inIMG[i+8][j+8]
*/
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


/*
    Perform the inverse DCT algorithm over an 8x8 block in the image
    starting at the point inIMG[i][j] and ending at inIMG[i+8][j+8]
*/
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


/*
    Perform the DCT algorithm over an entire image; 
    
    This loses accuracy over larger and larger image sizes 
    past an 8x8 
*/
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


/*
    Perform the inverse DCT algorithm over an entire image; 
    
    This loses accuracy over larger and larger image sizes 
    past an 8x8 
*/
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


/*
    Print an image to STDOUT
*/
void imPrint(image* im) {

    // If input image is a grayscale image
    if (im->channels == 1) {
        for (int y = 0; y < im->height; y++) {
            for (int x = 0; x < im->width; x++) {

                // NOTE: This should be a %X.Yf, where
                //       X = Y+5 for spacing values and 
                //       the sign of the values neatly
                //
                printf("[%8.3f] ", im->m[x][y].i);
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


/*
    Get the amount of average error per ELEMENT of two images;

    This is helpful in illustrating the percision of doubles
    as you +/- the percision in imValidate()
*/
double imERR1(image* imA, image* imB) {
    
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


/*
    Get the amount of average error over the entirety of two images
*/
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


/*
    Get the mean-squared-error of two images
*/
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


/*
    Check wether two images are the same given a 
    particular amount of precision
*/
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


/*
    Allocate space for a (width x height) size image and 
    return it's pointer
*/
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


/*
    Randomly generate a (width x height) size image and 
    return it's pointer
*/
image* generateImage(int width, int height, int channels, int padAmnt) {

    // Allocate the image structure
    image* im = allocateImage(width, (height + padAmnt), channels);

    // Randomly generate values at each pixel
    srand((unsigned)time(NULL));
    for (int y = 0; y < height; y++)
        for (int x = 0; x < width; x++)
            im->m[x][y].i = (double)(rand() % 255);

    // Initialize all padded area to 0's
    for (int y = height, sz = (height + padAmnt); y < sz; y++)
        for (int x = 0; x < width; x++)
            im->m[x][y].i = 0;

    return im;
}


/*
    Return a height that an image should have with extra padding;

    This number is determined by the starting (minimum) size
    and the amount of threads used in the process
*/
int imGetPadSize(int totalThreads, int startingSize) {
    int found = 0, sizeFound = 0;
    for (int s = startingSize; s < 15000; s++) {  
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


/*
    Remove padding from an image and reset it's variables 
    for dimensions based on the padding amount removed
*/
void imRemovePadding(image* srcIMG, image* dctIMG, image* idctIMG, int padAmnt) {
    for (int y = srcIMG->height, nSize = (srcIMG->height - padAmnt); y >= nSize; y--) {
        free(srcIMG->m[y]);
        free(dctIMG->m[y]);
        free(idctIMG->m[y]);
    }
    srcIMG->height  = (srcIMG->height  - padAmnt);
    dctIMG->height  = (dctIMG->height  - padAmnt);
    idctIMG->height = (idctIMG->height - padAmnt);
}


/*
    Read a file into an image structure
*/
void imread(image* im, FILE *inFile) {

    // If input image is a grayscale image
    if (im->channels == 1) {
        int value;
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


/*
    Write an image structure out to a file
*/
void imwrite(image* im, char* fileName) {
    FILE *inFile = fopen(fileName, "w+");
    fprintf(inFile, "P2\n%i %i\n255\n", im->height, im->width);

    for (int y = 0; y < im->height; y++) {
        for (int x = 0; x < im->width; x++) {
            if (im->m[x][y].i >= (valueType)0.0)
                fprintf(inFile, "%.0f%s", im->m[x][y].i, 
                          (x+1 == im->width? "\n":" "));

            else
                fprintf(inFile, "0%s", (x+1 == im->width? "\n":" "));
        }
    }
}


/*
    Delete an image from memory
*/
void imDelete(image* srcIMG, image* dctIMG, image* idctIMG) {
    for (int y = 0; y < srcIMG->height; y++) {
        free(srcIMG->m[y]);
        free(dctIMG->m[y]);
        free(idctIMG->m[y]);
    }
}