#include <Arduino.h>
#include <PID_v1.h>

enum Direction { UP, STOP, DOWN };

class Blind {
  private:
    int pinUp;
    int pinDown;
    int pinSpeed;
    int pinEncoder;
    double speedSetPoint;
    double speedValOutput;
    Direction currentDirection = STOP;
    Direction requestedDirection = STOP;
    double encoder_num_pulses = 0;
    int movementDuration = 0;
    int movementTicState = 0; // distance from top
    double Kp = 0.6, Ki = 4, Kd = 0;
    PID myPID;

    void handleChangeDirection() {
        if (currentDirection != requestedDirection) {
            switch (requestedDirection) {
            case UP:
                Serial.println("RUSZA SIE");
                movementDuration = 0;
                digitalWrite(pinUp, HIGH);
                digitalWrite(pinDown, LOW);
                break;
            case DOWN:
                Serial.println("RUSZA SIE");
                movementDuration = 0;
                digitalWrite(pinDown, HIGH);
                digitalWrite(pinUp, LOW);
                break;
            case STOP:
                digitalWrite(pinDown, LOW);
                digitalWrite(pinUp, LOW);
            }
            currentDirection = requestedDirection;
        }
    }
    void handleSpeed() {
        int result = myPID.Compute();
        if (result) {
            encoder_num_pulses = 0;
            analogWrite(pinSpeed, (int) speedValOutput);
        }
    }

  public:
    Blind(int pinUp, int pinDown, int pinSpeed, int pinEncoder, int speed)
        : myPID(&encoder_num_pulses, &speedValOutput, &speedSetPoint, Kp, Ki,
                Kd, DIRECT) {
        this->pinUp = pinUp;
        this->pinDown = pinDown;
        this->pinSpeed = pinSpeed;
        this->speedSetPoint = speed;
        this->pinEncoder = pinEncoder;
    }

    // YOU HAVE TO RUN attachInterupt(pinEncoder, func, CHANGE); yourself in
    // setup, 'func' has to look like this
    // void func() {
    //     blind1.incrementWheelTic()
    // }
    void init() { // has to be run in setup
        pinMode(pinUp, OUTPUT);
        pinMode(pinDown, OUTPUT);
        digitalWrite(pinUp, LOW);
        digitalWrite(pinDown, LOW);

        myPID.SetMode(AUTOMATIC);
        myPID.SetSampleTime(100);
        myPID.SetOutputLimits(0, 1023);
    }
    void handleMove() { // has to be run in arduino loop
        handleSpeed();
        handleChangeDirection();
    }
    void go(Direction dir) { requestedDirection = dir; }
    void ICACHE_RAM_ATTR incrementWheelTic() {
        encoder_num_pulses++;
        movementDuration++;
        if (currentDirection == DOWN) {
            movementTicState++;
        } else if (currentDirection == UP) {
            movementTicState--;
        }
    }
    int getMovementDuration() const { return movementDuration; }
    int getState() const { return movementTicState; }
    Direction getDirection() { return currentDirection; }
    void setSpeed(int speed) { speedSetPoint = speed; }
    void resetState() { movementTicState = 0; }
};

enum State { OPEN, MOVING, CLOSE };
class BlindMovement {
  private:
    Blind& blind;
    int steps;
    boolean autoMode = false;

  public:
    BlindMovement(Blind& blind_, int steps_) : blind(blind_) { steps = steps_; }
    void move(Direction dir) {
        blind.go(dir);
        autoMode = true;
    }
    void handleMovement() {
        Direction dir = blind.getDirection();
        int currentState = blind.getState();
        if(!autoMode){
            return;
        }
        // Stop blinds at the window boundaries.
        if (dir == UP && currentState <= 0) {
            blind.go(STOP);
            autoMode = false;
        } else if (dir == DOWN && currentState >= steps) {
            blind.go(STOP);
            autoMode = false;
        }
    };
};
