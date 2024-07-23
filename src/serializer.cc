

class Serializer {
    virtual void serialize() const   = 0;
    virtual void deserialize() const = 0;
};

class ImageSerializer: Serializer {
    void serialize() const override {}

    void deserialize() const override {}
};

#include <iostream>

auto main(int argc, char **argv) -> int {
    std::cout << "101010" << std::endl;

    auto imgs = new ImageSerializer();

    return 0;
}

/*

@0xbf5147cbbecf40c1;

struct Image {
  id @0 :Int32;
  filename @1 :Text;
  data @2 :Data;
}

struct TextFile {
  id @0 :Int32;
  filename @1 :Text;
  content @2 :Text;
}

struct DataCollection {
  images @0 :List(Image);
  texts @1 :List(TextFile);
}


*/