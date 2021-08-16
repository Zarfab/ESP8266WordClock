#ifndef DISPLAY_UTILS_H
#define DISPLAY_UTILS_H

#define VERSION 1 // 0 = old letters layout, above = new

#define LETTER_PER_LINE 13
#define NB_LINES 8

static const uint8_t digits[10][8][3] = {
  { // 0
    {0, 1, 0},
    {1, 0, 1},
    {1, 0, 1},
    {1, 0, 1},
    {1, 0, 1},
    {1, 0, 1},
    {1, 0, 1},
    {0, 1, 0}
  },
  { // 1
    {0, 0, 1},
    {0, 1, 1},
    {1, 0, 1},
    {0, 0, 1},
    {0, 0, 1},
    {0, 0, 1},
    {0, 0, 1},
    {0, 0, 1}
  },
  { // 2
    {0, 1, 0},
    {1, 0, 1},
    {1, 0, 1},
    {0, 0, 1},
    {0, 1, 0},
    {0, 1, 0},
    {1, 0, 0},
    {1, 1, 1}
  },
  { // 3
    {0, 1, 0},
    {1, 0, 1},
    {0, 0, 1},
    {0, 1, 0},
    {0, 0, 1},
    {0, 0, 1},
    {1, 0, 1},
    {0, 1, 0}
  }, 
  { // 4
    {0, 0, 1},
    {0, 1, 1},
    {1, 0, 1},
    {1, 0, 1},
    {1, 1, 1},
    {0, 0, 1},
    {0, 0, 1},
    {0, 0, 1}
  },
  { // 5
    {1, 1, 1},
    {1, 0, 0},
    {1, 0, 0},
    {1, 1, 0},
    {0, 0, 1},
    {0, 0, 1},
    {0, 0, 1},
    {1, 1, 0}
  }, 
  { // 6
    {0, 1, 1},
    {1, 0, 0},
    {1, 0, 0},
    {1, 1, 0},
    {1, 0, 1},
    {1, 0, 1},
    {1, 0, 1},
    {0, 1, 0}
  },
  { // 7
    {1, 1, 1},
    {0, 0, 1},
    {0, 0, 1},
    {0, 1, 0},
    {0, 1, 0},
    {1, 0, 0},
    {1, 0, 0},
    {1, 0, 0}
  }, 
  { // 8
    {0, 1, 0},
    {1, 0, 1},
    {1, 0, 1},
    {0, 1, 0},
    {1, 0, 1},
    {1, 0, 1},
    {1, 0, 1},
    {0, 1, 0}
  }, 
  { // 9
    {0, 1, 0},
    {1, 0, 1},
    {1, 0, 1},
    {0, 1, 1},
    {0, 0, 1},
    {0, 0, 1},
    {0, 0, 1},
    {1, 1, 0}
  }
};

#if (VERSION == 0)
const String letters[] = {
  "ilyestrunesix",
  "quatrelminuit",
  "cinqwdeuxsept",
  "dixshuitbneuf",
  "troismidionze",
  "pheureskmoins",
  "dixvingtacinq",
  "etjquartdemie"
};
#else
const String letters[] = {
  "ilyestrunesix",
  "quatrelminuit",
  "cinqwdeuxsept",
  "dixshuitbneuf",
  "troismidionze",
  "pheureskmoins",
  "dixvingtdemie",
  "letjquartcinq"
};
#endif


const int indexes[26][16] = {  
  {3, 15, 86, 96}, // 'a' appears 3 times, at indexes 15, 85 and 95
  {1, 47},         // 'b' appears 1 time, at indexes 47
  {2, 26, 87},     // ...
  {5, 31, 39, 59, 78, 99},
  {12, 3, 9, 18, 32, 36, 49, 64, 67, 70, 91, 100, 103},
  {1, 51},
  {1, 84},
  {2, 43, 66},
  {15, 0, 11, 21, 24, 27, 40, 45, 55, 58, 60, 75, 79, 82, 88, 102},
  {1, 93},
  {1, 72},
  {2, 1, 19},
  {4, 20, 57, 73, 101},
  {8, 8, 22, 28, 48, 62, 76, 83, 89},
  {3, 54, 61, 74},
  {2, 37, 65},
  {4, 13, 29, 90, 94},
  {5, 6, 17, 53, 69, 97},
  {7, 4, 10, 35, 42, 56, 71, 77},
  {9, 5, 16, 25, 38, 46, 52, 85, 92, 98},
  {8, 7, 14, 23, 33, 44, 50, 68, 95},
  {1, 81},
  {1, 30},
  {4, 12, 34, 41, 80},
  {1, 2},
  {1, 63}
};


int ledIndex(int letterIndex) {
  if(letterIndex < 0 || letterIndex >= LETTER_PER_LINE * NB_LINES) 
    return -1;
    
  int ledIndex = LETTER_PER_LINE * (letterIndex / LETTER_PER_LINE);
  if((letterIndex / LETTER_PER_LINE) % 2 == 1) {
    ledIndex += (LETTER_PER_LINE - letterIndex % LETTER_PER_LINE) - 1;
  }
  else {
    ledIndex += letterIndex % LETTER_PER_LINE;
  }
  return ledIndex;
}


int xy(int x, int y) {
  return x + y * LETTER_PER_LINE;
}


int getStringIndex(String str, int fromIndex = 0) {
  int line = 0;
  for(String l : letters) {
    int id = l.indexOf(str);
    if(id >= 0 && line * LETTER_PER_LINE + id >= fromIndex) {
      return line * LETTER_PER_LINE + id;
    }
    line++;
  }
  return -1;
}


int getCharIndex(char c, bool atRandom = true, int fromIndex = 0) {
  if(c >= 'a' && c <= 'z') {
    if(atRandom) {
      int nbOccurences = indexes[c - 'a'][0];
      if(fromIndex > indexes[c - 'a'][nbOccurences])
        return -1;
      else
        return indexes[c - 'a'][1 + random(nbOccurences)];
    }
    else {
      int line = 0;
      for(String l : letters) {
        int id = l.indexOf(c);
        if(id >= 0 && line * LETTER_PER_LINE + id >= fromIndex) {
          return line * LETTER_PER_LINE + id;
        }
        line++;
      }
    }
  }
  return -1;
}


char getChar(int index) {
  if(index >= 0 && index < LETTER_PER_LINE * NB_LINES)
    return letters[index / LETTER_PER_LINE].charAt(index % LETTER_PER_LINE);
  else
    return '*';
}


void displayDigitOnLedArray(uint8_t digit, bool* ledStateArray, int xOffset) {
  if(digit < 10 && xOffset <= 10) {
    for(int y_digit = 0; y_digit < 8; y_digit++) {
      for(int x_digit = 0; x_digit < 3; x_digit++) {
        int ledStateArrayIndex = xy(xOffset + x_digit, y_digit);
        ledStateArray[ledStateArrayIndex] = digits[digit][y_digit][x_digit];
      }
    }
  }
}
#endif // DISPLAY_UTILS_H
