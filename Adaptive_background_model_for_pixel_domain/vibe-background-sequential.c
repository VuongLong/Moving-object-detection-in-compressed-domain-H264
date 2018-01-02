/** 
    @file vibe-background-sequential.c
    @brief Implementation of vibe-background-sequential.h

    @author Marc Van Droogenbroeck

    @date May 2014
*/

/*
There are a few implementation "tricks" [by Marc Van Droogenbroeck]. 
Please note that THEY DON'T AFFECT THE BEHAVIOR OF ViBe but help speeding up the implementation; they are described hereafter. 

1. Swapping values in order to bring best matching candidates in the first places of the memory
 
The idea is that ViBe only needs two matches to classify a pixel into the background; to some extent, the 18 other pixels (out of N=20 pixels) are then useless. In addition, most of the time, there are no big changes between pixels located at the same place from successive images (90% of the pixels are in the background on average). Therefore, I introduced the mechanism of pixels swapping: if the algorithm finds two pixel matches, inside the collection of 20 pixels, at positions 8 and 16 for example, it should ideally bring these pixels at locations 0 and 1, by swapping 8 and 0, and 16 and 1, in the model. Then, for the next frame, the algorithm has a high probability to find the two matches at position 0 and 1, and to skip 18 tests. 


2. Organization of the memory for storing past sample values: use of two "historyImage"

By using trick 1, it appears that in all the cases the algorithm will check the two first pixel values, but rarely for the next 18 ones. That knowledge is then usable to increase the speed of the implementation. So, the memory is split into two "historyImages" that mimic the memory structure of an image (the number of historyImages is equal to the number of required matches). The 18 remaining sample values are stored in a general buffer. The idea is that the compiler is able to parallelize automatically the tests on the historyImages. This is an optimization trick specific to the "sequential" version of ViBe. 


3. Initialization of the sample values

The original article (IEEE Transactions on Image Processing, June 2011) proposes to initialize the model with sample values taken in the neighborhood. In this implementation, we choose to fill the initial model with the value of the current pixel plus some noise (see  int value_plus_noise = value + rand() % 20 - 10; ). 
In fact, there are several ways to consider the problem of initialization: 
A. use the original mechanism of ViBe (see patents and article)
B. use the mechanism proposed in this file (good compromise between speed and adaptability)
C. use a mechanism better tuned to your application. For example, if you have some prior knowledge about the scence, it might be more appropriate to select sample values that fit the needs of your application. For example, if you want to reset the model on the run because of an illumination change, it might be suitable to keep two original values (so to recover rapidly if needed). These mechanisms should be tailored to the application. 


4. Pre-compute all the random numbers and decision + store the results in buffers + use relative offsets

The reason for modifications related to random numbers is that the generation of random numbers is time consumming. There are several techniques to pre-compute random numbers, and the implementation reflects some of techniques that are working well (from a speed of processing point of view). These mechanisms include: 

* A. Pre-computing random numbers. 
Computing "if ((rand(16))%16 == 0) then update" is simply a random boolean (with a probability of updating of 1/16). 
So, instead of doing one random test per pixel, it is possible to pre-store the results of this random tests. 

* B. Transform successive random binary tests in jumps. 
If you take a decision to update a pixel for 1 out of 16 (== updatingFactor) pixels, you could skip 15 tests on average [note that this is about one test per pixel!]. So, I changed the mechanism of checking each pixel on turn for an update by randomly "hoping" from the current pixel location to the next pixel to be updated. The important property to be preserved is that 1 out of 16 pixels is updated on average.  

* C. Selection of one neighboring model to be updated. 
Likewise, instead of a random selection of the neighboring model to be updated, the implementation pre-stores the relative offset of the neighbor to be selected.  
*/
#include <stdio.h>
#include <assert.h>
#include <time.h>

#include "vibe-background-sequential.h"

#define NUMBER_OF_HISTORY_IMAGES 20

static inline int abs_uint(const int i)
{
  return (i >= 0) ? i : -i;
}

static inline int32_t distance_is_close_8u_C3R(uint8_t r1, uint8_t g1, uint8_t b1, uint8_t r2, uint8_t g2, uint8_t b2, uint32_t threshold)
{
  return (abs_uint(r1 - r2) + abs_uint(g1 - g2) + abs_uint(b1 - b2) <= 4.5 * threshold);
}

struct vibeModel_Sequential
{
  /* Parameters. */
  uint32_t width;
  uint32_t height;
  uint32_t numberOfSamples;
  uint32_t matchingThreshold;
  uint32_t matchingNumber;
  uint32_t updateFactor;

