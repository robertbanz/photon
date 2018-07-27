
static TimerInterface* interrupt_holder = nullptr;

static void DoInterrupt() {
  if (interrupt_holder != nullptr) {
    interrupt_holder->DoTimerOp();
  }
}

class HW_74HC4059 : class TimerInterface {
 public:
  // Uses 20 I/O pins, starting at io_start
  // First three wired to "Ka,b,c"
  // Next 16 wired to J1-J16
  // Last wired to Latch Enable
  HW_74HC4059(unsigned int io_start, unsigned int interrupt) :
      io_start_(io_start),
      interrupt_(interrupt),
      counter_(0),
      old_counter_(0) {
    for(int i = io_start_; i < io_start + 20; ++i) {
      pinMode(i, OUTPUT);
      digitalWrite(i, LOW);
    }
    pinMode(interrupt_, INPUT);
    attachInterrupt(digitalPinToInterrupt(2), )
  };
  void DisableTimer() {
    SetMode(0x00);
  }
  void EnableTimer() {
    SetMode(current_mode_);
  }
  void EnableLatch() {
    digitalWrite(io_start_ + le_base, HIGH);
  }
  void DisableLatch() {
    digitalWrite(io_start_ + le_base, LOW);
  }
  bool SetDivider(unsigned int count) {
    char d1, d2, d3, d4;
    char mode;
    mode = TryAllModes(count, &d4, &d3, &d2, &d1);
    if (mode == 0) {
      return false;
    }
    current_mode_ = mode;
    SetMode(current_mode_);
    WriteNibbles(io_start_ + jam_base, d1, d4);
    WriteNibbles(io_start_ + jam_base + 8, d3, d2);
  }
  void InLoop() override {
    if (counter_ != old_counter_) {
      // TODO: check overruns
      DoTimerOp();
      old_counter_ = counter_;
    }
  }
 private:
  void DoInterruptOp() {
    ++counter_;
  } 
  const unsigned char modes[6] =      { 7, 6, 5, 4, 3, 0};
  const unsigned char multiplier[6] = { 2, 4, 5, 8, 10, 0};
  const unsigned char remainder[6] =  { 1, 3, 4, 7, 9, 0 };
  const unsigned char maxd4[6] =      { 7, 3, 1, 1, 0, 0 };
  const unsigned char d4shift[6] =    { 1, 2, 3, 3, 0, 0 };
  
  const int mode_base = 0;
  const int jam_base = 3;
  const int le_base = 19;
  
  unsigned int io_start_;
  unsigned int interrupt_;
  unsigned char current_mode_;
  
  volatile unsigned long counter_;
  unsigned long old_counter_;
  void SetMode(unsigned char mode) {
	  current_mode_ = mode;
    digitalWrite(io_start_ + mode_base + 0, current_mode_ & 0x01 ? HIGH : LOW);
    digitalWrite(io_start_ + mode_base + 1, current_mode_ & 0x02 ? HIGH : LOW);
    digitalWrite(io_start_ + mode_base + 2, current_mode_ & 0x04 ? HIGH : LOW);
  }
  
  void WriteNibbles(unsigned int pin_base, char upper, char lower) {
    unsigned char whole = (upper << 4) | (lower & 0x0f);
    Serial.println(whole, HEX);
    for(int i = 0; i < 8; ++i) {
      digitalWrite(pin_base + i, (whole & (1 << i)) == 0 ? LOW : HIGH);
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
    
    *d4 = (target / 1000) > max_d4 ? max_d4 : (target / 1000);    
    target -= (*d4 * 1000);
    
    if (target > 100) {
      *d3 = (target / 100) > 15 ? 15 : (target / 100);
    }
    
    target -= (*d3 * 100);
    
    if (target > 10) {
      *d2 = (target / 10) > 15 ? 15 : (target / 10);
    }
    
    target -= (*d2 * 10);
    
    if (target > 15) {
      Serial.print("Too much remainingin target ");
      Serial.print(target);
      return false;
    }
    
    *d1 = target;
    
    return true;
  }
  
  bool TryMode(int mode, unsigned int count,
      char* d4, char* d3, char* d2, char* d1) {
    int m = multiplier[mode];
    int r = count % m;
    if (r >= remainder[mode]) {
      // remainder wouldn't work.
      Serial.println("wont work because remainder is too big.");
      return false;
    }
    if (!ClosestJams(count / m, maxd4[mode], d4, d3, d2, d1)) {
      return false;
    }
    *d4 = (*d4 << d4shift[mode]) | r;
    return true;
  }
  
  unsigned char TryAllModes(unsigned int count,
      char* d4, char* d3, char* d2, char* d1) {
    for(int i = 0; modes[i] != 0; i++) {
      Serial.print("trying ");
      Serial.println(modes[i]);
      if (TryMode(i, count, d4, d3, d2, d1)) {
        Serial.print("found mode");
        Serial.println(modes[i]);
        return modes[i];
      }
    }
    Serial.println("found no mode");
    return 0;
  }
};