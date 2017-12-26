#include <assert.h>
#include <time.h>

#include "vibe-background-sequential.h"

#define NUMBER_OF_HISTORY_IMAGES 25


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
  model->matchingThreshold       = 10;
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

// -----------------------------------------------------------------------------
int32_t libvibeModel_Sequential_SetUpdateFactor(
  vibeModel_Sequential_t *model,
  const uint32_t updateFactor
) {
  assert(model != NULL);
  assert(updateFactor > 0);

  model->updateFactor = updateFactor;

  /* We also need to change the values of the jump buffer ! */
  assert(model->jump != NULL);

  /* Shifts. */
  int size = (model->width > model->height) ? 2 * model->width + 1 : 2 * model->height + 1;

  for (int i = 0; i < size; ++i)
    model->jump[i] = (updateFactor == 1) ? 1 : (rand() % (2 * model->updateFactor)) + 1; // 1 or values between 1 and 2 * updateFactor.

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

// -----------------------------------------------------------------------------
// Allocates and initializes a C1R model structure
// -----------------------------------------------------------------------------
int32_t libvibeModel_Sequential_AllocInit_8u_C1R(
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
  model->historyImage = (uint8_t*)malloc(NUMBER_OF_HISTORY_IMAGES * width * height * sizeof(*(model->historyImage)));

  assert(model->historyImage != NULL);

  for (int i = 0; i < NUMBER_OF_HISTORY_IMAGES; ++i) {
    for (int index = width * height - 1; index >= 0; --index)
      model->historyImage[i * width * height + index] = image_data[index];
  }

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
    model->neighbor[i] = ((rand() % 3) - 1) + ((rand() % 3) - 1) * width; // Values between { -width - 1, ... , width + 1 }.
    model->position[i] = rand() % (model->numberOfSamples);               // Values between 0 and numberOfSamples - 1.
  }

  return(0);
}

// -----------------------------------------------------------------------------
// Segmentation of a C1R model
// -----------------------------------------------------------------------------
int32_t libvibeModel_Sequential_Segmentation_8u_C1R(
  vibeModel_Sequential_t *model,
  const uint8_t *image_data,
  uint8_t *segmentation_map,
  uint8_t *t
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
  memset(segmentation_map, matchingNumber - 1, width * height);

  memset(t, 0, width * height);

  /* First history Image structure. */
  for (int index = width * height - 1; index >= 0; --index) {
    if ((image_data[index] - historyImage[index]) > matchingThreshold)
      segmentation_map[index] = matchingNumber;
    t[index]=historyImage[index];
  }

  /* Next historyImages. */
  for (int i = 1; i < NUMBER_OF_HISTORY_IMAGES; ++i) {
    uint8_t *pels = historyImage + i * width * height;

    for (int index = width * height - 1; index >= 0; --index) {
      if ((image_data[index] - pels[index]) <= matchingThreshold)
        --segmentation_map[index];
      t[index]+=pels[index];
    }
  }


  for (int index = width * height - 1; index >= 0; --index) 
       if ((int)(t[index]/model->numberOfSamples)+matchingThreshold<image_data[index]) segmentation_map[index]=1;
       else segmentation_map[index]=0; 
  /* Produces the output. Note that this step is application-dependent. */
  for (uint8_t *mask = segmentation_map; mask < segmentation_map + (width * height); ++mask)
    if (*mask > 0) *mask = COLOR_FOREGROUND;

  return(0);
}

// ----------------------------------------------------------------------------
// Update a C1R model
// ----------------------------------------------------------------------------
int32_t libvibeModel_Sequential_Update_8u_C1R(
  vibeModel_Sequential_t *model,
  const uint8_t *image_data,
  uint8_t *updating_mask
) {
  /* Basic checks . */
  assert((image_data != NULL) && (model != NULL) && (updating_mask != NULL));
  assert((model->width > 0) && (model->height > 0));
  assert((model->jump != NULL) && (model->neighbor != NULL) && (model->position != NULL));

  /* Some variables. */
  uint32_t width = model->width;
  uint32_t height = model->height;

  uint8_t *historyImage = model->historyImage;

  /* Some utility variable. */

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
        uint8_t value = image_data[index];
        int index_neighbor = index + neighbor[shift];

        if (position[shift] < NUMBER_OF_HISTORY_IMAGES) {
          historyImage[index + position[shift] * width * height] = value;
          historyImage[index_neighbor + position[shift] * width * height] = value;
        }
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

    if (updating_mask[index] == COLOR_BACKGROUND) {
      if (position[shift] < NUMBER_OF_HISTORY_IMAGES)
        historyImage[index + position[shift] * width * height] = image_data[index];
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

    if (updating_mask[index] == COLOR_BACKGROUND) {
      if (position[shift] < NUMBER_OF_HISTORY_IMAGES)
        historyImage[index + position[shift] * width * height] = image_data[index];
     
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

    if (updating_mask[index] == COLOR_BACKGROUND) {
      if (position[shift] < NUMBER_OF_HISTORY_IMAGES)
        historyImage[index + position[shift] * width * height] = image_data[index];
     
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

    if (updating_mask[index] == COLOR_BACKGROUND) {
      if (position[shift] < NUMBER_OF_HISTORY_IMAGES )
        historyImage[index + position[shift] * width * height] = image_data[index];
    }
    ++shift; 
    indY += jump[shift];
  }

  /* The first pixel! */
  if (rand() % model->updateFactor == 0) {
    if (updating_mask[0] == 0) {
      int position = rand() % model->numberOfSamples;

      if (position < NUMBER_OF_HISTORY_IMAGES)
        historyImage[position * width * height] = image_data[0];    
    }
  }

  return(0);
}