  /* Storage for the history. */
  uint8_t *historyImage;
  uint32_t lastHistoryImageSwapped;

  /* Buffers with random values. */
  uint32_t *jump;
  int *neighbor;
  uint32_t *position;
};

// -----------------------------------------------------------------------------
// Print parameters
// -----------------------------------------------------------------------------
uint32_t libvibeModel_Sequential_PrintParameters(const vibeModel_Sequential_t *model)
{
  printf(
    "Using ViBe background subtraction algorithm\n"
      "  - Number of samples per pixel:       %03d\n"
      "  - Number of matches needed:          %03d\n"
      "  - Matching threshold:                %03d\n"
      "  - Model update subsampling factor:   %03d\n",
    libvibeModel_Sequential_GetNumberOfSamples(model),
    libvibeModel_Sequential_GetMatchingNumber(model),
    libvibeModel_Sequential_GetMatchingThreshold(model),
    libvibeModel_Sequential_GetUpdateFactor(model)
  );

  return(0);
}

// -----------------------------------------------------------------------------
// Creates the data structure
// -----------------------------------------------------------------------------
vibeModel_Sequential_t *libvibeModel_Sequential_New()
{
  /* Model structure alloc. */
  vibeModel_Sequential_t *model = NULL;
  model = (vibeModel_Sequential_t*)calloc(1, sizeof(*model));
  assert(model != NULL);

  /* Default parameters values. */
  model->numberOfSamples         = NUMBER_OF_HISTORY_IMAGES;
  model->matchingThreshold       = 20;
  model->matchingNumber          = 2;
  model->updateFactor            = 16;

  /* Storage for the history. */
  model->historyImage            = NULL;
  model->lastHistoryImageSwapped = 0;

  /* Buffers with random values. */
  model->jump                    = NULL;
  model->neighbor                = NULL;
  model->position                = NULL;

  return(model);
}

// -----------------------------------------------------------------------------
// Some "Get-ers"
// -----------------------------------------------------------------------------
uint32_t libvibeModel_Sequential_GetNumberOfSamples(const vibeModel_Sequential_t *model)
{
  assert(model != NULL); return(model->numberOfSamples);
}

uint32_t libvibeModel_Sequential_GetMatchingNumber(const vibeModel_Sequential_t *model)
{
  assert(model != NULL); return(model->matchingNumber);
}

uint32_t libvibeModel_Sequential_GetMatchingThreshold(const vibeModel_Sequential_t *model)
{
  assert(model != NULL); return(model->matchingThreshold);
}

uint32_t libvibeModel_Sequential_GetUpdateFactor(const vibeModel_Sequential_t *model)
{
  assert(model != NULL); return(model->updateFactor);
}

// -----------------------------------------------------------------------------
// Some "Set-ers"
// -----------------------------------------------------------------------------
int32_t libvibeModel_Sequential_SetMatchingThreshold(
  vibeModel_Sequential_t *model,
  const uint32_t matchingThreshold
) {
  assert(model != NULL);
  assert(matchingThreshold > 0);

  model->matchingThreshold = matchingThreshold;

  return(0);
}

// -----------------------------------------------------------------------------
int32_t libvibeModel_Sequential_SetMatchingNumber(
  vibeModel_Sequential_t *model,
  const uint32_t matchingNumber
) {
  assert(model != NULL);
  assert(matchingNumber > 0);

  model->matchingNumber = matchingNumber;

  return(0);
}

// ----------------------------------------------------------------------------
// Frees the structure
// ----------------------------------------------------------------------------
int32_t libvibeModel_Sequential_Free(vibeModel_Sequential_t *model)
{
  if (model == NULL)
    return(-1);


  free(model->historyImage);
  free(model->jump);
  free(model->neighbor);
  free(model->position);
  free(model);

  return(0);
}

