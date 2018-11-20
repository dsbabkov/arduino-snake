#include <LiquidCrystal_I2C.h>

enum DirectionPin {
  NO_PIN = -1,
  LEFT = 2,
  DOWN = 3,
  UP = 4,
  RIGHT = 5
};

constexpr uint8_t maxSnakeSize = 16;

constexpr uint8_t lcdRowCount = 2;
constexpr uint8_t lcdColumnCount = 16;
constexpr uint8_t lcdCharRowCount = 8;
constexpr uint8_t lcdCharColumnCount = 5;
constexpr uint8_t lcdRowPixelCount = lcdRowCount * lcdCharRowCount;
constexpr uint8_t lcdColumnPixelCount = lcdColumnCount * lcdCharColumnCount;
constexpr uint16_t lcdPixelCount = lcdRowPixelCount * lcdColumnPixelCount;

LiquidCrystal_I2C lcd(0x3F, lcdColumnCount, lcdRowCount);

void drawPixmapSnake() {
  using LcdChar = byte[lcdCharRowCount];
  constexpr LcdChar snake[] =
  {
  { B00000,
    B00000,
    B00011,
    B00110,
    B01100,
    B11000,
    B00000,
  },
  { B00000,
    B11000,
    B11110,
    B00011,
    B00001,
    B00000,
    B00000,
  },
  { B00000,
    B00000,
    B00000,
    B00000,
    B00000,
    B11111,
    B01110,
  },
  { B00000,
    B00000,
    B00011,
    B01111,
    B11000,
    B00000,
    B00000,
  },
  { B00000,
    B11100,
    B11111,
    B00001,
    B00000,
    B00000,
    B00000,
  },
  { B00000,
    B00000,
    B00000,
    B11000,
    B01101,
    B00111,
    B00000,
  },
  { B00000,
    B00000,
    B01110,
    B11011,
    B11111,
    B01110,
    B00000,
  },
  { B00000,
    B00000,
    B00000,
    B01000,
    B10000,
    B01000,
    B00000,
  }
  };
  
  for(byte i = 0; i < 8; ++i) {
    lcd.createChar(i, snake[i]);
    lcd.write(i);
  }
}

class Level {
public:
  using ColumnsStorage = uint16_t;
  using Storage = ColumnsStorage [lcdRowCount];

private:
  static constexpr auto columnsStorageSizeInBits = sizeof(ColumnsStorage) * 8;
  static_assert(columnsStorageSizeInBits == lcdColumnCount, "Invalid type to store level");  

public:
  explicit Level (const Storage &storage = {}) {
    for (size_t i = 0; i < lcdRowCount; ++i) {
      storage_[i] = storage[i];
    }
  }
  explicit Level (ColumnsStorage row1, ColumnsStorage row2)
    : Level({row1, row2})
    {}

  bool operator()(size_t i, unsigned j) const {
    const ColumnsStorage &columnsStorage = storage_[i];
    return columnsStorage & (static_cast<ColumnsStorage>(1) << (columnsStorageSizeInBits - 1 - j));
  }

private:
  Storage storage_;
};

Level Levelz[] = {
  Level(), // empty
  
  Level(0b1000000000000001,
        0b1000000000000001),
  
  Level(0b1000100000010001,
        0b1000000010000001),

  Level(0b1010000001000100,
        0b0000100100010001)
};


class Pixel {
  friend class Canvas;
public:
  operator bool () const {
    return canvasStorage_ & (1 << bitNumber_);
  }

  Pixel &operator= (bool val) {
    const uint16_t mask = (1u << bitNumber_);
    if (val) {
      canvasStorage_ |= mask;
    }
    else {
      canvasStorage_ &= ~mask;
    }
    return *this;
  }
  
private:
  Pixel(uint16_t &canvasStorage, uint8_t bitNumber)
    : canvasStorage_(canvasStorage), bitNumber_(bitNumber)
    {}

private:
  uint16_t &canvasStorage_;
  uint8_t bitNumber_;
};

class Canvas {
public:
  Canvas() = default;
  Canvas &operator= (const Canvas &) = default;
  
