int jpeg_decode(unsigned char* srcImg, unsigned int srcImgSize, int threadNumber) {
  struct jpeg_decompress_struct info;
  struct jpeg_error_mgr err;
  unsigned char* pixRowBuf[1];

  info.err = jpeg_std_error(&err);
  jpeg_create_decompress(&info);
  jpeg_mem_src(&info, srcImg, srcImgSize);
  jpeg_read_header(&info, TRUE);
  jpeg_start_decompress(&info);

  imageMetaData[threadNumber].colorspace = info.output_components;
  imageMetaData[threadNumber].width = info.output_width;
  imageMetaData[threadNumber].height = info.output_height;
  imageMetaData[threadNumber].rawImgSize = info.output_width * info.output_height * info.output_components;
  int subPixelsPerRow = info.output_width * info.output_components;
  imageMetaData[threadNumber].pixelData = malloc(imageMetaData[threadNumber].rawImgSize);

  /* Read scanlines */
  while(info.output_scanline < info.output_height) {
    *pixRowBuf = &imageMetaData[threadNumber].pixelData[subPixelsPerRow * info.output_scanline];
    jpeg_read_scanlines(&info, pixRowBuf, 1);
  }
  jpeg_finish_decompress(&info);
  jpeg_destroy_decompress(&info);
  return 0;
}



int jpeg_encode(int threadNumber) {
  struct jpeg_compress_struct info;
  struct jpeg_error_mgr err;
  unsigned char* pixRowBuf[1];
  imageMetaData[threadNumber].rawImgSize = imageMetaData[threadNumber].width * imageMetaData[threadNumber].height * imageMetaData[threadNumber].colorspace;
  imageMetaData[threadNumber].encodedImg = malloc(imageMetaData[threadNumber].rawImgSize);

  info.err = jpeg_std_error(&err);
  jpeg_create_compress(&info);
  imageMetaData[threadNumber].destImgSize = imageMetaData[threadNumber].rawImgSize;
  jpeg_mem_dest(&info, &imageMetaData[threadNumber].encodedImg, (size_t *)&imageMetaData[threadNumber].destImgSize);
  info.image_width = imageMetaData[threadNumber].width;
  info.image_height = imageMetaData[threadNumber].height;
  info.input_components = imageMetaData[threadNumber].colorspace;
  info.in_color_space = JCS_RGB;
  jpeg_set_defaults(&info);
  jpeg_set_quality(&info, 87, TRUE);
  jpeg_start_compress(&info, TRUE);
  int subPixelsPerRow = imageMetaData[threadNumber].width * imageMetaData[threadNumber].colorspace;

  /* Write scanlines */
  while(info.next_scanline < info.image_height) {
    *pixRowBuf = &imageMetaData[threadNumber].pixelData[subPixelsPerRow * info.next_scanline];
    jpeg_write_scanlines(&info, pixRowBuf, 1);
  }

  jpeg_finish_compress(&info);
  jpeg_destroy_compress(&info);
  return 0;
}



int png_decode(unsigned char* srcImg, unsigned int srcImgSize, int threadNumber) {
  png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
  if(!png_ptr) return 1;
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if(!info_ptr) {
    png_destroy_read_struct(&png_ptr, (png_infopp)NULL, (png_infopp)NULL);
    return 1;
  }
  png_infop end_info = png_create_info_struct(png_ptr);
  if(!end_info) {
    png_destroy_read_struct(&png_ptr, &info_ptr, (png_infopp)NULL);
    return 1;
  }
  if(setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
    return 1;
  }

  FILE *srcPNG = fmemopen(srcImg, srcImgSize, "rb");
  png_init_io(png_ptr, srcPNG);
  png_read_info(png_ptr, info_ptr);

  int color_type;
  png_get_IHDR(png_ptr, info_ptr, (unsigned int*)&imageMetaData[threadNumber].width, (unsigned int*)&imageMetaData[threadNumber].height, &imageMetaData[threadNumber].colorspace, &color_type, 0, 0, 0);
  if(imageMetaData[threadNumber].colorspace != 8) return 1;
  imageMetaData[threadNumber].colorspace = (color_type == PNG_COLOR_TYPE_RGB_ALPHA) ? 4 : 3;

  png_bytep *row_pointers = malloc(sizeof(png_bytep) * imageMetaData[threadNumber].height);
  imageMetaData[threadNumber].rawImgSize = imageMetaData[threadNumber].width * imageMetaData[threadNumber].height * imageMetaData[threadNumber].colorspace;
  imageMetaData[threadNumber].pixelData = malloc(imageMetaData[threadNumber].rawImgSize);

  for(int i = 0; i < imageMetaData[threadNumber].height; i++)
    row_pointers[i] = &imageMetaData[threadNumber].pixelData[i * imageMetaData[threadNumber].width * imageMetaData[threadNumber].colorspace];

  png_read_image(png_ptr, row_pointers);
  png_read_end(png_ptr, end_info);
  png_destroy_read_struct(&png_ptr, &info_ptr, &end_info);
  free(row_pointers);
  fclose(srcPNG);
  return 0;
}