// ----------------------------------------------------------------------------
// -------------------------- The same for C3R models -------------------------
// ----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// Allocates and initializes a C3R model structure
// -----------------------------------------------------------------------------
int32_t libvibeModel_Sequential_AllocInit_8u_C3R(
  vibeModel_Sequential_t *model,
  const uint8_t *image_data,
  const uint32_t width,
  const uint32_t height
) {
  /* Some basic checks. */
  assert((image_data != NULL) && (model != NULL));
  assert((width > 0) && (height > 0));

  /* Finish model alloc - parameters values cannot be changed anymore. */
  model->width = width;
  model->height = height;

  /* Creates the historyImage structure. */
  model->historyImage = NULL;
  model->historyImage = (uint8_t*)malloc(NUMBER_OF_HISTORY_IMAGES * (3 * width) * height * sizeof(uint8_t));
  assert(model->historyImage != NULL);

  for (int i = 0; i < NUMBER_OF_HISTORY_IMAGES; ++i) {
    for (int index = (3 * width) * height - 1; index >= 0; --index)
      model->historyImage[i * (3 * width) * height + index] = image_data[index];
  }

  assert(model->historyImage != NULL);

  /* Fills the buffers with random values. */
  int size = (width > height) ? 2 * width + 1 : 2 * height + 1;

  model->jump = (uint32_t*)malloc(size * sizeof(*(model->jump)));
  assert(model->jump != NULL);

  model->neighbor = (int*)malloc(size * sizeof(*(model->neighbor)));
  assert(model->neighbor != NULL);

  model->position = (uint32_t*)malloc(size * sizeof(*(model->position)));
  assert(model->position != NULL);

  for (int i = 0; i < size; ++i) {
    model->jump[i] = (rand() % (2 * model->updateFactor)) + 1;            // Values between 1 and 2 * updateFactor.
    model->neighbor[i] = ((rand() % 3) - 1) + ((rand() % 3) - 1) * width; // Values between { width - 1, ... , width + 1 }.
    model->position[i] = rand() % (model->numberOfSamples);               // Values between 0 and numberOfSamples - 1.
  }

  return(0);
}

// -----------------------------------------------------------------------------
// Segmentation of a C3R model
// -----------------------------------------------------------------------------
int32_t libvibeModel_Sequential_Segmentation_8u_C3R(
  vibeModel_Sequential_t *model,
  const uint8_t *image_data,
  uint8_t *segmentation_map
) {
  /* Basic checks. */
  assert((image_data != NULL) && (model != NULL) && (segmentation_map != NULL));
  assert((model->width > 0) && (model->height > 0));
  assert((model->jump != NULL) && (model->neighbor != NULL) && (model->position != NULL));

  /* Some variables. */
  uint32_t width = model->width;
  uint32_t height = model->height;
  uint32_t matchingNumber = model->matchingNumber;
  uint32_t matchingThreshold = model->matchingThreshold;

  uint8_t *historyImage = model->historyImage;

  /* Segmentation. */
  memset(segmentation_map, matchingNumber, width * height);
  for (int i = 0; i < NUMBER_OF_HISTORY_IMAGES; ++i) {
    uint8_t *pels = historyImage + i * (3 * width) * height;

    for (int index = width * height - 1; index >= 0; --index) {
      if (
        distance_is_close_8u_C3R( 
          image_data[3 * index], image_data[3 * index + 1], image_data[3 * index + 2],
          pels[3 * index], pels[3 * index + 1], pels[3 * index + 2], matchingThreshold)
      )
        if (segmentation_map[index]>0) segmentation_map[index]--;
    }
  }
  for (int index = width * height - 1; index >= 0; --index){
    if (segmentation_map[index]>0)
      segmentation_map[index] = COLOR_FOREGROUND;
  }

  /* Produces the output. Note that this step is application-dependent. */
  //for (uint8_t *mask = segmentation_map; mask < segmentation_map + (width * height); ++mask)
  //  if (*mask > 0) *mask = COLOR_FOREGROUND;

  return(0);
}

