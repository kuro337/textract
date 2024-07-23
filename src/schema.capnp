

# Void: Void
# Boolean: Bool
# Integers: Int8, Int16, Int32, Int64
# Unsigned integers: UInt8, UInt16, UInt32, UInt64
# Floating-point: Float32, Float64
# Blobs: Text, Data
# Lists: List(T)

struct Timestamp {
  unixtime @0 :Int64;
}

struct UUID {
  type @0 :Type = Seven;
  low  @1 :UInt64;
  high @2 :UInt64;

  enum Type {
      Seven @0;
      Four @1;
    }
}

interface Image {
 exists @0 () -> (result : Bool);
 toText @1 () -> (text : Data);
 toHash @2 () -> (result : Data);
}

struct KImage {
  id @0  :UUID;
  uri @1 :Text;
  size @3 :UInt64;
  hash @4 :Data;
  text @5 :Text;
  fuzzhash @6 :Text;
}

# Image 
# - id
# - uri
# - hash
# - text
# - format
# - names
