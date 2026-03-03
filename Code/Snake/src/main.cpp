#include <FastLED.h>
#include <LEDMatrix.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// LED Matrix Konfiguration
#define NUM_LEDS_PER_STRIP 256
#define DATA_PIN_UPPER 25
#define DATA_PIN_LOWER 26
#define MATRIX_WIDTH 32
#define MATRIX_HEIGHT 16
#define MATRIX_TYPE VERTICAL_ZIGZAG_MATRIX

// Joystick Pins
#define JOYSTICK_BUTTON_PIN 32
#define JOYSTICK_X_PIN 34
#define JOYSTICK_Y_PIN 35

#define TASK_DELAY 10

// Richtung
enum Direction { UP, RIGHT, DOWN, LEFT };

// LED Arrays
CRGB leds_upper[NUM_LEDS_PER_STRIP];
CRGB leds_lower[NUM_LEDS_PER_STRIP];
CRGB leds_array[NUM_LEDS_PER_STRIP * 2];
cLEDMatrix<-MATRIX_WIDTH, MATRIX_HEIGHT, MATRIX_TYPE> leds;

// Globale Spielflags
volatile int currentTask = 0;  // 0 = Snake, 1 = Testanimation

// Struktur für LED-Adresse
struct LedAddress {
  CRGB* array;
  int index;
};

// Snake-Variablen
int snakeX[100], snakeY[100];
int snakeLength = 3;
Direction snakeDirection = RIGHT;
int foodX, foodY;
unsigned long lastMoveTime = 0;
int gameSpeed = 400;
bool gameOver = false;
int score = 0;

// Mapping-Funktion
LedAddress mapXY(int x, int y) {
  LedAddress result;
  int led;
  if (y < 8) {
    led = x * 8 + ((x % 2 == 0) ? y : 7 - y);
    result.array = leds_lower;
    result.index = led;
  } else {
    int flippedX = MATRIX_WIDTH - 1 - x;
    int flippedY = 15 - y;
    led = flippedX * 8 + ((flippedX % 2 == 0) ? flippedY : 7 - flippedY);
    result.array = leds_upper;
    result.index = led;
  }
  return result;
}

// Snake-Funktionen
void generateFood() {
  bool valid;
  do {
    valid = true;
    foodX = random(MATRIX_WIDTH);
    foodY = random(MATRIX_HEIGHT);
    for (int i = 0; i < snakeLength; i++) {
      if (foodX == snakeX[i] && foodY == snakeY[i]) valid = false;
    }
  } while (!valid);
}

void initGame() {
  snakeLength = 3;
  for (int i = 0; i < snakeLength; i++) {
    snakeX[i] = MATRIX_WIDTH / 2 - i;
    snakeY[i] = MATRIX_HEIGHT / 2;
  }
  snakeDirection = RIGHT;
  score = 0;
  generateFood();
  gameSpeed = 400;
  gameOver = false;
  lastMoveTime = millis();
}

void readJoystick() {
  int x = analogRead(JOYSTICK_X_PIN);
  int y = analogRead(JOYSTICK_Y_PIN);
  const int mid = 2048;
  const int thresh = 1000;

  if (y > mid + thresh && snakeDirection != LEFT) snakeDirection = RIGHT;
  else if (y < mid - thresh && snakeDirection != RIGHT) snakeDirection = LEFT;
  else if (x > mid + thresh && snakeDirection != DOWN) snakeDirection = UP;
  else if (x < mid - thresh && snakeDirection != UP) snakeDirection = DOWN;
}

