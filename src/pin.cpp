// Includes
#include "LPC17xx.h"
#include "pin.h"
#include "interrupt.h"

// Namespaces
using namespace System;

namespace System {

	Pin::Pin (uint32_t pin) : _pin(pin) {
	}

	void GPIO::setFunction (uint32_t pin, GPIO::Function function) {
		if (pin > PIN(4, 31)) {
			return;
		}

		uint32_t index = pin >> 4;
		pin = (pin & 0xF) << 1;

		uint32_t* LPC_PINSEL = (uint32_t*) &(LPC_PINCON->PINSEL0);
		LPC_PINSEL[index] = (LPC_PINSEL[index] & ~(0x03 << pin)) | (((uint32_t) function) << pin);
	}

	void GPIO::setPullMode (uint32_t pin, Pin::PullMode mode) {
		if (pin > PIN(4, 31)) {
			return;
		}

		uint32_t index = pin >> 4;
		pin = (pin & 0xF) << 1;

		uint32_t* LPC_PINMODE = (uint32_t*) &(LPC_PINCON->PINMODE0);
		LPC_PINMODE[index] = (LPC_PINMODE[index] & ~(0x03 << pin)) | (((uint32_t) mode) << pin);
	}

	void GPIO::setOpenDrain (uint32_t pin, bool open_drain) {
		if (pin > PIN(4, 31)) {
			return;
		}

		uint32_t port = pin >> 5;
		pin = pin & 0x1F;
		uint32_t* LPC_PINMODE_OD = (uint32_t*) &(LPC_PINCON->PINMODE_OD0);
		if (open_drain) {
			LPC_PINMODE_OD[port] |= (1 << pin);
		} else {
			LPC_PINMODE_OD[port] &= ~(1 << pin);
		}
	}

	void GPIO::setDirection (uint32_t pin, Pin::Direction direction) {
		if (pin > PIN(4, 31)) {
			return;
		}

		LPC_GPIO_TypeDef* LPC_GPIO = (LPC_GPIO_TypeDef*)(LPC_GPIO_BASE + (pin & ~0x1F));
		pin = pin & 0x1F;
		if (direction == Pin::Direction::input) {
			LPC_GPIO->FIODIR &= ~(1 << pin);
		} else {
			LPC_GPIO->FIODIR |= (1 << pin);
		}
	}

	void GPIO::set (uint32_t pin) {
		if (pin > PIN(4, 31)) {
			return;
		}

		LPC_GPIO_TypeDef* LPC_GPIO = (LPC_GPIO_TypeDef*)(LPC_GPIO_BASE + (pin & ~0x1F));
		pin = pin & 0x1F;
		LPC_GPIO->FIOSET = (1 << pin);
	}

	void GPIO::clear (uint32_t pin) {
		if (pin > PIN(4, 31)) {
			return;
		}

		LPC_GPIO_TypeDef* LPC_GPIO = (LPC_GPIO_TypeDef*)(LPC_GPIO_BASE + (pin & ~0x1F));
		pin = pin & 0x1F;
		LPC_GPIO->FIOCLR = (1 << pin);
	}

	void GPIO::write (uint32_t pin, Pin::Level level) {
		if (pin > PIN(4, 31)) {
			return;
		}

		LPC_GPIO_TypeDef* LPC_GPIO = (LPC_GPIO_TypeDef*)(LPC_GPIO_BASE + (pin & ~0x1F));
		pin = pin & 0x1F;

		if (level == Pin::Level::low) {
			LPC_GPIO->FIOCLR = (1 << pin);
		} else {
			LPC_GPIO->FIOSET = (1 << pin);
		}
	}

	void GPIO::writePort (uint32_t pin_lsb, uint32_t mask, uint32_t value) {
		if (pin_lsb > PIN(4, 31)) {
			return;
		}

		LPC_GPIO_TypeDef* LPC_GPIO = (LPC_GPIO_TypeDef*)(LPC_GPIO_BASE + (pin_lsb & ~0x1F));
		LPC_GPIO->FIOMASK = ~mask;
		LPC_GPIO->FIOPIN = value;
		LPC_GPIO->FIOMASK = 0;
	}

