int seam_carving(int resize, int threadNumber) {
  unsigned char *sobelImg = sobel(threadNumber);
  unsigned char *writePtr, *readPtr, *p = imageMetaData[threadNumber].pixelData;
  int *energyMap = malloc(sizeof(int) * imageMetaData[threadNumber].height * imageMetaData[threadNumber].width);
  int* seam = malloc(sizeof(int) * imageMetaData[threadNumber].height);
  int x __attribute__((aligned(32))), y __attribute__((aligned(32))), z, *energyPtr, width __attribute__((aligned(32))) = imageMetaData[threadNumber].width;
  register int r;

  while(imageMetaData[threadNumber].width > resize) { //main seam removal loop

    energyPtr = &energyMap[width * (imageMetaData[threadNumber].height - 1)];
    readPtr = &sobelImg[width * (imageMetaData[threadNumber].height - 1)];
    r = width;
    while(r--) //copy bottom row from sobelImg to energy map
      *energyPtr++ = *readPtr++;

    for(int i = imageMetaData[threadNumber].height - 2; i >= 0; i--) { //build energy map, bottom-up
      x = width * i;
      y = x + width;
      energyMap[x] = sobelImg[x] + ((energyMap[y+1] < energyMap[y]) ? energyMap[y+1] : energyMap[y]); //left edge
      energyMap[y-1] = sobelImg[y-1] + ((energyMap[y+width-2] < energyMap[y+width-1]) ? energyMap[y+width-2] : energyMap[y+width-1]); //right edge
      for(int j = width - 2; j > 0; j--) {
        z = (energyMap[y+j-1] < energyMap[y+j]) ? energyMap[y+j-1] : energyMap[y+j];
        energyMap[x+j] = sobelImg[x+j] + ((energyMap[y+j+1] < z) ? energyMap[y+j+1] : z);
      }
    }

    /* start seam */
    r = 0;
    for(int i = 1; i < width; i++) //find lowest energy starting point in first row
      r = (energyMap[i] < energyMap[r]) ? i : r; //idx pos
    y = seam[0] = r;

    for(int i = 1; i < imageMetaData[threadNumber].height; i++) { //trace seam on energy map
      x = y + width;
      seam[i] = width;
      if(x == width * i) //left edge
        seam[i] += energyMap[x+1] < energyMap[x];
      else if(x == width * (i+1) - 1) //right edge
        seam[i] -= energyMap[x-1] < energyMap[x];
      else { //middle (not a side edge)
        r = (energyMap[x-1] < energyMap[x]) ? energyMap[x-1] : energyMap[x]; //relative advancement
        if(energyMap[x+1] < r) seam[i] += 1;
        else seam[i] -= energyMap[x-1] < energyMap[x];
      }
      y += seam[i]; //seam total (for finding the tail)
    }

    /* img array compacting */

    //filter/gradient img
    writePtr = readPtr = &sobelImg[seam[0]]; //trim seam from sobel filtered img
    readPtr++;
    for(int i = 1; i < imageMetaData[threadNumber].height; i++) { //compact array, deleting seam
      r = seam[i] - 1;
      memcpy(writePtr, readPtr, r);
      writePtr += r;
      readPtr += r + 1;
    }
    r = imageMetaData[threadNumber].height * width - y - 1; //last half of last line
    while(r--) //compact the tail
      *writePtr++ = *readPtr++;

    /* compact src img */

    writePtr = readPtr = &p[seam[0] * imageMetaData[threadNumber].colorspace]; //trim seam from src img
    readPtr += imageMetaData[threadNumber].colorspace;
    for(int i = 1; i < imageMetaData[threadNumber].height; i++) { //compact array, deleting seam
      r = (seam[i] - 1) * imageMetaData[threadNumber].colorspace;
      memcpy(writePtr, readPtr, r);
      writePtr += r;
      readPtr += r + imageMetaData[threadNumber].colorspace;
    }
    r = (imageMetaData[threadNumber].height * width - y - 1) * imageMetaData[threadNumber].colorspace; //last half of last line
    while(r--) //compact the tail
      *writePtr++ = *readPtr++;

    width = --imageMetaData[threadNumber].width;
  } //end seam carving loop

  free(seam);
  free(energyMap);
  free(sobelImg);
  return 0;
}