int png_encode(int threadNumber) {
  imageMetaData[threadNumber].rawImgSize = imageMetaData[threadNumber].width * imageMetaData[threadNumber].height * imageMetaData[threadNumber].colorspace;
  png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
  if(!png_ptr)
    return 1;
  png_infop info_ptr = png_create_info_struct(png_ptr);
  if(!info_ptr) {
    png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
    return 1;
  }
  if(setjmp(png_jmpbuf(png_ptr))) {
    png_destroy_write_struct(&png_ptr, &info_ptr);
    return 2;
  }

  imageMetaData[threadNumber].encodedImg = malloc(imageMetaData[threadNumber].rawImgSize + 128);
  FILE *destImg = fmemopen(imageMetaData[threadNumber].encodedImg, imageMetaData[threadNumber].rawImgSize + 128, "wb");
  png_init_io(png_ptr, destImg);

  int color_type = (imageMetaData[threadNumber].colorspace == 3) ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_RGB_ALPHA;
  png_set_IHDR(png_ptr, info_ptr, imageMetaData[threadNumber].width, imageMetaData[threadNumber].height, 8, color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
  png_write_info(png_ptr, info_ptr);

  png_bytep *row_pointers = malloc(sizeof(png_bytep) * imageMetaData[threadNumber].height);
  for(int i = 0; i < imageMetaData[threadNumber].height; i++)
    row_pointers[i] = &imageMetaData[threadNumber].pixelData[i * imageMetaData[threadNumber].width * imageMetaData[threadNumber].colorspace];

  png_write_image(png_ptr, row_pointers);
  png_write_end(png_ptr, 0);
  png_destroy_write_struct(&png_ptr, &info_ptr);
  free(row_pointers);
  imageMetaData[threadNumber].destImgSize = ftell(destImg);
  fclose(destImg);
  return 0;
}



int webp_decode(unsigned char* srcImg, unsigned int srcImgSize, int threadNumber) {
  if(!WebPGetInfo(srcImg, srcImgSize, &imageMetaData[threadNumber].width, &imageMetaData[threadNumber].height)) return 1; //lib version mismatch

  WebPBitstreamFeatures features;
  if(WebPGetFeatures(srcImg, srcImgSize, &features)) return 2;

  WebPDecoderConfig config;
  WebPInitDecoderConfig(&config);
  imageMetaData[threadNumber].colorspace = (features.has_alpha | features.has_animation) ? 4 : 3;
  config.output.colorspace = (features.has_alpha | features.has_animation) ? MODE_RGBA : MODE_RGB;
  config.options.no_fancy_upsampling = 1;
  config.options.bypass_filtering = 1;
  config.options.use_scaling = 0;
  config.options.dithering_strength = 0;
  config.options.alpha_dithering_strength = 0;

  //if animated img, grab first frame
  if(features.has_animation) {
    WebPData webp_data;
    webp_data.bytes = srcImg;
    webp_data.size = srcImgSize;
    WebPDemuxer* demux = WebPDemux(&webp_data);
    WebPIterator framePtr;
    if(!WebPDemuxGetFrame(demux, 1, &framePtr)) return 3;
    //config.input.height = framePtr.height;
    //config.input.width = framePtr.width;
    //config.input.has_alpha = framePtr.has_alpha;
    //config.input.has_animation = 1;

    if(WebPDecode(framePtr.fragment.bytes, framePtr.fragment.size, &config)) return 4;
    WebPDemuxReleaseIterator(&framePtr);
    WebPDemuxDelete(demux);
  }
  else //not animated
    if(WebPDecode(srcImg, srcImgSize, &config)) return 5;
  imageMetaData[threadNumber].pixelData = config.output.u.RGBA.rgba;
  imageMetaData[threadNumber].rawImgSize = config.output.u.RGBA.size;

  if(config.output.u.RGBA.stride != imageMetaData[threadNumber].width * imageMetaData[threadNumber].colorspace) {
    printf("stride error: end of row padding inserted\nW*C %i\nStride %i\n",imageMetaData[threadNumber].width * imageMetaData[threadNumber].colorspace, config.output.u.RGBA.stride);
    return 6;
  }
  return 0;
}



int bmp_decode(void* srcImg, unsigned int srcImgSize, int threadNumber) {
  int* dib = srcImg + 2;
  unsigned short *colorspace = srcImg+28;
  if(dib[7]) return 1; //img compression not supported
  switch(*colorspace) {
    case 24: imageMetaData[threadNumber].colorspace = 3; break;
    case 32: imageMetaData[threadNumber].colorspace = 4; break;
    default: return 2; //1/4/8/16 bit not supported
  }
  imageMetaData[threadNumber].width = dib[4];
  imageMetaData[threadNumber].height = abs(dib[5]);
  imageMetaData[threadNumber].rawImgSize = imageMetaData[threadNumber].width * imageMetaData[threadNumber].height * imageMetaData[threadNumber].colorspace;
  imageMetaData[threadNumber].pixelData = malloc(imageMetaData[threadNumber].rawImgSize);

  //reverse row order and remove line padding
  int rowWidth = imageMetaData[threadNumber].width * imageMetaData[threadNumber].colorspace;
  int rowPadded = rowWidth;
  rowPadded += (rowPadded % 4) ? 4 - (rowPadded % 4) : 0;
  unsigned char *s = imageMetaData[threadNumber].pixelData, *d = srcImg + dib[2];

  register int r = imageMetaData[threadNumber].height - 1;
  if(dib[5] < 0) //rows already top-to-bottom (reversed)
    for(int i = 0; i < imageMetaData[threadNumber].height; i++)
      memcpy(&s[i * rowWidth], &d[rowPadded * i], rowWidth);
  else //rows are bottom-up
    for(int i = 0; i < imageMetaData[threadNumber].height; i++)
      memcpy(&s[i * rowWidth], &d[rowPadded * r--], rowWidth);

  //BGR -> RGB
  unsigned char b;
  for(int i = 0; i < imageMetaData[threadNumber].rawImgSize; i+=imageMetaData[threadNumber].colorspace) {
    b = s[i];
    s[i] = s[i+2];
    s[i+2] = b;
  }
  return 0;
}