	void GPIO::writeByte (uint32_t pin_lsb, uint8_t value) {
		if (pin_lsb > PIN(4, 31)) {
			return;
		}

		LPC_GPIO_TypeDef* LPC_GPIO = (LPC_GPIO_TypeDef*)(LPC_GPIO_BASE + (pin_lsb & ~0x1F));
		if (pin_lsb == 0) {
			LPC_GPIO->FIOPIN0 = value;
		} else if (pin_lsb == 8) {
			LPC_GPIO->FIOPIN1 = value;
		} else if (pin_lsb == 16) {
			LPC_GPIO->FIOPIN2 = value;
		} else if (pin_lsb == 24) {
			LPC_GPIO->FIOPIN3 = value;
		}
	}

	void GPIO::writeHalfword (uint32_t pin_lsb, uint16_t value) {
		if (pin_lsb > PIN(4, 31)) {
			return;
		}

		LPC_GPIO_TypeDef* LPC_GPIO = (LPC_GPIO_TypeDef*)(LPC_GPIO_BASE + (pin_lsb & ~0x1F));
		if (pin_lsb == 0) {
			LPC_GPIO->FIOPINL = value;
		} else if (pin_lsb == 16) {
			LPC_GPIO->FIOPINH = value;
		}
	}

	Pin::Level GPIO::read (uint32_t pin) {
		if (pin > PIN(4, 31)) {
			return Pin::Level::low;
		}

		LPC_GPIO_TypeDef* LPC_GPIO = (LPC_GPIO_TypeDef*)(LPC_GPIO_BASE + (pin & ~0x1F));
		pin = pin & 0x1F;
		if (LPC_GPIO->FIOPIN & (1 << pin)) {
			return Pin::Level::high;
		} else {
			return Pin::Level::low;
		}
	}

	GPIOPin::GPIOPin (uint32_t pin) : Pin(pin) {
	}

	void GPIOPin::setFunction (GPIO::Function function) {
		GPIO::setFunction(_pin, function);
	}

	void GPIOPin::setDirection (Pin::Direction direction) {
		GPIO::setDirection(_pin, direction);
	}

	void GPIOPin::setPullMode (Pin::PullMode mode) {
		GPIO::setPullMode(_pin, mode);
	}

	void GPIOPin::setOpenDrain (bool open_drain) {
		GPIO::setOpenDrain(_pin, open_drain);
	}

	void GPIOPin::set (void) {
		GPIO::set(_pin);
	}

	void GPIOPin::clear (void) {
		GPIO::clear(_pin);
	}

	void GPIOPin::write (Pin::Level level) {
		GPIO::write(_pin, level);
	}

	Pin::Level GPIOPin::read (void) {
		return GPIO::read(_pin);
	}

	ExternalInterruptPin::ExternalInterruptPin (uint32_t pin) : GPIOPin(pin) {
	}

	void ExternalInterruptPin::enable (Mode mode, Polarity polarity) {
		if ((_pin < PIN(2, 10)) || (_pin > PIN(2, 13)))
			return;

		setFunction(GPIO::Function::alternate_1);
		setDirection(Pin::Direction::input);

		uint32_t interrupt = (_pin & 0x1F) - 10;
		if (mode == ExternalInterruptPin::Mode::level) {
			LPC_SC->EXTMODE &= ~(1 << interrupt);
		} else {
			LPC_SC->EXTMODE |= (1 << interrupt);
		}

		if (polarity == ExternalInterruptPin::Polarity::low) {
			LPC_SC->EXTPOLAR &= ~(1 << interrupt);
		} else {
			LPC_SC->EXTPOLAR |= (1 << interrupt);
		}

		Interrupt::enable(IRQn_Type ((uint32_t) EINT0_IRQn + interrupt));
	}

	void ExternalInterruptPin::disable (void) {
		if ((_pin < PIN(2, 10)) || (_pin > PIN(2, 13)))
			return;

		uint32_t interrupt = (_pin & 0x1F) - 10;
		Interrupt::disable(IRQn_Type ((uint32_t) EINT0_IRQn + interrupt));
		setFunction(GPIO::Function::gpio);
	}

