int scale_by_half(int antiAliasing, int threadNumber) {
  //rounds down
  unsigned char (*p)[imageMetaData[threadNumber].width * imageMetaData[threadNumber].colorspace] = (unsigned char (*)[imageMetaData[threadNumber].width * imageMetaData[threadNumber].colorspace])imageMetaData[threadNumber].pixelData;
  unsigned char *s = malloc((imageMetaData[threadNumber].width * imageMetaData[threadNumber].height * imageMetaData[threadNumber].colorspace) >> 2);
  int z = 0, w = imageMetaData[threadNumber].width, h = imageMetaData[threadNumber].height, c = imageMetaData[threadNumber].colorspace, oddColumns = w & 1, oddRows = h & 1;
  register int r;

  if(antiAliasing == 0) { //4 block avg
    for(int i = 0; i < h - 1; i+=2) {
      for(int j = 0; j < (w - 1) * c; j += c*2) {
        for(int k = 0; k < c; k++) {
          r = j+k;
          s[z++] = (p[i][r] + p[i][r+c] + p[i+1][r] + p[i+1][r+c]) >> 2;
        }
      }
    }
  }
  else { //9 block weighted avg
    for(int i = 0; i < h - 2; i+=2) {
      for(int j = 0; j < (w - 2) * c; j += c*2) {
        for(int k = 0; k < c; k++) {
          r = j+k;
          s[z++] = (p[i][r] + p[i][r+c] + p[i][r+c+c] + p[i+1][r] + 5 * p[i+1][r+c] + (p[i+1][r+c+c] << 1) + p[i+2][r] + (p[i+2][r+c] << 1) + (p[i+2][r+c+c] << 1)) >> 4;
          //slightly weaker anti-aliasing
          //s[z++] = (p[i][r] + p[i][r+c] + p[i][r+c+c] + p[i+1][r] + (p[i+1][r+c] << 3) + p[i+1][r+c+c] + p[i+2][r] + p[i+2][r+c] + p[i+2][r+c+c]) >> 4;
        }
      }
      for(int j = w*c - c*(1-oddColumns); j < w*c; j++) { //last column, only if even columns
        s[z++] = (p[i][j-c] + p[i][j] + p[i+1][j-c] + p[i+1][j]) >> 2;
      }
    }
    for(int i = w*c*oddRows; i < (w-2)*c; i+=c*2) { //last row, only if even rows
      for(int j = 0; j < c; j++) {
        s[z++] = (p[h-2][i+j] + p[h-2][i+j+c] + p[h-2][i+j+c+c] + p[h-1][i+j] + 3 * p[h-1][i+j+c] + p[h-1][i+j+c+c]) >> 3;
      }
    }
    if(!(oddRows | oddColumns)) { //last pixel, only if even rows AND even columns
      for(int i = w*c-c; i < w*c; i++) {
        s[z++] = (p[h-2][i-c] + p[h-2][i] + p[h-1][i-c] + p[h-1][i]) >> 2;
      }
    }
  }

  imageMetaData[threadNumber].height >>= 1;
  imageMetaData[threadNumber].width >>= 1;
  free(imageMetaData[threadNumber].pixelData);
  imageMetaData[threadNumber].pixelData = s;
  return 0;
}



int scale_by_third(int threadNumber) {
  //rounds down
  unsigned char (*p)[imageMetaData[threadNumber].width * imageMetaData[threadNumber].colorspace] = (unsigned char (*)[imageMetaData[threadNumber].width * imageMetaData[threadNumber].colorspace])imageMetaData[threadNumber].pixelData;
  unsigned char *s = malloc(imageMetaData[threadNumber].width * imageMetaData[threadNumber].height * imageMetaData[threadNumber].colorspace / 9);
  int z = 0, w = imageMetaData[threadNumber].width, h = imageMetaData[threadNumber].height, c = imageMetaData[threadNumber].colorspace;
  register int r;

  for(int i = 0; i < h - 2; i+=3) {
    for(int j = 0; j < (w - 2) * c; j += c*3) {
      for(int k = 0; k < c; k++) {
        r = j+k;
        s[z++] = (p[i][r] + p[i][r+c] + p[i][r+c+c] + p[i+1][r] + (p[i+1][r+c] << 3) + p[i+1][r+c+c] + p[i+2][r] + p[i+2][r+c] + p[i+2][r+c+c]) >> 4;
      }
    }
  }

  imageMetaData[threadNumber].height /= 3;
  imageMetaData[threadNumber].width /= 3;
  free(imageMetaData[threadNumber].pixelData);
  imageMetaData[threadNumber].pixelData = s;
  return 0;
}



