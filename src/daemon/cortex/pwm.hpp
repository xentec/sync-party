#pragma once

#include "asio.hpp"
#include "def.hpp"
#include "logger.hpp"
#include "types.hpp"

/**
 * @brief PWM control over sysfs-interface
 */
struct PWM
{
	/**
	 * @param period PWM period in ns
	 */
	PWM(u32 period);
	~PWM();

	void enable(bool on);

	/**
	 * @param pwm  duty cycle in ns
	 */
	void set_duty_cycle(u32 pwm);
private:
	struct {
		int dc, enable;
	} fds;
};

/**
 * @brief Wheel steering
 */
struct Steering
{
	static const def::Scale limit;

	Steering();

	/**
	 * @param deg  Steering direction in degrees between -90° and 90°
	 * @note Input will be clamped to Steering::limit
	 * @return true if input was clamped
	 */
	bool steer(i32 deg);

private:
	loggr logger;
	PWM pwm_ctrl;
	u32 pwm_prev;
};
