#pragma once

// Macros
#define PIN(port, pin)				(((port) << 5) + (pin))

namespace System::Pin {

	// Type definitions
	typedef enum {
		input = 0,
		output = 1
	} Direction;

	typedef enum {
		gpio = 0,
		primary = 0,
		alternate_1 = 1,
		alternate_2 = 2,
		alternate_3 = 3
	} Function;

	typedef enum {
		pull_up = 0,
		repeat = 1,
		no_pull = 2,
		none = 2,
		pull_down = 3
	} PullMode;

	typedef enum {
		low = 0,
		high = 1
	} Level;

	// Function prototypes
	void setFunction (uint32_t pin, Function function);
	void setDirection (uint32_t pin, Direction direction);
	void setPullMode (uint32_t pin, PullMode mode);
	void setOpenDrain (uint32_t pin, bool open_drain);
	void set (uint32_t pin);
	void clear (uint32_t pin);
	void write (uint32_t pin, Level level);
	void write (uint32_t pin_lsb, uint32_t mask, uint32_t value);
	void writeByte (uint32_t pin_lsb, uint8_t value);
	void writeHalfword (uint32_t pin_lsb, uint16_t value);
	Level read (uint32_t pin);

	namespace ExternalInterrupt {

		// Type definitions
		typedef enum {
			level = 0,
			edge = 1,
		} Mode;

		typedef enum {
			low = 0,
			falling = 0,
			high = 1,
			rising = 1,
		} Polarity;

		// Function prototypes
		void enable (uint32_t pin, Mode mode, Polarity polarity);
		void disable (uint32_t pin);
		bool isFlagged (uint32_t pin);
		void clearFlag (uint32_t pin);
	}

	namespace GPIOInterrupt {

		// Type definitions
		typedef enum {
			rising = 0,
			falling = 1
		} Polarity;

		// Function prototypes
		void enable (void);
		void disable (void);
		void enable (uint32_t pin, Polarity polarity);
		void disable (uint32_t pin, Polarity polarity);
		bool isFlagged (uint32_t pin, Polarity polarity);
		void clearFlag (uint32_t pin);
	}
}
