#include <textract/textract>

int main(int argc, char **argv) {

  imgstr::printSystemInfo();

  imgstr::ImgProcessor app = imgstr::ImgProcessor();

  /* Process Images from a Directory and write results to a path */

  std::vector<std::string> images = {"screenshot.png", "imgtext.jpeg",
                                     "compleximgtext.png", "scatteredtext.png"};

  app.processImagesDir("", true, "output");

  app.getResults();

  /* Iterately add Images */

  std::vector<std::string> additional_images = {
      "path/to/image1.jpeg", "path/to/image2.tif", "path/to/image3.png",
      "path/to/image4.jpg",  "path/to/image5.bmp",
  };

  app.addFiles(additional_images);

  app.processImages();

  return 0;
}
