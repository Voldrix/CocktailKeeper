unsigned char* rgb_to_monochrome(unsigned char *rgbImg, int weighted, int threadNumber) { //RGB(A) -> Mono
  unsigned char *monoImg = malloc(imageMetaData[threadNumber].width * imageMetaData[threadNumber].height);
  int m = 0, n = 0;
  register int r = imageMetaData[threadNumber].width * imageMetaData[threadNumber].height;

  //weighted averaging better matches human perception. Human eyes perceive green more strongly than red, and red more strongly than blue
  //the standard weights are about .3R + .6G + .1B
  //the weights here are adusted to make them faster to compute. The difference is minimal
  if(imageMetaData[threadNumber].colorspace == 3) { //RGB
    if(weighted)
      while(r--)
        monoImg[m++] = (rgbImg[n++] >> 2) + (rgbImg[n++] >> 1) + (rgbImg[n++] >> 2);
    else  //simple average method
      while(r--)
        monoImg[m++] = (rgbImg[n++] + rgbImg[n++] + rgbImg[n++]) / 3;
  }
  else { //RGBA
    if(weighted) //weighted averaging
      while(r--)
        monoImg[m++] = ((rgbImg[n++] >> 2) + (rgbImg[n++] >> 1) + (rgbImg[n++] >> 2)) * (rgbImg[n++] / 255);
    else //simple average method
      while(r--)
        monoImg[m++] = ((rgbImg[n++] + rgbImg[n++] + rgbImg[n++]) / 3) * (rgbImg[n++] / 255);
  }
  return monoImg;
}



unsigned char* monochrome_to_rgb(unsigned char *monoImg, int threadNumber) { //Mono -> RGB
  unsigned char *rgbImg = malloc(imageMetaData[threadNumber].width * imageMetaData[threadNumber].height * 3);
  int m = 0, n = 0;
  register int r = imageMetaData[threadNumber].width * imageMetaData[threadNumber].height;
  while(r--)
      rgbImg[n++] = rgbImg[n++] = rgbImg[n++] = monoImg[m++];
  return rgbImg;
}



unsigned char* transpose_monochrome(unsigned char *img, int threadNumber) { //X,Y -> Y,X Mono
  if(!img) return 0;
  int w = imageMetaData[threadNumber].width, h = imageMetaData[threadNumber].height;
  unsigned char *transposed = malloc(w * h);

  for(int r = 0; r < h; r++) {
    for(int c = 0; c < w; c++) {
      transposed[c * h + r] = img[r * w + c];
    }
  }
  free(img);
  return transposed;
}



void transpose_image(int threadNumber) { //X,Y -> Y,X RGB(A)
  int w = imageMetaData[threadNumber].width, h = imageMetaData[threadNumber].height;
  unsigned char *transposed = malloc(w * h * imageMetaData[threadNumber].colorspace);

  if(imageMetaData[threadNumber].colorspace == 3) { //RGB
    register int x,y;
    for(int r = 0; r < h; r++) {
      for(int c = 0; c < w; c++) {
        x = 3*w * r + 3*c;
        y = 3*h * c + 3*r;
        transposed[y] = imageMetaData[threadNumber].pixelData[x];
        transposed[y + 1] = imageMetaData[threadNumber].pixelData[x + 1];
        transposed[y + 2] = imageMetaData[threadNumber].pixelData[x + 2];
      }
    }
  }

  if(imageMetaData[threadNumber].colorspace == 4) { //RGBA
    int *p = (int*)imageMetaData[threadNumber].pixelData;
    int *t = (int*)transposed;
    for(int r = 0; r < h; r++) {
      for(int c = 0; c < w; c++) {
        t[c * h + r] = p[r * w + c];
      }
    }
  }
  imageMetaData[threadNumber].width = h;
  imageMetaData[threadNumber].height = w;
  free(imageMetaData[threadNumber].pixelData);
  imageMetaData[threadNumber].pixelData = transposed;
}



