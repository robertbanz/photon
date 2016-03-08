

class 74HC4059 {
 public:
  // Uses 20 I/O pins, starting at io_start
  // First three wired to "Ka,b,c"
  // Next 16 wired to J1-J16
  // Last wired to Latch Enable
  74HC4059(unsigned int io_start) :
      io_start_(io_start) {
    for(int i = 0; i < 20; ++i) {
      pinMode(i, OUTPUT);
    }
  };
  void DisableTimer() {
    SetMode(0x00);
  }
  void EnableTimer() {
    SetMode(current_mode_);
  }
  void EnableLatch() {
    digitalWrite(io_start_ + 19, HIGH);
  }
  void DisableLatch() {
    digitalWrite(io_start_ + 19, LOW);
  }
  bool SetDivider(unsigned int count) {
    char d1, d2, d3, d4;
    char mode;
    mode = TryAllMode(count, &d4, &d3, &d2, &d1);
    if (mode == 0) {
      return false;
    }
    current_mode_ = mode;
    SetMode(current_mode_);
    WriteNibbles(io_start_ + 3, d2, d1);
    WriteNibbles(io_start_ + 11, d4, d3);
  }
 private:
  static unsigned char modes[] =      { 7, 6, 5, 4, 3, 0};
  static unsigned char multiplier[] = { 2, 4, 5, 8, 10, 0};
  static unsigned char remainder[] =  { 1, 3, 4, 7, 9, 0 };
  static unsigned char maxd4[] =      { 7, 3, 1, 1, 0, 0 };
  static unsigned char d4shift[] =    { 1, 2, 3, 3, 0, 0 };
  
  static int mode_base = 0;
  static int jam_base = 3;
  static int le_base = 19;
  
  int io_start_;
  
  unsigned char current_mode_;
  
  void SetMode() {
    digitalWrite(io_start_ + mode_base + 0, current_mode_ & 0x01 ? HIGH : LOW);
    digitalWrite(io_start_ + mode_base + 1, current_mode_ & 0x02 ? HIGH : LOW);
    digitalWrite(io_start_ + mode_base + 2, current_mode_ & 0x03 ? HIGH : LOW);
  }
  
  void WriteNibbles(unsigned int pin_base, char upper, char lower) {
    unsigned char whole = (upper << 4) | lower & 0x0f;
    for(int i = 0; i < 8; ++i) {
      digitalWrite(pin_base + 8, (whole & (1 << i)) == 0 ? LOW : HIGH);
    }
  }
  
  bool ClosestJams(
      unsigned int count, int max_d4, char* d4, char* d3, char* d2, char* d1) {
    auto target = count;
    unsigned int max = ( max_d4 * 1000 + 15 * 100 + 15 * 10 + 15);
    if (count > max) {
      return false;
    }
    
    *d4 = *d3 = *d2 = 0;
    
    *d4 = max / 1000;
    if (*d4 > max_d4) {
      *d4 = max_d4;
    }
    
    target -= (*d4 * 1000);
    
    if (target > 100) {
      *d3 = (target / 100) > 15 : 15 ? (target / 100);
    }
    
    target -= (*d3 * 100);
    
    if (target > 10) {
      *d2 = (target / 10) > 15 : 15 ? (target / 10);
    }
    
    target -= (*d2 * 10);
    
    if (target > 15) {
      return false;
    }
    
    *d1 = target;
    
    return false;
  }
  
  bool TryMode(int mode, unsigned int count,
      char* d4, char* d3, char* d2, char* d1) {
    int m = multiplier[mode];
    int r = count % m;
    if (r >= reaminder[mode]) {
      // remainder wouldn't work.
      return false;
    }
    *d1 = r;
    if (!ClosestJams(count / m, max_d4[mode], d4, d3, d2, d1)) {
      return false;
    }
    *d4 = (*d4 << d4shift[mode]) | r;
    return true;
  }
  
  unsigned char TryAllModes(unsigned int count,
      char* d4, char* d3, char* d2, char* d1) {
    for(int i = 0; modes[i] == 0; i++) {
      if (TryMode(mode, count, d4, d3, d2, d1)) {
        return modes[i];
      }
    }
    return 0;
  }
};