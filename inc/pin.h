#pragma once

#include <cstdint>

namespace System {

	// Compress port & pin into a single number
	static constexpr uint32_t PIN(uint32_t port, uint32_t pin) {
		return (port << 5) + pin;
	}

	class Pin {

	public:

		// Type definitions
		typedef enum {
			input = 0,
			output = 1
		} Direction;

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

	protected:
		uint32_t _pin;

	public:

		// Function prototypes
		Pin (uint32_t pin);
		virtual void setDirection (Direction direction) = 0;
		virtual void setPullMode (PullMode mode) = 0;
		virtual void setOpenDrain (bool open_drain) = 0;
		virtual void set (void) = 0;
		virtual void clear (void) = 0;
		virtual void write (Level level) = 0;
		virtual Level read (void) = 0;
	};

	class GPIO {

	public:

		// Type definitions
		typedef enum {
			gpio = 0,
			primary = 0,
			alternate_1 = 1,
			alternate_2 = 2,
			alternate_3 = 3
		} Function;

	public:

		static void setFunction (uint32_t pin, GPIO::Function function);

		static void setDirection (uint32_t pin, Pin::Direction direction);
		static void setPullMode (uint32_t pin, Pin::PullMode mode);
		static void setOpenDrain (uint32_t pin, bool open_drain);
		static void set (uint32_t pin);
		static void clear (uint32_t pin);
		static void write (uint32_t pin, Pin::Level level);
		static Pin::Level read (uint32_t pin);

		static void writePort (uint32_t pin_lsb, uint32_t mask, uint32_t value);
		static void writeByte (uint32_t pin_lsb, uint8_t value);
		static void writeHalfword (uint32_t pin_lsb, uint16_t value);
	};

	class GPIOPin : public Pin {

	public:

		// Function prototypes
		GPIOPin (uint32_t pin);
		void setFunction (GPIO::Function function);
		void setDirection (Pin::Direction direction);
		void setPullMode (Pin::PullMode mode);
		void setOpenDrain (bool open_drain);
		void set (void);
		void clear (void);
		void write (Pin::Level level);
		Pin::Level read (void);
	};

	class ExternalInterruptPin : public GPIOPin {

	public:

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

	public:

		// Function prototypes
		ExternalInterruptPin (uint32_t pin);
		void enable (Mode mode, Polarity polarity);
		void disable (void);
		bool isFlagged (void);
		void clearFlag (void);
	};

	class GPIOInterruptPin : public GPIOPin {

	public:

		// Type definitions
		typedef enum {
			rising = 0,
			falling = 1
		} Polarity;

	public:

		// Function prototypes
		GPIOInterruptPin (uint32_t pin);
		static void enable (void);
		static void disable (void);
		void enable (Polarity polarity);
		void disable (Polarity polarity);
		bool isFlagged (Polarity polarity);
		void clearFlag (void);
	};
}