void moveSnake() {
  if (millis() - lastMoveTime < gameSpeed || gameOver) return;
  lastMoveTime = millis();

  for (int i = snakeLength - 1; i > 0; i--) {
    snakeX[i] = snakeX[i - 1];
    snakeY[i] = snakeY[i - 1];
  }

  switch (snakeDirection) {
    case UP: snakeY[0]--; break;
    case RIGHT: snakeX[0]++; break;
    case DOWN: snakeY[0]++; break;
    case LEFT: snakeX[0]--; break;
  }

  if (snakeX[0] < 0 || snakeX[0] >= MATRIX_WIDTH ||
      snakeY[0] < 0 || snakeY[0] >= MATRIX_HEIGHT) {
    gameOver = true;
    return;
  }

  for (int i = 1; i < snakeLength; i++) {
    if (snakeX[0] == snakeX[i] && snakeY[0] == snakeY[i]) {
      gameOver = true;
      return;
    }
  }

  if (snakeX[0] == foodX && snakeY[0] == foodY) {
    if (snakeLength < 100) snakeLength++;
    score++;
    gameSpeed = max(80, gameSpeed - 10);
    generateFood();
  }
}

void drawGame() {
  FastLED.clear();
  for (int i = 0; i < snakeLength; i++) {
    if (snakeX[i] >= 0 && snakeX[i] < MATRIX_WIDTH &&
        snakeY[i] >= 0 && snakeY[i] < MATRIX_HEIGHT) {
      LedAddress addr = mapXY(snakeX[i], snakeY[i]);
      addr.array[addr.index] = (i == 0) ? CRGB::White :
        CRGB(0, map(i, 1, snakeLength - 1, 200, 50), 0);
    }
  }

  if (!gameOver) {
    LedAddress addr = mapXY(foodX, foodY);
    int brightness = 150 + 100 * sin(millis() / 200.0);
    addr.array[addr.index] = CRGB(brightness, 0, 0);
  }

  FastLED.show();
}

// === TASKS ===

void snakeTask(void* pvParameters) {
  while (1) {
    if (currentTask == 0) {
      readJoystick();
      moveSnake();
      drawGame();
      if (gameOver && millis() - lastMoveTime > 3000) initGame();
    }
    vTaskDelay(pdMS_TO_TICKS(TASK_DELAY));
  }
}

void altAnimationTask(void* pvParameters) {
  while (1) {
    if (currentTask == 1) {
      for (int i = 0; i < NUM_LEDS_PER_STRIP * 2; i++) {
        leds_array[i] = CRGB::Blue;
      }
      FastLED.show();
    }
    vTaskDelay(pdMS_TO_TICKS(TASK_DELAY));
  }
}

void joystickButtonTask(void* pvParameters) {
  bool lastState = HIGH;
  while (1) {
    bool currentState = digitalRead(JOYSTICK_BUTTON_PIN);
    if (lastState == HIGH && currentState == LOW) {
      currentTask = (currentTask + 1) % 2;
      Serial.print("Switched to Task ");
      Serial.println(currentTask);
      delay(200); // Entprellung
    }
    lastState = currentState;
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(JOYSTICK_BUTTON_PIN, INPUT_PULLUP);
  pinMode(JOYSTICK_X_PIN, INPUT);
  pinMode(JOYSTICK_Y_PIN, INPUT);

  FastLED.addLeds<WS2812B, DATA_PIN_UPPER, GRB>(leds_upper, NUM_LEDS_PER_STRIP);
  FastLED.addLeds<WS2812B, DATA_PIN_LOWER, GRB>(leds_lower, NUM_LEDS_PER_STRIP);
  FastLED.setBrightness(50);

  for (int i = 0; i < NUM_LEDS_PER_STRIP; i++) {
    leds_array[i] = leds_lower[i];
    leds_array[i + NUM_LEDS_PER_STRIP] = leds_upper[i];
  }
  leds.SetLEDArray(leds_array);

  FastLED.clear();
  FastLED.show();
  randomSeed(analogRead(36));

  initGame();

  // FreeRTOS Tasks
  xTaskCreate(snakeTask, "SnakeTask", 4096, NULL, 1, NULL);
  xTaskCreate(altAnimationTask, "AltTask", 2048, NULL, 1, NULL);
  xTaskCreate(joystickButtonTask, "JoystickTask", 1024, NULL, 2, NULL);
}

void loop() {
  // leer; FreeRTOS übernimmt alles
}