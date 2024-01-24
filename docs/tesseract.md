# tesseract

Deps

```bash
brew install tesseract
brew install libtesseract

https://tesseract-ocr.github.io/tessdoc/APIExample.html

```

CLI Usage

```bash
To use the LSTM engine:
tesseract input.tiff output --oem 1 -l eng

To use the Legacy engine:
tesseract input.tiff output --oem 0 -l eng

tesseract --tessdata-dir /opt/homebrew/opt/tesseract/share/tessdata screenshot.png outputbase -l eng --psm 3

# for searchable PDF

tesseract images/toc.png images/toc -l eng --oem 1 pdf

# awkward image
z0d2041bcb10cb63a.jpg

# psm 3 does well for this
tesseract --tessdata-dir /opt/homebrew/opt/tesseract/share/tessdata z0d2041bcb10cb63a.jpg test -l eng --psm 3

tesseract --tessdata-dir /opt/homebrew/opt/tesseract/share/tessdata z0d2041bcb10cb63a.jpg test -l eng --psm 4


```

PSM Modes

```bash

0 - Orientation and script detection (OSD) only.

Use when you only need to determine the orientation or script (language) of the text, not to perform actual OCR.
1 - Automatic page segmentation with OSD.

Suitable for images with a clear layout and where the orientation or script is unknown.
2 - Automatic page segmentation, but no OSD, or OCR.

Useful for creating a box file for training Tesseract.
3 - Fully automatic page segmentation, but no OSD. (Default)

The best general option for images with a single column of text and unknown layout.
4 - Assume a single column of text of variable sizes.

Good for images with a single column of text that varies in size.
5 - Assume a single uniform block of vertically aligned text.

Use for text that is aligned vertically.
6 - Assume a single uniform block of text.

Suitable for images with a single block of text.
7 - Treat the image as a single text line.

Optimal for images with a single line of text, such as a street sign or a sentence.
8 - Treat the image as a single word.

Ideal for images containing a single word.
9 - Treat the image as a single word in a circle.

Useful for OCR on circular text.
10 - Treat the image as a single character.

Best for images with a single character.
11 - Sparse text. Find as much text as possible in no particular order.

Good for images with sparse text scattered around.
12 - Sparse text with OSD.

Similar to 11, but also attempts to find the orientation.
13 - Raw line. Treat the image as a single text line, bypassing hacks that are Tesseract-specific.

Use for raw processing of text lines.

```
