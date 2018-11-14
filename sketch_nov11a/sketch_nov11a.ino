#include <LiquidCrystal_I2C.h>

enum DirectionPin {
  NO_PIN = -1,
  LEFT = 2,
  DOWN = 3,
  UP = 4,
  RIGHT = 5
};

byte mySnake[8][8] =
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

boolean levelz[][2][16] = {
{{false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false},
{false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false}},
 
{{true,false,false,false,false,false,false,false,false,false,false,false,false,false,false,true},
{true,false,false,false,false,false,false,false,false,false,false,false,false,false,false,true}},
 
{{true,false,false,false,true,false,false,false,false,false,false,true,false,false,false,true},
{true,false,false,false,false,false,false,false,true,false,false,false,false,false,false,true}},
 
{{true,false,true,false,false,false,false,false,false,true,false,false,false,true,false,false},
{false,false,false,false,true,false,false,true,false,false,false,true,false,false,false,true}}
};

constexpr size_t levels = sizeof(levelz) / sizeof(levelz); //number of levels

LiquidCrystal_I2C lcd(0x3F, 16, 2);
unsigned long time, timeNow;
int gameSpeed;
boolean skip, gameOver, gameStarted;
int olddir;
boolean (*selectedLevel)[2][16] = levelz;
 
int key=-1;
int oldkey=-1;
 
boolean x[16][80];
byte myChar[8];
byte nullChar[8] = { 0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0 };
boolean special;
 
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
  x[pr][pc] = true;
  for(int r=0;r<2;r++)
  {
    for(int c=0;c<16;c++)
    {
      special = false;
      for(int i=0;i<8;i++)
      {
        byte b=B00000;
        if (x[r*8+i][c*5+0]) {b+=B10000; special = true;}
        if (x[r*8+i][c*5+1]) {b+=B01000; special = true;}
        if (x[r*8+i][c*5+2]) {b+=B00100; special = true;}
        if (x[r*8+i][c*5+3]) {b+=B00010; special = true;}
        if (x[r*8+i][c*5+4]) {b+=B00001; special = true;}
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
        if (selectedLevel[r][c]) lcd.write(255);
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
    if (selectedLevel[pr / 8][pc / 5]) newp=true;
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
 
  if (selectedLevel[head->row / 8][head->column / 5]) gameOver = true; // wall collision check
 
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
  x[head->row][head->column] = true;
 
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
  x[p->row][p->column] = false;
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
  for (i=0;i<16;i++)
    for (j=0;j<80;j++)
      x[i][j] = false;
         
  part *p, *q;
  tail = (part*)malloc(sizeof(part));
  tail->row = 7;
  tail->column = 39 + n/2;
  tail->dir = LEFT;
  q = tail;
  x[tail->row][tail->column] = true;
  for (i = 0; i < n-1; i++) // build snake from tail to head
  {
    p = (part*)malloc(sizeof(part));
    p->row = q->row;
    p->column = q->column - 1; //initial snake id placed horizoltally
    x[p->row][p->column] = true;
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
  selectedLevel = 1;
 
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Select level: 1");
  for(i=0;i<8;i++)
  {
    lcd.createChar(i,mySnake[i]);
    lcd.setCursor(i+4,1);
    lcd.write(byte(i));
  }
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
         if (key==UP && selectedLevel<levels) selectedLevel++;
         if (key==DOWN && selectedLevel>1) selectedLevel--;
         if (key==RIGHT)
         {
           lcd.clear();
           selectedLevel--;
           newPoint();
           gameStarted = true;
         }
         else
         {
           lcd.setCursor(14,0);
           lcd.print(levelz - selectedLevel + 1);
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