// ----------------------------------------------------------------------------
// Update a C3R model
// ----------------------------------------------------------------------------
int32_t libvibeModel_Sequential_Update_8u_C3R(
  vibeModel_Sequential_t *model,
  const uint8_t *image_data,
  uint8_t *updating_mask
) {
  /* Basic checks. */
  assert((image_data != NULL) && (model != NULL) && (updating_mask != NULL));
  assert((model->width > 0) && (model->height > 0));
  assert((model->jump != NULL) && (model->neighbor != NULL) && (model->position != NULL));

  /* Some variables. */
  uint32_t width = model->width;
  uint32_t height = model->height;

  uint8_t *historyImage = model->historyImage;


  /* Updating. */
  uint32_t *jump = model->jump;
  int *neighbor = model->neighbor;
  uint32_t *position = model->position;

  /* All the frame, except the border. */
  uint32_t shift, indX, indY;
  int x, y;

  for (y = 1; y < height - 1; ++y) {
    shift = rand() % width;
    indX = jump[shift]; // index_jump should never be zero (> 1).

    while (indX < width - 1) {
      int index = indX + y * width;

      if (updating_mask[index] == COLOR_BACKGROUND) {
        /* In-place substitution. */
        uint8_t r = image_data[3 * index];
        uint8_t g = image_data[3 * index + 1];
        uint8_t b = image_data[3 * index + 2];

        int index_neighbor = 3 * (index + neighbor[shift]);

        historyImage[3 * index + position[shift] * (3 * width) * height    ] = r;
        historyImage[3 * index + position[shift] * (3 * width) * height + 1] = g;
        historyImage[3 * index + position[shift] * (3 * width) * height + 2] = b;

        historyImage[index_neighbor + position[shift] * (3 * width) * height    ] = r;
        historyImage[index_neighbor + position[shift] * (3 * width) * height + 1] = g;
        historyImage[index_neighbor + position[shift] * (3 * width) * height + 2] = b;
      }

      ++shift;
      indX += jump[shift];
    }
  }

  /* First row. */
  y = 0;
  shift = rand() % width;
  indX = jump[shift]; // index_jump should never be zero (> 1).

  while (indX <= width - 1) {
    int index = indX + y * width;

    uint8_t r = image_data[3 * index];
    uint8_t g = image_data[3 * index + 1];
    uint8_t b = image_data[3 * index + 2];

    if (updating_mask[index] == COLOR_BACKGROUND) {
      historyImage[3 * index + position[shift] * (3 * width) * height    ] = r;
      historyImage[3 * index + position[shift] * (3 * width) * height + 1] = g;
      historyImage[3 * index + position[shift] * (3 * width) * height + 2] = b;
    }

    ++shift;
    indX += jump[shift];
  }

  /* Last row. */
  y = height - 1;
  shift = rand() % width;
  indX = jump[shift]; // index_jump should never be zero (> 1).

  while (indX <= width - 1) {
    int index = indX + y * width;

    uint8_t r = image_data[3 * index];
    uint8_t g = image_data[3 * index + 1];
    uint8_t b = image_data[3 * index + 2];

    if (updating_mask[index] == COLOR_BACKGROUND) {
      historyImage[3 * index + position[shift] * (3 * width) * height    ] = r;
      historyImage[3 * index + position[shift] * (3 * width) * height + 1] = g;
      historyImage[3 * index + position[shift] * (3 * width) * height + 2] = b;
    }

    ++shift;
    indX += jump[shift];
  }

  /* First column. */
  x = 0;
  shift = rand() % height;
  indY = jump[shift]; // index_jump should never be zero (> 1).

  while (indY <= height - 1) {
    int index = x + indY * width;

    uint8_t r = image_data[3 * index];
    uint8_t g = image_data[3 * index + 1];
    uint8_t b = image_data[3 * index + 2];

    if (updating_mask[index] == COLOR_BACKGROUND) {
      historyImage[3 * index + position[shift] * (3 * width) * height    ] = r;
      historyImage[3 * index + position[shift] * (3 * width) * height + 1] = g;
      historyImage[3 * index + position[shift] * (3 * width) * height + 2] = b;
    }

    ++shift;
    indY += jump[shift];
  }

  /* Last column. */
  x = width - 1;
  shift = rand() % height;
  indY = jump[shift]; // index_jump should never be zero (> 1).

  while (indY <= height - 1) {
    int index = x + indY * width;

    uint8_t r = image_data[3 * index];
    uint8_t g = image_data[3 * index + 1];
    uint8_t b = image_data[3 * index + 2];

    if (updating_mask[index] == COLOR_BACKGROUND) {
      historyImage[3 * index + position[shift] * (3 * width) * height    ] = r;
      historyImage[3 * index + position[shift] * (3 * width) * height + 1] = g;
      historyImage[3 * index + position[shift] * (3 * width) * height + 2] = b;
    }

    ++shift;
    indY += jump[shift];
  }

  /* The first pixel! */
  if (rand() % model->updateFactor == 0) {
    if (updating_mask[0] == 0) {
      int position = rand() % model->numberOfSamples;

      uint8_t r = image_data[0];
      uint8_t g = image_data[1];
      uint8_t b = image_data[2];

      if (position < NUMBER_OF_HISTORY_IMAGES) {
        historyImage[position * (3 * width) * height    ] = r;
        historyImage[position * (3 * width) * height + 1] = g;
        historyImage[position * (3 * width) * height + 2] = b;
      }
    }
  }

  return(0);
}