  Pixel operator()(uint8_t row, size_t col) {
    return Pixel(pixels_[col], row);
  }

  void clear() {
    *this = Canvas();
  }
  
private:
  uint16_t pixels_[80] = {};
};

constexpr size_t levels = sizeof(Levelz) / sizeof(*Levelz); //number of levels

unsigned long time, timeNow;
int gameSpeed;
boolean skip, gameOver, gameStarted;
int olddir;
const Level *selectedLevel = Levelz;
 
int key=-1;
int oldkey=-1;
 
Canvas x;
byte myChar[8];
 
struct partdef
{
  int row,column,dir;
  struct partdef *next;
};
typedef partdef part;
 
part *head, *tail;
int i,j,collected;
long pc,pr;
 
void drawMatrix()
{
  int cc=0;
  if (!gameOver)
  {
  x(pr, pc) = true;
  for(int r = 0; r < lcdRowCount; ++r)
  {
    for(int c = 0; c < lcdColumnCount; ++c)
    {
      boolean special = false;
      for(int i=0; i < lcdCharRowCount; ++i)
      {
        byte b = 0;
        for (int j = 0; j < lcdCharColumnCount; ++j) {
          if (x(r * lcdCharRowCount + i, c * lcdCharColumnCount + j)) {
            constexpr byte leftPointPattern = 1 << (lcdCharColumnCount - 1);
            b |= leftPointPattern >> j;
          }
        }
        special = special || b;
        myChar[i] = b;
      }
      if (special)
      {
        lcd.createChar(cc, myChar);
        lcd.setCursor(c,r);
        lcd.write(byte(cc));
        cc++;
      }
      else
      {
        lcd.setCursor(c,r);
        if ((*selectedLevel)(r, c)) lcd.write(255);
        else lcd.write(254);
      }
    }
  }
  }
}
 
void freeList()
{
  part *p,*q;
  p = tail;
  while (p!=NULL)
  {
    q = p;
    p = p->next;
    free(q);
  }
  head = tail = NULL;
}
 
void gameOverFunction()
{
  delay(1000);
  lcd.clear();
  freeList();
  lcd.setCursor(3,0);
  lcd.print("Game Over!");
  lcd.setCursor(4,1);
  lcd.print("Score: ");
  lcd.print(collected);
  delay(1000);
}
 
void growSnake()
{
  part *p;
  p = (part*)malloc(sizeof(part));
  p->row = tail->row;
  p->column = tail->column;
  p->dir = tail->dir;
  p->next = tail;
  tail = p;
}
 
void newPoint()
{
 
  part *p;
  p = tail;
  boolean newp = true;
  while (newp)
  {
    pr = random(16);
    pc = random(80);
    newp = false;
    if ((*selectedLevel)(pr / 8, pc / 5)) newp=true;
    while (p->next != NULL && !newp)
    {
      if (p->row == pr && p->column == pc) newp = true;
      p = p->next;
    }
  }
 
  if (collected < 13 && gameStarted) growSnake();
}
 
void moveHead()
{
  switch(head->dir) // 1 step in direction
  {
    case UP: head->row--; break;
    case DOWN: head->row++; break;
    case RIGHT: head->column++; break;
    case LEFT: head->column--; break;
    default : break;
  }
  if (head->column >= 80) head->column = 0;
  if (head->column < 0) head->column = 79;
  if (head->row >= 16) head->row = 0;
  if (head->row < 0) head->row = 15;
 
  if ((*selectedLevel)(head->row / 8, head->column / 5)) gameOver = true; // wall collision check
 
  part *p;
  p = tail;
  while (p != head && !gameOver) // self collision
  {
    if (p->row == head->row && p->column == head->column) gameOver = true;
    p = p->next;
  }
  if (gameOver)
    gameOverFunction();
  else
  {
  x(head->row, head->column) = true;
 
  if (head->row == pr && head->column == pc) // point pickup check
  {
    collected++;
    if (gameSpeed < 25) gameSpeed+=3;
    newPoint();
  }
  }
}
 
