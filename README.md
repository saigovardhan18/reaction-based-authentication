# Arduino-Based Human Reaction Signature Authentication System

An embedded systems project that explores behavioral authentication using human reaction-time patterns. Built using an Arduino-compatible microcontroller, Arduino C++, an LED, and a push button.

## Overview

Instead of using a traditional password, this project measures how quickly a user responds to randomly timed LED signals. The Arduino collects reaction-time measurements, creates a behavioral profile, and compares it with new measurements during authentication.

The system displays **ACCESS GRANTED** or **ACCESS DENIED** based on a calculated similarity score.

This project is designed for educational purposes to demonstrate embedded programming, timing, data processing, and basic authentication logic.

## Hardware Requirements

* Arduino-compatible development board
* LED (built-in LED or external LED with a suitable resistor)
* Momentary push button
* USB cable
* Breadboard and jumper wires (optional)

## Circuit Connections

| Component              | Arduino Connection                       |
| ---------------------- | ---------------------------------------- |
| LED                    | Digital Pin 2, if supported by the board |
| Push button terminal 1 | Digital Pin 4                            |
| Push button terminal 2 | GND                                      |

The button uses `INPUT_PULLUP`, meaning:

* `HIGH` = button released
* `LOW` = button pressed

Check your Arduino board's pinout before connecting components. Pin assignments and timing capabilities may vary between boards.

## How It Works

### 1. Registration

The user selects the registration option through the Serial Monitor.

* The LED remains OFF during a randomized waiting period.
* The LED turns ON to signal the user.
* The Arduino measures the time until the button is pressed.
* This process repeats for five trials.
* The Arduino calculates the average reaction time.
* The average is stored in RAM as the registered profile.

### 2. Authentication

The user selects the authentication option and performs five more reaction tests.

The Arduino calculates the new average and compares it with the registered average.

### 3. Similarity Calculation

The system calculates the absolute difference between the registered and current averages.

The similarity formula is:

`Similarity = max(0, 100 × (1 − Difference / Registered Average))`

A similarity score of at least 70% produces **ACCESS GRANTED**. Otherwise, the system displays **ACCESS DENIED**.

The threshold is configurable and intended only for demonstration.

## Software Requirements

* Arduino IDE
* Appropriate board support package
* USB drivers, if required
* Basic Arduino C++ knowledge

## How to Run

1. Install the Arduino IDE.
2. Connect your Arduino board to your computer through USB.
3. Open the project's `.ino` file.
4. Select the appropriate board and port.
5. Upload the code.
6. Open Serial Monitor at **115200 baud**, if supported by your board.
7. Select `1` to register a user.
8. Complete all five reaction tests.
9. Select `2` to authenticate.
10. Select `3` to view the registered profile, if supported by your code.

Follow the instructions displayed in the Serial Monitor.

## Programming Concepts Used

* GPIO input and output
* `pinMode()`, `digitalRead()`, and `digitalWrite()`
* `micros()` for reaction-time measurement
* Randomized delays
* Arrays and loops
* Functions and conditional statements
* Average calculations
* Similarity scoring and threshold-based decisions
* Serial communication

## Limitations

* Reaction time changes with fatigue, distraction, and practice.
* Different people can have similar reaction times.
* Five trials are insufficient for scientifically reliable biometric authentication.
* The 70% threshold has not been scientifically validated.
* The registered profile is stored in RAM and is lost when the board resets or loses power.
* The system is not suitable for real-world security applications.
* Timing precision and available pins depend on the selected Arduino board.

## Future Improvements

* Store profiles in non-volatile memory.
* Support multiple registered users.
* Include reaction-time variation as an additional feature.
* Collect larger datasets and evaluate authentication accuracy.
* Measure false acceptance and false rejection rates.
* Add a PIN-based verification layer.
* Build a display or web interface for better usability.

## Learning Outcomes

This project demonstrates how an Arduino-compatible microcontroller can collect human behavioral data, process measurements, and make a basic authentication decision. It combines embedded systems, timing, programming logic, and data analysis in one compact prototype.

## License

Choose an appropriate open-source license before distributing or reusing this project.

---

**Disclaimer:** This is an educational prototype, not a production-grade biometric authentication system. Its similarity score should not be interpreted as the probability that a user is genuine.
