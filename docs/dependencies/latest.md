# Deps

```bash
apt list --installed | grep tesseract

# Available Versions
apt-cache policy tesseract-ocr



```

# Tesseract Latest v5.x from Source

```bash
git@github.com:tesseract-ocr/tesseract.git


./autogen.sh
./configure --prefix=/usr/local/
make
make install
sudo ldconfig  # To update the library cache

/usr/local/bin/tesseract --version

# Available Options
./configure --help

```