void moveAll()
{
  part *p;
  p = tail;
  x(p->row, p->column) = false;
  while (p->next != NULL)
  {
    p->row = p->next->row;
    p->column = p->next->column;
    p->dir = p->next->dir;
    p = p->next;
  }
  moveHead();
}
 
void createSnake(int n) // n = size of snake
{
  x.clear();
         
  part *p, *q;
  tail = (part*)malloc(sizeof(part));
  tail->row = 7;
  tail->column = 39 + n/2;
  tail->dir = LEFT;
  q = tail;
  x(tail->row, tail->column) = true;
  for (i = 0; i < n-1; i++) // build snake from tail to head
  {
    p = (part*)malloc(sizeof(part));
    p->row = q->row;
    p->column = q->column - 1; //initial snake id placed horizoltally
    x(p->row, p->column) = true;
    p->dir = q->dir;
    q->next = p;
    q = p;
  }
  if (n>1)
  {
    p->next = NULL;
    head  = p;
  }
  else
  {
    tail->next = NULL;
    head = tail;
  }
}
 
void startF()
{
  gameOver = false;
  gameStarted = false;
  selectedLevel = Levelz;
 
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Select level: 1");
  lcd.setCursor(4, 1);
  drawPixmapSnake();
  collected = 0;
  gameSpeed = 8;
  createSnake(3);
  time = 0;
}
void setup()
{
  lcd.init();                     
  lcd.backlight();// Включаем подсветку дисплея
  lcd.clear();
  startF();
  pinMode(LEFT, INPUT_PULLUP);
  pinMode(DOWN, INPUT_PULLUP);
  pinMode(UP, INPUT_PULLUP);
  pinMode(RIGHT, INPUT_PULLUP);
}
 
void loop()
{
  if (!gameOver && !gameStarted)
  {
   key = get_key();  // convert into key press
   if (key != oldkey)   // if keypress is detected
   {
     delay(50);  // wait for debounce time
     key = get_key();    // convert into key press
     if (key != oldkey)    
     {  
       oldkey = key;
       if (key >=0)
       {
         olddir = head->dir;
         if (key==UP && selectedLevel<(Levelz + levels - 1)) selectedLevel++;
         if (key==DOWN && selectedLevel>Levelz) selectedLevel--;
         if (key==RIGHT)
         {
           lcd.clear();
           newPoint();
           gameStarted = true;
         }
         else
         {
           lcd.setCursor(14,0);
           lcd.print(selectedLevel - Levelz + 1);
         }
       }
     }
   }
  }
  if (!gameOver && gameStarted)
  {
   skip = false; //skip the second moveAll() function call if the first was made
   
   key = get_key();  // convert into key press
   if (key != oldkey)   // if keypress is detected
   {
     delay(50);  // wait for debounce time
     key = get_key();    // convert into key press
     if (key != oldkey)    
     {  
       oldkey = key;
       if (key >=0)
       {
         olddir = head->dir;
         if (key==RIGHT && head->dir!=LEFT) head->dir = RIGHT;
         if (key==UP && head->dir!=DOWN) head->dir = UP;
         if (key==DOWN && head->dir!=UP) head->dir = DOWN;
         if (key==LEFT && head->dir!=RIGHT) head->dir = LEFT;
         
         if (olddir != head->dir)
         {
           skip = true;
           delay(1000/gameSpeed);
           moveAll();
           drawMatrix();
         }
       }
     }
   }
   if (!skip)
   {
     timeNow = millis();
     if (timeNow - time > 1000 / gameSpeed)
     {
       moveAll();
       drawMatrix();
       time = millis();
     }
   }
  }
  if(gameOver)
  {
   
   key = get_key();  // convert into key press
   if (key != oldkey)   // if keypress is detected
   {
     delay(50);  // wait for debounce time
     key = get_key();    // convert into key press
     if (key != oldkey)    
     {  
       oldkey = key;
       if (key >=0)
       {
          startF();
       }
     }
   }
   
  }
}

DirectionPin get_key() {
  if (!digitalRead(LEFT)) return LEFT;
  if (!digitalRead(RIGHT)) return RIGHT;
  if (!digitalRead(UP)) return UP;
  if (!digitalRead(DOWN)) return DOWN;
  return NO_PIN;
}