int scale_by_quarter(int threadNumber) {
  //rounds down
  unsigned char (*p)[imageMetaData[threadNumber].width * imageMetaData[threadNumber].colorspace] = (unsigned char (*)[imageMetaData[threadNumber].width * imageMetaData[threadNumber].colorspace])imageMetaData[threadNumber].pixelData;
  unsigned char *s = malloc((imageMetaData[threadNumber].width * imageMetaData[threadNumber].height * imageMetaData[threadNumber].colorspace) >> 2);
  int z = 0, w = imageMetaData[threadNumber].width, h = imageMetaData[threadNumber].height, c = imageMetaData[threadNumber].colorspace;
  register int r;

  for(int i = 0; i < h - 3; i+=4) {
    for(int j = 0; j < (w - 3) * c; j += c*4) {
      for(int k = 0; k < c; k++) {
        r = j+k+c;
        s[z++] = ((p[i][r-c]   >> 1) + (p[i][r]   >> 2) + (p[i][r+c]   >> 2) + (p[i][r+c+c]   >> 1) +\
                  (p[i+1][r-c] >> 2) +  p[i+1][r]       +  p[i+1][r+c]       + (p[i+1][r+c+c] >> 2) +\
                  (p[i+2][r-c] >> 2) +  p[i+2][r]       +  p[i+2][r+c]       + (p[i+2][r+c+c] >> 2) +\
                  (p[i+3][r-c] >> 1) + (p[i+3][r] >> 2) + (p[i+3][r+c] >> 2) + (p[i+3][r+c+c] >> 1)) >> 3;
      }
    }
  }

  imageMetaData[threadNumber].height >>= 2;
  imageMetaData[threadNumber].width >>= 2;
  free(imageMetaData[threadNumber].pixelData);
  imageMetaData[threadNumber].pixelData = s;
  return 0;
}



int scale_by_double(int threadNumber) {
  unsigned char (*p)[imageMetaData[threadNumber].width * imageMetaData[threadNumber].colorspace] = (unsigned char (*)[imageMetaData[threadNumber].width * imageMetaData[threadNumber].colorspace])imageMetaData[threadNumber].pixelData;
  unsigned char (*s)[imageMetaData[threadNumber].width * imageMetaData[threadNumber].colorspace * 2] = malloc(imageMetaData[threadNumber].width * imageMetaData[threadNumber].height * imageMetaData[threadNumber].colorspace * 4);
  int w = imageMetaData[threadNumber].width, h = imageMetaData[threadNumber].height, c = imageMetaData[threadNumber].colorspace, wc = w * c - c, wx = w * c * 2 - c - c, ii = 0;
  register int r;

  for(int i = 0; i < h - 1; i+=1) {
    for(int j = 0; j < (w - 1) * c; j+=c) {
      ii = i << 1;
      for(int k = 0; k < c; k++) {
        r = j * 2 + k;
        s[ii][r] = p[i][j+k];
        s[ii][r+c] = (p[i][j+k] + p[i][j+k+c]) >> 1;
        s[ii+1][r] = (p[i][j+k] + p[i+1][j+k]) >> 1;
        s[ii+1][r+c] = (p[i][j+k] + p[i+1][j+k+c]) >> 1;
      }
    }
    r = wx;
    for(int k = 0; k < c; k++) { //right edge
      s[ii][r+k] = p[i][wc+k];
      s[ii][r+k+c] = p[i][wc+k] + (p[i][wc+k] - p[i][wc+k-c]); //extrapolate gradient
      s[ii+1][r+k] = (p[i][wc+k] + p[i+1][wc+k]) >> 1;
      s[ii+1][r+k+c] = (s[ii+1][r+k] + s[ii][r+k+c]) >> 1;
    }
  }
  ii = (h << 1) - 2;
  for(int j = 0; j < (w - 1) * c; j+=c) { //bottom edge
    wc = j << 1;
    for(int k = 0; k < c; k++) {
      r = wc + k;
      s[ii][r] = p[h-1][j+k];
      s[ii][r+c] = (p[h-1][j+k] + p[h-1][j+k+c]) >> 1;
      s[ii+1][r] = (p[h-1][j+k] + (p[h-1][j+k] - p[h-2][j+k])); //extrapolate gradient
      s[ii+1][r+c] = (s[ii+1][r] + s[ii][r+c]) >> 1;
    }
  }

  imageMetaData[threadNumber].height <<= 1;
  imageMetaData[threadNumber].width <<= 1;
  imageMetaData[threadNumber].rawImgSize <<= 2;
  free(imageMetaData[threadNumber].pixelData);
  imageMetaData[threadNumber].pixelData = (unsigned char *)s;
  return 0;
}