unsigned char* sobel(int threadNumber) {
  unsigned char (*p)[imageMetaData[threadNumber].width * imageMetaData[threadNumber].colorspace] = (unsigned char (*)[imageMetaData[threadNumber].width * imageMetaData[threadNumber].colorspace])imageMetaData[threadNumber].pixelData; //Multidimensional array pointer, pointing to a single dimension array
  unsigned char (*gradient)[imageMetaData[threadNumber].width] = malloc(imageMetaData[threadNumber].height * imageMetaData[threadNumber].width); //Final filtered image, single channel
  int x, y, z, w = imageMetaData[threadNumber].width, h = imageMetaData[threadNumber].height;

  //greyscale copy of src img
  unsigned char (*grayscale)[imageMetaData[threadNumber].width] = (unsigned char (*)[imageMetaData[threadNumber].width])rgb_to_monochrome(imageMetaData[threadNumber].pixelData, UNWEIGHTED_AVG, threadNumber);

  //start sobel convolution (x & y)

  //corners
  x = (p[0][1] - p[0][0] + p[1][1] - p[1][0]) >> 2;
  y = (p[1][0] - p[0][0] + p[1][1] - p[0][1]) >> 2;
  gradient[0][0] = sqrt(x*x + y*y); //TL

  x = (p[0][w-1] - p[0][w-2] + p[1][w-1] - p[1][w-2]) >> 2;
  y = (p[1][w-1] - p[0][w-1] + p[1][w-2] - p[0][w-2]) >> 2;
  gradient[0][w-1] = sqrt(x*x + y*y); //TR

  x = (p[h-2][1] - p[h-2][0] + p[h-1][1] - p[h-1][0]) >> 2;
  y = (p[h-1][0] - p[h-2][0] + p[h-1][1] - p[h-2][1]) >> 2;
  gradient[h-1][0] = sqrt(x*x + y*y); //BL

  x = (p[h-2][w-1] - p[h-2][w-2] + p[h-1][w-1] - p[h-1][w-2]) >> 2;
  y = (p[h-1][w-1] - p[h-2][w-1] + p[h-1][w-2] - p[h-2][w-2]) >> 2;
  gradient[h-1][w-1] = sqrt(x*x + y*y); //BR

  for(int j = 1; j < imageMetaData[threadNumber].width - 1; j++) {
    //top row
    x = grayscale[1][j+1] - grayscale[1][j-1] + 2 * grayscale[0][j+1] - 2 * grayscale[0][j-1];
    y = grayscale[1][j-1] + grayscale[1][j+1] - grayscale[0][j-1] - grayscale[0][j+1] + 2 * grayscale[1][j] - 2 * grayscale[0][j];
    x /= 6;
    y >>= 3;
    z = sqrt(x*x + y*y);
    gradient[0][j] = (z > 255) ? 255 : z;
    //bottom row
    x = grayscale[imageMetaData[threadNumber].height-2][j+1] - grayscale[imageMetaData[threadNumber].height-2][j-1] + 2 * grayscale[imageMetaData[threadNumber].height-1][j+1] - 2 * grayscale[imageMetaData[threadNumber].height-1][j-1];
    y = grayscale[imageMetaData[threadNumber].height-1][j-1] + grayscale[imageMetaData[threadNumber].height-1][j+1] - grayscale[imageMetaData[threadNumber].height-2][j-1] - grayscale[imageMetaData[threadNumber].height-2][j+1] + 2 * grayscale[imageMetaData[threadNumber].height-1][j] - 2 * grayscale[imageMetaData[threadNumber].height-2][j];
    x /= 6;
    y >>= 3;
    z = sqrt(x*x + y*y);
    gradient[imageMetaData[threadNumber].height-1][j] = (z > 255) ? 255 : z;
  }

  for(int i = 1; i < imageMetaData[threadNumber].height - 1; i++) {
    //left edge
    x = grayscale[i-1][1] + grayscale[i+1][1] - grayscale[i-1][0] - grayscale[i+1][0] + 2 * grayscale[i][1] - 2 * grayscale[i][0];
    y = grayscale[i+1][1] - grayscale[i-1][1] + 2 * grayscale[i+1][0] - 2 * grayscale[i-1][0];
    x >>= 3;
    y /= 6;
    z = sqrt(x*x + y*y);
    gradient[i][0] = (z > 255) ? 255 : z;
    //right edge
    x = grayscale[i-1][imageMetaData[threadNumber].width-1] + grayscale[i+1][imageMetaData[threadNumber].width-1] - grayscale[i-1][imageMetaData[threadNumber].width-2] - grayscale[i+1][imageMetaData[threadNumber].width-2] + 2 * grayscale[i][imageMetaData[threadNumber].width-1] - 2 * grayscale[i][imageMetaData[threadNumber].width-2];
    y = grayscale[i+1][imageMetaData[threadNumber].width-2] - grayscale[i-1][imageMetaData[threadNumber].width-2] + 2 * grayscale[i+1][imageMetaData[threadNumber].width-1] - 2 * grayscale[i-1][imageMetaData[threadNumber].width-1];
    x >>= 3;
    y /= 6;
    z = sqrt(x*x + y*y);
    gradient[i][imageMetaData[threadNumber].width-1] = (z > 255) ? 255 : z;
    //rest of the row
    for(int j = 1; j < imageMetaData[threadNumber].width - 1; j++) {
      x = grayscale[i-1][j+1] + grayscale[i+1][j+1] - grayscale[i-1][j-1] - grayscale[i+1][j-1] + 2 * grayscale[i][j+1] - 2 * grayscale[i][j-1];
      y = grayscale[i+1][j-1] + grayscale[i+1][j+1] - grayscale[i-1][j-1] - grayscale[i-1][j+1] + 2 * grayscale[i+1][j] - 2 * grayscale[i-1][j];
      x >>= 3;
      y >>= 3;
      z = sqrt(x*x + y*y);
      gradient[i][j] = (z > 255) ? 255 : z;
    }
  }

  free(grayscale);
  return (unsigned char *)gradient;
}