	bool ExternalInterruptPin::isFlagged (void) {
		if ((_pin < PIN(2, 10)) || (_pin > PIN(2, 13)))
			return false;

		uint32_t interrupt = (_pin & 0x1F) - 10;
		return ((LPC_SC->EXTINT & (1 << interrupt)) != 0);
	}

	void ExternalInterruptPin::clearFlag (void) {
		if ((_pin < PIN(2, 10)) || (_pin > PIN(2, 13)))
			return;

		uint32_t interrupt = (_pin & 0x1F) - 10;
		LPC_SC->EXTINT |= (1 << interrupt);
	}

	GPIOInterruptPin::GPIOInterruptPin (uint32_t pin) : GPIOPin(pin) {
	}

	void GPIOInterruptPin::enable (void) {
		Interrupt::enable(EINT3_IRQn);
	}

	void GPIOInterruptPin::disable (void) {
		Interrupt::disable(EINT3_IRQn);
	}

	void GPIOInterruptPin::enable (Polarity polarity) {
		if ((_pin >= PIN(0, 0)) && (_pin <= PIN(0, 31))) {
			uint32_t pin = _pin & 0x1F;
			if (polarity == GPIOInterruptPin::Polarity::rising) {
				LPC_GPIOINT->IO0IntEnR |= (1 << pin);
			} else {
				LPC_GPIOINT->IO0IntEnF |= (1 << pin);
			}
		} else if ((_pin >= PIN(2, 0)) && (_pin <= PIN(2, 31))) {
			uint32_t pin = _pin & 0x1F;
			if (polarity == GPIOInterruptPin::Polarity::rising) {
				LPC_GPIOINT->IO2IntEnR |= (1 << pin);
			} else {
				LPC_GPIOINT->IO2IntEnF |= (1 << pin);
			}
		}
	}

	void GPIOInterruptPin::disable (Polarity polarity) {
		if ((_pin >= PIN(0, 0)) && (_pin <= PIN(0, 31))) {
			uint32_t pin = _pin & 0x1F;
			if (polarity == GPIOInterruptPin::Polarity::rising) {
				LPC_GPIOINT->IO0IntEnR &= ~(1 << pin);
			} else {
				LPC_GPIOINT->IO0IntEnF &= ~(1 << pin);
			}
		} else if ((_pin >= PIN(2, 0)) && (_pin <= PIN(2, 31))) {
			uint32_t pin = _pin & 0x1F;
			if (polarity == GPIOInterruptPin::Polarity::rising) {
				LPC_GPIOINT->IO2IntEnR &= ~(1 << pin);
			} else {
				LPC_GPIOINT->IO2IntEnF &= ~(1 << pin);
			}
		}
	}

	bool GPIOInterruptPin::isFlagged (Polarity polarity) {
		if ((_pin >= PIN(0, 0)) && (_pin <= PIN(0, 31))) {
			uint32_t pin = _pin & 0x1F;
			if (polarity == GPIOInterruptPin::Polarity::rising) {
				return ((LPC_GPIOINT->IO0IntStatR & (1 << pin)) != 0);
			} else {
				return ((LPC_GPIOINT->IO0IntStatF & (1 << pin)) != 0);
			}
		} else if ((_pin >= PIN(2, 0)) && (_pin <= PIN(2, 31))) {
			uint32_t pin = _pin & 0x1F;
			if (polarity == GPIOInterruptPin::Polarity::rising) {
				return ((LPC_GPIOINT->IO2IntStatR & (1 << pin)) != 0);
			} else {
				return ((LPC_GPIOINT->IO2IntStatF & (1 << pin)) != 0);
			}
		}
		return false;
	}

	void GPIOInterruptPin::clearFlag (void) {
		if ((_pin >= PIN(0, 0)) && (_pin <= PIN(0, 31))) {
			uint32_t pin = _pin & 0x1F;
			LPC_GPIOINT->IO0IntClr |= (1 << pin);
		} else if ((_pin >= PIN(2, 0)) && (_pin <= PIN(2, 31))) {
			uint32_t pin = _pin & 0x1F;
			LPC_GPIOINT->IO2IntClr |= (1 << pin);
		}
	}
}
